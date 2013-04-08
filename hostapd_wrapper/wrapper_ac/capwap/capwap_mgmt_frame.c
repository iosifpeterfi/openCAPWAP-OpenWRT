#include "utils/includes.h"

#include "utils/common.h"
#include "radius/radius.h"
#include "drivers/driver.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/wpa_ctrl.h"
#include "crypto/random.h"


#define MAC_LENGTH 6
#define HLEN_80211 24
#define HLEN_LLC 8

int same_mac( unsigned char *mac1, unsigned char *mac2){// return 0
	int i;
	for(i=0;i<MAC_LENGTH;i++)if(mac1[i]!=mac2[i])return -1;
	return 0;
}

int GetEapol_Frame( unsigned char *sa,  unsigned char *buf, int len){
	struct ieee80211_hdr *hdr;
	hdr = (struct ieee80211_hdr *) buf;
	os_memcpy(sa, hdr->addr2, MAC_LENGTH);
	return HLEN_80211 + HLEN_LLC;
}

int isEAPOL_Frame( unsigned char *buf, int len){
	unsigned char rfc1042_header[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	int i;
	
	for(i=0; i<6; i++)if(rfc1042_header[i]!=buf[i + HLEN_80211])return 0;
	return 1;
}

void stampa_mac(char *s,  unsigned char *mac){
	int i;
	wpa_printf(MSG_DEBUG,"%s",s);
	for(i=0; i<6; i++)wpa_printf(MSG_DEBUG,"%02X ",mac[i]);
	wpa_printf(MSG_DEBUG,"\n");
}

void stamp_all_max( unsigned char *buf,  unsigned char *own_mac){
	struct ieee80211_hdr *hdr;
	
	hdr = (struct ieee80211_hdr *) buf;

	stampa_mac("addr1 ",hdr->addr1);
	stampa_mac("addr2 ",hdr->addr2);
	stampa_mac("addr3 ",hdr->addr3);
	stampa_mac("own_mac ", own_mac);
}

int isCallBackFrame( unsigned char *buf, int len,  unsigned char *own_mac){
	// return 1 if is it CALL Back Frame
	// return 0 if is NOT CALL Back Frame
	struct ieee80211_hdr *hdr;
	u16 fc;
	
	hdr = (struct ieee80211_hdr *) buf;
	fc = le_to_host16(hdr->frame_control);

	if( same_mac(hdr->addr2,own_mac)==0 && same_mac(hdr->addr3,own_mac)==0 )return 1;
	return 0;
}

int AC_get_SubType( unsigned char *buf, int len){
	struct ieee80211_hdr *hdr;
	u16 fc;
	
	hdr = (struct ieee80211_hdr *) buf;
	fc = le_to_host16(hdr->frame_control);
	
	return WLAN_FC_GET_STYPE(fc);
}

int AC_get_Type( unsigned char *buf, int len){
	struct ieee80211_hdr *hdr;
	u16 fc;
	
	hdr = (struct ieee80211_hdr *) buf;
	fc = le_to_host16(hdr->frame_control);
	
	return WLAN_FC_GET_TYPE(fc);
}

