#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "eloop.h"
#include "utils/includes.h"


#include "utils/common.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "drivers/driver.h"
#include "ap/hostapd.h"
#include "ap/ap_config.h"
#include "ap/ap_drv_ops.h"

#include "file_conf.h"
#include "smac_code.h"


#define MAX_BUF 3000

//#define LOCALUDP
//#define NETUDP
//#define USEIPV6

struct config_ac con_ac;
unsigned char wlan0_capa[21];

int fd_con;


#if defined(LOCALUDP)
	struct sockaddr_un addr;
	struct sockaddr_un local;
	int rn;
	
#else
	#if defined(USEIPV6)
		struct sockaddr_in6 client;
		struct sockaddr_in6 addr;
	#else
		struct sockaddr_in client;
		struct sockaddr_in addr;
	#endif
#endif

#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))



int address_size;

int capability_is_A(){
	if( CHECKBIT(wlan0_capa[8],1))return 1;
	else return 0;
}
int capability_is_B(){
	if( CHECKBIT(wlan0_capa[8],0))return 1;
	else return 0;
}
int capability_is_G(){
	if( CHECKBIT(wlan0_capa[8],2))return 1;
	else return 0;
}
int capability_is_N(){
	if( CHECKBIT(wlan0_capa[8],3))return 1;
	else return 0;
}

int capability_get_num_modes(){
	int num_modes = 0;
	
	if( CHECKBIT(wlan0_capa[8],0)) num_modes++;
	if( CHECKBIT(wlan0_capa[8],1)) num_modes++;
	if( CHECKBIT(wlan0_capa[8],2)) num_modes++;
	if( CHECKBIT(wlan0_capa[8],3)) num_modes++;
	
	return num_modes;
}

int capability_get_first_channel(){
	return (int)wlan0_capa[10];
}

int capability_get_num_channels(){
	return (int)wlan0_capa[12];
}

int capability_get_max_dBm(){
	return (int)wlan0_capa[14];
}

int capability_get_rates(int *rate_arr){
	int num_rates = 0;
	int i;
	for(i=0; i<8; i++){
		if( wlan0_capa[i]==0 ) continue;

		if( CHECKBIT(wlan0_capa[i],7) ){
			CLEARBIT(wlan0_capa[i],7);
			rate_arr[num_rates] = (int)(wlan0_capa[i] * -5);
		}else{
			rate_arr[num_rates] = (int)(wlan0_capa[i] * 5);
		}
		num_rates++;
	}
	return num_rates;
}

void capability_get_mac(char *buf){
	memcpy(buf, wlan0_capa + 15, 6);
}

void prep_beacon(int fd,struct hostapd_data *hapd,struct wpa_driver_ap_params *params){
	struct hostapd_data *h = hapd;
	short ssid_len = (short)h->conf->ssid.ssid_len;
	short key_len = (short) h->conf->ssid.wep.len[h->conf->ssid.wep.idx];
	
	
	wpa_printf(MSG_DEBUG,"SSID: %s ssid_len:%d\n",h->conf->ssid.ssid,ssid_len);
	wpa_printf(MSG_DEBUG,"IDX:%d   key_len:%d    key:%s  \n",h->conf->ssid.wep.idx,key_len,h->conf->ssid.wep.key[h->conf->ssid.wep.idx]);

	unsigned char A1 = 0;
	unsigned char A2 = 0;
	
	SETBIT(A1,7); 	//E (ESS)
	CLEARBIT(A1,6); //I (IBSS)
	CLEARBIT(A1,5); //C (CF Pollable)
	CLEARBIT(A1,4); //F (CF Pollable Request)
	if(params->privacy)		SETBIT(A1,3); //P (Privacy)
	else					CLEARBIT(A1,3); //P (Privacy)
	if(params->preamble)	SETBIT(A1,2); //S (Sort Preamble)
	else    				CLEARBIT(A1,2); //S (Sort Preamble)
	CLEARBIT(A1,1); //B (PBCC)
	CLEARBIT(A1,0); //A (Channel Agility)
	
	CLEARBIT(A2,7); //I (IBSS)
	CLEARBIT(A2,6); //I (IBSS)
	if(params->short_slot_time)		SETBIT(A2,5); //I (IBSS)
	else  							CLEARBIT(A2,5); //I (IBSS)
	CLEARBIT(A2,4); //I (IBSS)
	CLEARBIT(A2,3); //I (IBSS)
	CLEARBIT(A2,2); //I (IBSS)
	CLEARBIT(A2,1); //I (IBSS)
	CLEARBIT(A2,0); //I (IBSS)
	


	int tot_len = 19 + ssid_len + key_len;
	
	
	unsigned char buf[tot_len];
	
	wpa_printf(MSG_DEBUG,"Tot Len:%d \n",tot_len);
	
	buf[0] = 0; // Radio ID
	buf[1] = 0; // WLAN ID
	buf[2] = A1; // Flags Part1
	buf[3] = A2;  // Flags Part2
	
	buf[4] = h->conf->ssid.wep.idx;  // Key Index
	
	if(params->privacy)	buf[5] = 1;  // Key Status
	else 				buf[5] = 0;  // Key Status
	
	wpa_printf(MSG_DEBUG,"keylen: %d \n",key_len);
	
	
	//memcpy(buf+6, &key_len, 1);
	//memcpy(buf+7, &key_len, 1);
	buf[6] = *(&key_len + 1);  // Key Length Part1
	buf[7] = *(&key_len + 0);  // Key Length Part2
	
	wpa_printf(MSG_DEBUG,"keylen: %d %02X %02X \n",key_len,buf[6],buf[7]);
	
	if( key_len ) memcpy( buf + 8, h->conf->ssid.wep.key[h->conf->ssid.wep.idx], key_len);
	
	buf[8+key_len] = 0;   // Group TSC Part1
	buf[9+key_len] = 0;   // Group TSC Part2
	buf[10+key_len] = 0;  // Group TSC Part3
	buf[11+key_len] = 0;  // Group TSC Part4
	
	buf[12+key_len] = 0;  // Group TSC Part5
	buf[13+key_len] = 0;  // Group TSC Part6
	buf[14+key_len] = 0;  // QoS
	
	if(params->privacy)	buf[15+key_len] = 1;  // Auth Type
	else 				buf[15+key_len] = 0;  // Auth Type
	
	buf[16+key_len] = 1;  // Mac Mode
	buf[17+key_len] = 2;  // Tunnel Mode
	buf[18+key_len] = 1;  // Suppress SSID
	
	memcpy(buf + key_len + 19, h->conf->ssid.ssid, ssid_len);
	
	int i;
	for(i=0; i<tot_len; i++){
		if((i%4)==0)printf("\n");
		wpa_printf(MSG_DEBUG,"%02X ",buf[i]);
	}
	wpa_printf(MSG_DEBUG,"\n");
	
	send_response(fd, ADD_WLAN, buf, tot_len);
}

void ipc_send_ADD_WLAN(int fd,char *ssid, int len){

	int new_len = 19 + len;
	unsigned char cmd[new_len];
	
	
	int i;
	for(i=0; i<new_len; i++)cmd[i]=0;
	
	cmd[0] = 0; // Radio ID
	cmd[1] = 0; // WLAN ID
	cmd[2] = 128; // Flags Part1
	cmd[3] = 0;  // Flags Part2
	
	cmd[4] = 0;  // Key Index
	cmd[5] = 0;  // Key Status
	cmd[6] = 0;  // Key Length Part1
	cmd[7] = 0;  // Key Length Part2
	
	cmd[8] = 0;   // Group TSC Part1
	cmd[9] = 0;   // Group TSC Part2
	cmd[10] = 0;  // Group TSC Part3
	cmd[11] = 0;  // Group TSC Part4
	
	cmd[12] = 0;  // Group TSC Part5
	cmd[13] = 0;  // Group TSC Part6
	cmd[14] = 0;  // QoS
	cmd[15] = 0;  // Auth Type
	
	cmd[16] = 1;  // Mac Mode
	cmd[17] = 2;  // Tunnel Mode
	cmd[18] = 1;  // Suppress SSID
	
	
	memcpy(cmd+19, ssid, len);
	
	send_response(fd, ADD_WLAN, cmd, new_len);
}


void ipc_send_DEL_WLAN(int fd){
	unsigned char cmd[2];

	cmd[0] = 0; // Radio ID
	cmd[1] = 0; // WLAN ID
	wpa_printf(MSG_DEBUG,"DEL 1\n");
	send_response(fd, DEL_WLAN, cmd, 2);
}

void ipc_send_CLOSE_to_AC(int fd){
	char cmd[10];
	send_response(fd, CLOSE, cmd, 10);
}

void ipc_send_add_station(int fd, u8 *buf, int len){
	u16 fc;
	int status = -1;
	unsigned char cmd[6];
	struct ieee80211_mgmt *mgmt;
	struct ieee80211_hdr *hdr;
	mgmt = (struct ieee80211_mgmt *) buf;
	hdr = (struct ieee80211_hdr *) buf;
	
	fc = le_to_host16(hdr->frame_control);
	
	if( WLAN_FC_GET_STYPE(fc)==WLAN_FC_STYPE_ASSOC_RESP ){
		status = le_to_host16(mgmt->u.assoc_resp.status_code);
		
	}else if( WLAN_FC_GET_STYPE(fc)==WLAN_FC_STYPE_REASSOC_RESP){
		status = le_to_host16(mgmt->u.reassoc_resp.status_code);
		
	}else{
		wpa_printf(MSG_ERROR, "Error: ipc_send_add_station\n");
		return;
	}
	
	if(status == 0){
		memcpy(cmd,mgmt->da,6);
		send_response(fd, SET_ADDR, cmd, 6);
	}
	
	
}

void ipc_send_del_station(int fd, u8 *buf, int len){
	u16 fc;
	unsigned char cmd[6];
	struct ieee80211_hdr *hdr;
	hdr = (struct ieee80211_hdr *) buf;
	
	fc = le_to_host16(hdr->frame_control);
	
	if( WLAN_FC_GET_STYPE(fc) != WLAN_FC_STYPE_DISASSOC ){
		wpa_printf(MSG_DEBUG,"Il pacchetto non e' di tipo DISASSOC\n");
		return;
	}

	wpa_printf(MSG_DEBUG,"DEL ADDR: %02X %02X %02X %02X %02X %02X \n",hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],hdr->addr2[3],hdr->addr2[4],hdr->addr2[5]);

	memcpy(cmd,hdr->addr2,6);
	send_response(fd, DEL_ADDR, cmd, 6);

}

void ipc_send_80211_to_wtp(int fd, u8 *buf, int len){
	
	send_response(fd, DATE_TO_WTP, buf, len);
}

void send_response(int fd, u8 code, u8 *buf, int len){
	u8 tmp_buf[MAX_BUF];
	tmp_buf[0] = code;
	
	int n;
	
	#if defined(LOCALUDP)
		sprintf(tmp_buf + 1, "%05d", rn);
		memcpy(tmp_buf + 6, buf, len);
		n = sendto(fd, tmp_buf, len + 6, 0, (struct sockaddr *)&addr, address_size);
	#else
		memcpy(tmp_buf + 1, buf, len);
		n = sendto(fd, tmp_buf, len + 1, 0, (struct sockaddr *)&addr, address_size);
	#endif
	

	if ( n < 0 ) {
		perror("send");
		return;
	}
	
}


void management_recv(int fd, u8 code, u8 *buf, int len, void *hapd, void *inject_func){

	if(code == PING){
		send_response(fd, PONG, buf, len);
		
	}else if( code==DATE_TO_AC ){
		struct hostapd_data *h = hapd;
		void (*pointer_inject_frame_in_hostapd) (void*,unsigned char*,int);
		pointer_inject_frame_in_hostapd = inject_func;
		pointer_inject_frame_in_hostapd(h->drv_priv, buf, len);
		//hostapd_inject_frame_in_hostapd(h, buf, len);
		
	}else{
		wpa_printf(MSG_DEBUG,"ERROR IPC: received unrecognizedcode: %d\n",code);
	}

}

int recv_request(int fd,void *hapd, void *inject_func){
	char str[MAX_BUF];

	int n;
	
	#if defined(LOCALUDP)
		n = recvfrom(fd, str, MAX_BUF, 0, (struct sockaddr *)&local, &address_size);
	#else
		n = recvfrom(fd, str, MAX_BUF, 0, (struct sockaddr *)&addr, &address_size);
	#endif
	

    if(n<=0){
		end_ipc(fd);
		return -1;
	}
	
		
	management_recv(fd, str[0], str + 1, n -1, hapd, inject_func);	
}



int open_socket(){
	
	
	int fd_ac, n, con_res;
	
	char buffer[100];
	
	#if defined(LOCALUDP)
		srand((unsigned)time(0));
		fd_ac = socket(AF_UNIX, SOCK_DGRAM, 0);
		
	#elif defined(NETUDP)
		#if defined(USEIPV6)
			fd_ac = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		#else
			fd_ac = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		#endif	
		
	#else
		#if defined(USEIPV6)
			fd_ac = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
		#else
			fd_ac = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		#endif

	#endif
	
	#if defined(LOCALUDP)
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, con_ac.path_unix_socket);
		local.sun_family = AF_UNIX;
		
		while(1){
			rn = rand()%100000;
			sprintf(local.sun_path, "%s%05d", con_ac.path_unix_socket, rn);
			if( bind(fd_ac, (struct sockaddr *)&local, strlen(local.sun_path) + sizeof(local.sun_family))==-1){
				sleep(1); 
				continue; 
			}
			break;
		}
		wpa_printf(MSG_DEBUG,"Connect to %s from %s\n",addr.sun_path, local.sun_path);				
			
	#else
		#if defined(USEIPV6)
			addr.sin6_family = AF_INET6;
			addr.sin6_port = con_ac.ac_port;
			inet_pton(AF_INET6, con_ac.ip_ac, &addr.sin6_addr);
		#else
			addr.sin_family = AF_INET;
			addr.sin_port = con_ac.ac_port;
			addr.sin_addr.s_addr = inet_addr(con_ac.ip_ac);
			
		#endif
		wpa_printf(MSG_DEBUG,"Try connecting to %s:%d\n",con_ac.ip_ac, con_ac.ac_port);
		
	#endif
	
	address_size = sizeof(addr);
		
	while(1){
		#if defined(LOCALUDP)
			sprintf(buffer,"X%05dconnect",rn);
		#else
			sprintf(buffer,"Xconnect");
		#endif
		
		buffer[0] = CONNECT;
		n = sendto(fd_ac, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, address_size);
  
		#if defined(LOCALUDP)
			n = recvfrom(fd_ac, buffer, sizeof(buffer), 0, (struct sockaddr *)&local, &address_size);
		#else
			n = recvfrom(fd_ac, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &address_size);
		#endif
		
		if(buffer[0] == CONNECT_R){
			break;

		}else{
			sleep(0.5);
			
		}
	}
	
    return fd_ac;
}


void wait_capability_from_AC(int fd, void *hapd){
	unsigned char buffer[10];
	int n;
	
	sleep(0.5);
	
	while(1){
		
		#if defined(LOCALUDP)
			sprintf(buffer,"X%05d",rn);
		#else
			sprintf(buffer,"X");
		#endif
		
		buffer[0] = WANT_GOLIVE;
		n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, address_size);
  


		#if defined(LOCALUDP)
			n = recvfrom(fd, buffer, 10, 0, (struct sockaddr *)&local, &address_size);
		#else
			n = recvfrom(fd, buffer, 10, 0, (struct sockaddr *)&addr, &address_size);
		#endif
	
		if(n<=0){
			end_ipc(fd);
			return;
		}
		
		if(buffer[0] == SET_WTPRINFO){
			wpa_printf(MSG_DEBUG,"SET_WTPRINFO: %02X\n",buffer[1]);
			memcpy( wlan0_capa+8, buffer+1, 1);
			send_response(fd, SET_WTPRINFO_R, NULL, 0);
			
		}else if(buffer[0] == SET_RATES){
			wpa_printf(MSG_DEBUG,"SET_RATES: %02X %02X %02X %02X %02X %02X %02X %02X\n",buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);
			memcpy( wlan0_capa, buffer+1, 8);
			send_response(fd, SET_RATES_R, NULL, 0);
			
		}else if(buffer[0] == SET_MDC){
			wpa_printf(MSG_DEBUG,"SET_MDC: %02X %02X %02X %02X %02X %02X\n",buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6]);
			memcpy( wlan0_capa+9, buffer+1, 6);
			send_response(fd, SET_MDC_R, NULL, 0);
			
		}else if(buffer[0] == SET_MAC){
			wpa_printf(MSG_DEBUG,"SET_MAC: %02X %02X %02X %02X %02X %02X\n",buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6]);
			memcpy( wlan0_capa+15, buffer+1, 6);
			send_response(fd, SET_MAC_R, NULL, 0);
			
		}else if(buffer[0] == GOLIVE){
			wpa_printf(MSG_DEBUG,"GOLIVE:\n");
			send_response(fd, GOLIVE_R, NULL, 0);
			break;
			
		}else if(buffer[0] == HAVE_TO_WAIT){
			sleep(1);
		
		}else if(buffer[0] == CLOSE){
			end_ipc(fd);
			
		}else{
			wpa_printf(MSG_DEBUG,"Unknow code (%d) received in CONNECTed Fase\n",buffer[0]);
		}
	}
	
}

int end_ipc(int fd){
	
	ipc_send_CLOSE_to_AC(fd);
	if(fd>=0){
		eloop_unregister_read_sock(fd);
		if(close(fd)<0 ){
			
			return -1;
		}
	}
	
	if(	fd_con>=0 ){
		if(close(fd_con)<0 ){
			
		}
	}
	
	return 0;
}

int start_ipc(void *hapd,void *inject_func){
	ReadConfiguration(&con_ac);
	wpa_printf(MSG_DEBUG,"< DISCONNECTED >\n");
	
	int sockfd = open_socket();
	wpa_printf(MSG_DEBUG,"< CONNECTED >\n");
	
	wait_capability_from_AC(sockfd, hapd);
	wpa_printf(MSG_DEBUG,"< LIVE >\n");
	

	if(sockfd){
		if (eloop_register_read_sock(sockfd, recv_request, hapd, inject_func)) {
			wpa_printf(MSG_ERROR, "Clould not register IPC socket start_ipc");
			return 0;
		}
	}
	
	return sockfd;
}


