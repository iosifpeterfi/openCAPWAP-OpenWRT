#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "eloop.h"
#include "utils/includes.h"


#include "utils/common.h"
#include "drivers/driver.h"
#include "ap/hostapd.h"
#include "ap/ap_config.h"
#include "ap/ap_drv_ops.h"

#include "file_conf.h"
#include "smac_code.h"


#define MAX_BUF 3000

//#define LOCALUDP
//#define NETUDP
//#define NETSEQ
//#define USEIPV6


struct pre_txq {
	unsigned char *buf;
	int len;
	int free;
};

struct queue_qos {
	int queue_id;
	int cwmin;
	int cwmax;
	int aifs;
	int burst_time;
};

struct wlan_state {
	int rts;
	int frag;
	struct hostapd_freq_params freq_params;
	struct queue_qos que[4];
};

struct config_wtp con_wtp;
unsigned char wlan0_capa[21];
struct pre_txq buf_txq[8];



#if defined(LOCALUDP)
	struct sockaddr_un addr;
	struct sockaddr_un local;
	int rn;
	
#else
	#if defined(USEIPV6)
		struct sockaddr_in6 addr;
	#else
		struct sockaddr_in addr;
	#endif
#endif



int address_size;
struct wlan_state wl;
int fd_con;
struct sockaddr_un local, remote;



void ipc_send_CLOSE_to_WTP(int fd){
	char cmd[10];
	send_response(fd, CLOSE, cmd, 10);
}

void ipc_send_80211_to_ac(int fd, u8 *buf, int len){
	send_response(fd, DATE_TO_AC, buf, len);
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

/* HANDLE */

void SET_RTS_handle(int fd, u8 *buf, int len, void *hapd){
	struct hostapd_data *h = hapd;
	u8 tmp[MAX_BUF];
	memcpy(tmp,buf,len);
	tmp[len] = 0;

	int rts = atoi(tmp);
	wpa_printf(MSG_DEBUG, "SER RTS: %d",rts);
	
	wl.rts = rts;
	
	int ret = hostapd_set_rts(h, rts);
	
	u8 ret_str[10];
	sprintf(ret_str, "%d", ret);
	send_response(fd, SET_RTS_R, ret_str, strlen(ret_str)); 
}

void GET_RTS_handle(int fd, u8 *buf, int len, void *hapd){
	
	char tmp_str[100];
	sprintf(tmp_str, "%d", wl.rts);
	send_response(fd, GET_RTS_R, tmp_str, strlen(tmp_str));
	
}



void SET_FRAG_handle(int fd, u8 *buf, int len, void *hapd){
	struct hostapd_data *h = hapd;
	u8 tmp[MAX_BUF];
	memcpy(tmp,buf,len);
	tmp[len] = 0;

	int frag = atoi(tmp);
	wpa_printf(MSG_DEBUG, "SER FRAQ: %d",frag);
	wl.frag = frag;
	
	int ret = hostapd_set_frag(h, frag);
	u8 ret_str[10];
	sprintf(ret_str, "%d", ret);
	send_response(fd, SET_FRAG_R, ret_str, strlen(ret_str)); 
}

void GET_FRAG_handle(int fd, u8 *buf, int len, void *hapd){
	
	char tmp_str[100];
	sprintf(tmp_str, "%d", wl.frag);
	send_response(fd, GET_FRAG_R, tmp_str, strlen(tmp_str));
	
}


void SET_FREQ_handle(int fd, u8 *buf, int len, void *hapd){
	struct hostapd_data *h = hapd;
	char str[5][10];
	
	int i,j=0;
	int cnt = 0;
	
	for(i=0; i<len; i++){
		if(buf[i]==' '){
			str[cnt][j] = 0;
			j = 0;
			cnt++;
		}else{
			str[cnt][j] = buf[i];
			j++;
		}
	}
	
	wl.freq_params.freq = atoi(str[0]);
	wl.freq_params.sec_channel_offset = atoi(str[1]);
	wl.freq_params.ht_enabled = atoi(str[2]);
	wl.freq_params.channel = atoi(str[3]);
	wl.freq_params.mode = atoi(str[4]);
	
	
	int ret = hostapd_set_freq(h, wl.freq_params.mode, wl.freq_params.freq, wl.freq_params.channel, 
						wl.freq_params.ht_enabled, wl.freq_params.sec_channel_offset);
	u8 ret_str[10];
	sprintf(ret_str, "%d", ret);
	send_response(fd, SET_FREQ_R, ret_str, strlen(ret_str)); 
}

void GET_FREQ_handle(int fd, u8 *buf, int len, void *hapd){
	
	int ret_str[100];

	sprintf(ret_str, "%d %d %d %d %d", wl.freq_params.freq,wl.freq_params.sec_channel_offset,wl.freq_params.ht_enabled,
									   wl.freq_params.channel,wl.freq_params.mode);
									   
	send_response(fd, GET_FREQ_R, ret_str, strlen(ret_str)); 
	
}


void add_in_SET_TXQ_handle(u8 *buf, int len){
	int i;
	for(i=0; i<8; i++){
		if( buf_txq[i].free == 0){
			
			buf_txq[i].buf = malloc( sizeof(char) * len);
			memcpy( buf_txq[i].buf, buf, len);
			buf_txq[i].len = len;
			buf_txq[i].free = 1;
			
		}
	}
}

void flush_SET_TXQ_handle(int fd, void *hapd){
	
	int i;
	for(i=0; i<8; i++){
		if( buf_txq[i].free == 1){
			buf_txq[i].free = 0;
			SET_TXQ_handle(fd, buf_txq[i].buf, buf_txq[i].len, hapd);
			
		}
	}
}

void SET_TXQ_handle(int fd, u8 *buf, int len, void *hapd){
	 
	struct hostapd_data *h = hapd;
	char str[5][10];
	
	buf[len]=0;

	
	int i,j=0;
	int cnt = 0;
	
	for(i=0; i<len; i++){
		if(buf[i]==' '){
			str[cnt][j] = 0;
			j = 0;
			cnt++;
		}else{
			str[cnt][j] = buf[i];
			j++;
		}
	}
	
	wl.que[atoi(str[0])].queue_id = atoi(str[0]);
	wl.que[atoi(str[0])].cwmin = atoi(str[1]);
	wl.que[atoi(str[0])].cwmax = atoi(str[2]);
	wl.que[atoi(str[0])].aifs = atoi(str[3]);
	wl.que[atoi(str[0])].burst_time = atoi(str[4]);
	
	int ret = hostapd_set_tx_queue_params(h, wl.que[atoi(str[0])].queue_id, 
											 wl.que[atoi(str[0])].aifs,
											 wl.que[atoi(str[0])].cwmin, 
											 wl.que[atoi(str[0])].cwmax,
											 wl.que[atoi(str[0])].burst_time);

	
	
	u8 ret_str[10];
	sprintf(ret_str, "%d", ret);
	send_response(fd, SET_TXQ_R, ret_str, strlen(ret_str)); 
}

void SET_ADDR_handle(int fd, u8 *buf, int len, void *hapd){
	wpa_printf(MSG_DEBUG, "ADD ADDR: %02X %02X %02X %02X %02X %02X",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]); 
	struct hostapd_data *h = hapd;
	static  unsigned char sup[4]={ 0x82 ,0x84 ,0x8B ,0x96 };
	
	hostapd_sta_add(h, buf, ( unsigned short)1, ( unsigned short)33,  sup, 4,
			    (u16)1,
			    NULL,
			    (u32)32931);
}

void DEL_ADDR_handle(int fd, u8 *buf, int len, void *hapd){
	wpa_printf(MSG_DEBUG, "DEL ADDR: %02X %02X %02X %02X %02X %02X",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]); 
	struct hostapd_data *h = hapd;
	static  unsigned char sup[4]={ 0x82 ,0x84 ,0x8B ,0x96 };
	hostapd_drv_sta_remove(h, buf);
	
}

void DEL_WLAN_handle(int fd, u8 *buf, int len, void *hapd){
	
	struct hostapd_data *h = hapd;
	wpa_printf(MSG_DEBUG, "DEL WLAN");
	
	if( h->driver->hapd_deinit ){
		
		h->driver->hapd_deinit(h->iface->bss[0]->drv_priv);
		
	}
	hostapd_interface_free(h->iface);
	end_ipc(fd);
	
	exit(0);

}

void ADD_WLAN_handle(int fd, u8 *buf, int len, void *hapd, unsigned char *ssid_p, int *ssid_len_p){
	wpa_printf(MSG_DEBUG, "ADD WLAN: len:%d",len);
	int i,k;
	
	struct hostapd_data *h = hapd;
	
	wpa_printf(MSG_DEBUG, "Radio ID: %d",buf[0]);
	wpa_printf(MSG_DEBUG, "Wlan ID: %d",buf[1]);
	wpa_printf(MSG_DEBUG, "Flag 1: %02X",buf[2]);
	wpa_printf(MSG_DEBUG, "Flag 2: %02X",buf[3]);
	
	wpa_printf(MSG_DEBUG, "Key Index: %d",buf[4]);
	wpa_printf(MSG_DEBUG, "Key Status: %d",buf[5]);
	
	
	short key_len;
	*(&key_len + 1) = buf[6];
	*(&key_len + 0) = buf[7];
	
	//wpa_printf(MSG_DEBUG, "Keylen: %d",key_len);
	
	unsigned char key[key_len];
	
	if(key_len){
		memcpy(key, buf+8, key_len);
		//wpa_printf(MSG_DEBUG, "Key: %s",key);
		
		h->conf->ssid.wep.idx = 0;
		h->conf->ssid.wep.len[0] = key_len;
		h->conf->ssid.wep.keys_set = 1;
		h->conf->ssid.security_policy=SECURITY_STATIC_WEP;
		
		char tmp_key[128];
		
		
		for(k=0; k<key_len; k++)tmp_key[k]=*(buf+8+k);
		h->conf->ssid.wep.key[0] = malloc(128);
		
		
	    memcpy(h->conf->ssid.wep.key[0], tmp_key, key_len);
		
	}
	
	int key_len_int = (int) key_len;
	
	//wpa_printf(MSG_DEBUG, "key len %d",key_len_int);
		
	int pos_mac_mod = 16 + key_len_int;
	
	wpa_printf(MSG_DEBUG, "Mac Mode: %d",buf[pos_mac_mod]);
	wpa_printf(MSG_DEBUG, "Tunnel Mode:  %02X",buf[17+key_len_int]);
	wpa_printf(MSG_DEBUG, "Suppress SSID:  %02X",buf[18+key_len_int]);
	
	if( buf[18+key_len_int]!=0x00 ){
		int ssid_len = len - 19 - key_len_int;
		char tmp_ssid[33];
		
		wpa_printf(MSG_DEBUG, "SSID LEN: %d",ssid_len);
		
		for(k=0;k<ssid_len;k++)	tmp_ssid[k]=*(buf + 19 + key_len_int +k);
		
		*ssid_len_p = ssid_len;
		memcpy(ssid_p, tmp_ssid, ssid_len);
		
		h->conf->ssid.ssid_len = ssid_len;
		memcpy(h->conf->ssid.ssid, tmp_ssid, ssid_len);
		
	}

}


void management_recv(int fd, u8 code, u8 *buf, int len, void *hapd,void *inject_func){

	if(code == PING){
		send_response(fd, PONG, buf, len);
		
	}else if( code==SET_RTS ){
		SET_RTS_handle(fd, buf, len, hapd);
		
	}else if( code==SET_FRAG ){
		SET_FRAG_handle(fd, buf, len, hapd);
		
	}else if( code==GET_RTS ){
		GET_RTS_handle(fd, buf, len, hapd);
		
	}else if( code==GET_FRAG ){
		GET_FRAG_handle(fd, buf, len, hapd);
		
	}else if( code==SET_FREQ ){
		SET_FREQ_handle(fd, buf, len, hapd);
		
	}else if( code==GET_FREQ ){
		GET_FREQ_handle(fd, buf, len, hapd);
		
	}else if( code==SET_TXQ ){
		SET_TXQ_handle(fd, buf, len, hapd);
		
	}else if( code==DATE_TO_WTP ){
		struct hostapd_data *h = hapd;
		void (*pointer_WTP_inject_frame_in_air)(void*,unsigned char*,int);
	    pointer_WTP_inject_frame_in_air = inject_func;
	    pointer_WTP_inject_frame_in_air(h->drv_priv, buf, len);
		
	}else if( code==SET_ADDR ){
		SET_ADDR_handle(fd, buf, len, hapd);

	}else if( code==DEL_ADDR ){
		DEL_ADDR_handle(fd, buf, len, hapd);
	
	}else if( code==DEL_WLAN ){
		DEL_WLAN_handle(fd, buf, len, hapd);
		
		
	}else{
		wpa_printf(MSG_ERROR, "ERROR IPC: received unrecognizedcode: %d",code);
	}

}

int recv_request(int fd,void *hapd, void *inject_func){
	char str[MAX_BUF];
 	
	int n;
	
	#if defined(LOCALUDP)
		n = recvfrom(fd, str, MAX_BUF, 0,(struct sockaddr *)&local, &address_size);
	#else
		n = recvfrom(fd, str, MAX_BUF, 0,(struct sockaddr *)&addr, &address_size);
	#endif
	
	
    if(n<=0){
		end_ipc(fd);
		return -1;
	}
	management_recv(fd, str[0], str + 1, n -1, hapd, inject_func);	
}



int open_socket(){
	
	wpa_printf(MSG_DEBUG, "Ip:Port  %s:%d  %s",con_wtp.ip_wtp,con_wtp.wtp_port,con_wtp.path_unix_socket);

	int fd_wtp, n, con_res;
	
	char buffer[100];

	#if defined(LOCALUDP)
		srand((unsigned)time(0));
		fd_wtp = socket(AF_UNIX, SOCK_DGRAM, 0);
	
	#elif defined(NETUDP)
		#if defined(USEIPV6)
			fd_wtp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		#else
			fd_wtp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		#endif

	#else
		#if defined(USEIPV6)
			fd_wtp = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
		#else
			fd_wtp = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		#endif
	
	#endif
 
	#if defined(LOCALUDP)
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, con_wtp.path_unix_socket);

		local.sun_family = AF_UNIX;
	
		while(1){
			rn = rand()%100000;
			sprintf(local.sun_path, "%s%05d", con_wtp.path_unix_socket, rn);

			if (bind(fd_wtp, (struct sockaddr *)&local, strlen(local.sun_path) + sizeof(local.sun_family)) == -1) {
				sleep(1);
				continue;
			}
			break;
		}
		wpa_printf(MSG_DEBUG, "Try connecting to %s from %s",addr.sun_path, local.sun_path);
		
	#else
		#if defined(USEIPV6)
			addr.sin6_family = AF_INET6;
			addr.sin6_port = con_wtp.wtp_port;
			inet_pton(AF_INET6, con_wtp.ip_wtp, &addr.sin6_addr);
						
		#else
			addr.sin_family = AF_INET;
			addr.sin_port = con_wtp.wtp_port;
			addr.sin_addr.s_addr = inet_addr(con_wtp.ip_wtp);
						
		#endif
		wpa_printf(MSG_DEBUG, "Try connecting to %s:%d",con_wtp.ip_wtp,con_wtp.wtp_port);
		
	#endif
	
	address_size =  sizeof(addr);

	
	wpa_printf(MSG_DEBUG, "<NON CONNECT>");
	
	while(1){
		#if defined(LOCALUDP)
			sprintf(buffer,"Z%05dconnect",rn);
		#else
			sprintf(buffer,"Zconnect");
		#endif
		
		buffer[0] = CONNECT;
		n = sendto(fd_wtp, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, address_size);
		
		n = recvfrom(fd_wtp, buffer, sizeof(buffer),0,(struct sockaddr *)&local, &address_size);
		
		if(buffer[0] == CONNECT_R){
			break;
		}
		sleep(1);

	}
	wpa_printf(MSG_DEBUG, "<CONNECTED>");

    return fd_wtp;
    
}

int end_ipc(int fd){
	wpa_printf(MSG_DEBUG, "Close IPC");
	ipc_send_CLOSE_to_WTP(fd);
	
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

void goto_preconnect(int fd, void *hapd){
	char buf[MAX_BUF];
	int n;
	
	while(1){// Wait Packet
	 
		#if defined(LOCALUDP)
			n = recvfrom(fd, buf, MAX_BUF, 0,(struct sockaddr *)&local, &address_size);
		#else
			n = recvfrom(fd, buf, MAX_BUF, 0,(struct sockaddr *)&addr, &address_size);
		#endif
		
		
		if(n<=0){
			end_ipc(fd);
			return -1;
		}
	
		if( buf[0]==WTPRINFO ){
			send_response(fd, WTPRINFO_R, wlan0_capa+8 , 1);
			
		}else if( buf[0]==GET_RATES ){
			send_response(fd, GET_RATES_R, wlan0_capa , 8);
		
		}else if( buf[0]==GET_MDC ){
			send_response(fd, GET_MDC_R, wlan0_capa + 9 , 6);
		
		}else if( buf[0]==GET_MAC ){
			send_response(fd, GET_MAC_R, wlan0_capa + 15 , 6);
			
		}else if( buf[0]==GOWAITWLAN ){
			send_response(fd, GOWAITWLAN_R, NULL, 0);
			break;
		}else{
			wpa_printf(MSG_DEBUG, "Unknow Command in CONNECTED State %d",buf[0]);
		}
	}
	wpa_printf(MSG_DEBUG, "<WAIT WLAN>");
}
	

void wait_ADD_WLAN(int fd, unsigned char *ssid_p, int *ssid_len_p, void *hapd ){
	char buf[MAX_BUF];
 	
	int n;
	
	while(1){// Wait Packet until recv ADD WLAN
	 		
		#if defined(LOCALUDP)
			n = recvfrom(fd, buf, MAX_BUF, 0,(struct sockaddr *)&local, &address_size);
		#else
			n = recvfrom(fd, buf, MAX_BUF, 0,(struct sockaddr *)&addr, &address_size);
		#endif
		
		
		if(n<=0){
			end_ipc(fd);
			return -1;
		}
	
		if( buf[0]==ADD_WLAN ){

			ADD_WLAN_handle(fd, buf+1, n-1, hapd, ssid_p, ssid_len_p);
			break;
			
		}else if ( buf[0]==DEL_WLAN_handle ){
			DEL_WLAN_handle(fd, NULL, 0, hapd);
		
		}else if( buf[0]==SET_TXQ ){
			add_in_SET_TXQ_handle( buf+1, n-1 );
		
		}else if( buf[0]==DEL_WLAN){
			DEL_WLAN_handle(fd,NULL,0,hapd);
		
		}else{
			wpa_printf(MSG_DEBUG, "Unknow Command in WAITWLAN State %d",buf[0]);
		}
	}
	wpa_printf(MSG_DEBUG, "<LIVE>");
}

int start_ipc(void *hapd,unsigned char *ssid_p, int *ssid_len_p,void *inject_func, char *cap_info){
	
	memcpy( wlan0_capa, cap_info, 21);
	
	ReadConfiguration(&con_wtp);
	
	int sockfd = open_socket();
	
	goto_preconnect(sockfd, hapd);
	wait_ADD_WLAN(sockfd, ssid_p, ssid_len_p, hapd); 
	
	wl.frag = -1;
	wl.rts = -1;
	wl.freq_params.channel = 0;
	wl.freq_params.ht_enabled = 0;
	wl.freq_params.mode = 0;
	wl.freq_params.sec_channel_offset = 0;
	wl.freq_params.freq = 2452;
	
	wl.que[0].queue_id = 0;
	wl.que[0].cwmin = 3;
	wl.que[0].cwmax = 7;
	wl.que[0].aifs = 2;
	
	wl.que[1].queue_id = 1;
	wl.que[1].cwmin = 7;
	wl.que[1].cwmax = 15;
	wl.que[1].aifs = 2;
	
	wl.que[2].queue_id = 2;
	wl.que[2].cwmin = 15;
	wl.que[2].cwmax = 1023;
	wl.que[2].aifs = 3;
	
	wl.que[3].queue_id = 3;
	wl.que[3].cwmin = 31;
	wl.que[3].cwmax = 1023;
	wl.que[3].aifs = 7;
	
	//void (*pointer_WTP_inject_frame_in_air)(void*,unsigned char*,int);
	//pointer_WTP_inject_frame_in_air = inject_func;
		
	if(sockfd){
		if (eloop_register_read_sock(sockfd, recv_request, hapd, inject_func)) {
			wpa_printf(MSG_ERROR, "Clould not register IPC socket start_ipc");
			return 0;
		}
	}
	wpa_printf(MSG_DEBUG, "SOCKET Created %d",sockfd);
	return sockfd;
}
