#include "utils/includes.h"

#include "utils/common.h"
#include "radius/radius.h"
#include "drivers/driver.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"


#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))

#define TYPE_LEN 2
#define ETH_ALEN 6
#define ETH_HLEN 14

int add_8022_header( unsigned char *inbuf, int inlen,  unsigned char *outbuf){

	int indx = 0;
	 unsigned char DSAP = 0xAA;
	 unsigned char SSAP = 0xAA;
	 unsigned char Control = 0x03;
	 unsigned char Organ_Code[3] = { 0x00, 0x00, 0x00 }; //Encapsulated Ethernet
	 unsigned char Type[2] = { 0x88, 0x8E };

	os_memcpy(outbuf + indx, &DSAP, 1);
	indx +=1;

	os_memcpy(outbuf + indx, &SSAP, 1);
	indx +=1;

	os_memcpy(outbuf + indx, &Control, 1);
	indx +=1;

	os_memcpy(outbuf + indx, Organ_Code, sizeof(Organ_Code));
	indx +=sizeof(Organ_Code);

	os_memcpy(outbuf + indx, Type, sizeof(Type));
	indx +=sizeof(Type);

	os_memcpy(outbuf + indx, inbuf, inlen);
	indx += inlen;

	return indx;
}

int add_80211_Data_header( unsigned char *da,  unsigned char *sa,  unsigned char *bssid_add, int toDS, int FromDS,  unsigned char *inbuf, int inlen,  unsigned char *outbuf){
	int indx = 0;

	int encaps_len, skip_header_bytes;
	struct ieee80211_hdr hdr;
	u16 fc,ethertype;
	os_memset(&hdr,0,sizeof(struct ieee80211_hdr));

	hdr.frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
	
	hdr.duration_id = 0;
	hdr.seq_ctrl = 0;
	
	if (toDS == 0 && FromDS == 0) {
		os_memcpy(hdr.addr1, da, ETH_ALEN);
		os_memcpy(hdr.addr2, sa, ETH_ALEN);
		os_memcpy(hdr.addr3, bssid_add, ETH_ALEN);
		CLEARBIT(hdr.frame_control,8);
		CLEARBIT(hdr.frame_control,9);
		
	}else if (toDS == 1 && FromDS == 0) {
		os_memcpy(hdr.addr1, bssid_add, ETH_ALEN);
		os_memcpy(hdr.addr2, sa, ETH_ALEN);
		os_memcpy(hdr.addr3, da, ETH_ALEN);
		SETBIT(hdr.frame_control,8);
		CLEARBIT(hdr.frame_control,9);
		
	}else if (toDS == 0 && FromDS == 1) {
		os_memcpy(hdr.addr1, da, ETH_ALEN);
		os_memcpy(hdr.addr2, bssid_add, ETH_ALEN);
		os_memcpy(hdr.addr3, sa, ETH_ALEN);
		CLEARBIT(hdr.frame_control,8);
		SETBIT(hdr.frame_control,9);
		
	}else{
		wpa_printf(MSG_ERROR, "ERROR add_80211_Data_header\n");
	}

	os_memcpy(outbuf + indx, &hdr, sizeof(hdr));
	indx += sizeof(hdr);
	
	os_memcpy(outbuf + indx, inbuf, inlen);
	indx += inlen;
 
	return indx;
}

int add_8023_header( unsigned char *mac_src, unsigned char *mac_dst, unsigned char *inbuf, int inlen,  unsigned char *outbuf){
	int indx = 0;
	 unsigned char Type[2] = { 0x08, 0x00 };
	
	os_memcpy(outbuf + indx, mac_dst, ETH_ALEN);
	indx += ETH_ALEN;
	
	os_memcpy(outbuf + indx, mac_src, ETH_ALEN);
	indx += ETH_ALEN;
	
	os_memcpy(outbuf + indx, Type,  sizeof(Type));
	indx +=  sizeof(Type);
	
	os_memcpy(outbuf + indx, inbuf, inlen);
	indx += inlen;
	
	return indx;
}

int from_8023_to_80211( unsigned char *inbuffer,int inlen, unsigned char *outbuffer, unsigned char *own_addr){

	int encaps_len, skip_header_bytes;
	int indx=0;
	struct ieee80211_hdr hdr;
	u16 fc,ethertype;
	os_memset(&hdr,0,sizeof(struct ieee80211_hdr));

	hdr.frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
	hdr.duration_id = 0;
	hdr.seq_ctrl = 0;

	os_memcpy(hdr.addr1, inbuffer, ETH_ALEN);
	os_memcpy(hdr.addr2, own_addr, ETH_ALEN);
	os_memcpy(hdr.addr3, inbuffer + ETH_ALEN, ETH_ALEN);
	CLEARBIT(hdr.frame_control,8);
	SETBIT(hdr.frame_control,9);	
	
	os_memcpy(outbuffer + indx,&hdr,sizeof(hdr));
	indx += sizeof(hdr);
	os_memcpy(outbuffer + indx, inbuffer, inlen);
	indx += inlen;
	
	return indx;
}

int from_80211_to_8023( unsigned char *inbuffer,int inlen, unsigned char *outbuffer){
	
	int hlen_80211 = 24;
	memcpy(outbuffer, inbuffer + hlen_80211, inlen - hlen_80211);
	return inlen - hlen_80211;
	
}
