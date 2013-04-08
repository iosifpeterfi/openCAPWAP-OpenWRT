#include "utils/includes.h"

#include "utils/common.h"
#include "radius/radius.h"
#include "drivers/driver.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/wpa_ctrl.h"
#include "crypto/random.h"
#include "p2p/p2p.h"
#include "wps/wps.h"
#include "ap/hostapd.h"
#include "ap/ieee802_11.h"
#include "ap/sta_info.h"
#include "ap/accounting.h"
#include "ap/tkip_countermeasures.h"
#include "ap/iapp.h"
#include "ap/ieee802_1x.h"
#include "ap/wpa_auth.h"
#include "ap/wmm.h"
#include "ap/wps_hostapd.h"
#include "ap/ap_drv_ops.h"
#include "ap/ap_config.h"

#define HAPD_BROADCAST ((struct hostapd_data *) -1)
#define HLEN_80211	24

void WTP_handle_assoc_cb(struct hostapd_data *hapd,const struct ieee80211_mgmt *mgmt,size_t len, int reassoc, int ok){
	wpa_printf(MSG_DEBUG,"WTP: WTP_handle_assoc_cb\n");
	u16 status;
	struct sta_info *sta;
	int new_assoc = 1;
	struct ieee80211_ht_capabilities ht_cap;

	if (!ok) {
		hostapd_logger(hapd, mgmt->da, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "did not acknowledge association response");
		return;
	}

	if (len < IEEE80211_HDRLEN + (reassoc ? sizeof(mgmt->u.reassoc_resp) :
				      sizeof(mgmt->u.assoc_resp))) {
		printf("handle_assoc_cb(reassoc=%d) - too short payload "
		       "(len=%lu)\n", reassoc, (unsigned long) len);
		return;
	}

	if (reassoc)
		status = le_to_host16(mgmt->u.reassoc_resp.status_code);
	else
		status = le_to_host16(mgmt->u.assoc_resp.status_code);

	ap_sta_add(hapd, mgmt->da);
	sta = ap_get_sta(hapd, mgmt->da);
	
	static  unsigned char sup[4]={ 0x82 ,0x84 ,0x8B ,0x96 };
	
	hostapd_sta_add(hapd, sta->addr, ( unsigned short)1, ( unsigned short)33,  sup, 4,
			    (u16)1,
			    NULL,
			    (u32)32931);
	return ;
	
}

struct hostapd_data * WTP_get_hapd_bssid(struct hostapd_iface *iface,const  unsigned char *bssid){
	size_t i;
	if (bssid == NULL)
		return NULL;
	if (bssid[0] == 0xff && bssid[1] == 0xff && bssid[2] == 0xff &&
	    bssid[3] == 0xff && bssid[4] == 0xff && bssid[5] == 0xff)
		return HAPD_BROADCAST;

	for (i = 0; i < iface->num_bss; i++) {
		if (os_memcmp(bssid, iface->bss[i]->own_addr, ETH_ALEN) == 0)
			return iface->bss[i];
	}
	return NULL;
}

void WTP_handle_tx_callback_ASS_RES(void *ctx,  unsigned char *buf, size_t len, int ok){
	return;
	struct ieee80211_hdr *hdr;
	const struct ieee80211_mgmt *mgmt;

	u16 fc;
	union wpa_event_data event;
    struct hostapd_data *hapd=ctx;
    
	hdr = (struct ieee80211_hdr *) buf;
	mgmt = (const struct ieee80211_mgmt *) buf;
	fc = le_to_host16(hdr->frame_control);
	
	os_memset(&event, 0, sizeof(event));
	event.tx_status.type = WLAN_FC_GET_TYPE(fc);
	event.tx_status.stype = WLAN_FC_GET_STYPE(fc);
	event.tx_status.dst = hdr->addr1;
	event.tx_status.data = buf;
	event.tx_status.data_len = len;
	event.tx_status.ack = ok;
	 
	//hapd = WTP_get_hapd_bssid(hapd->iface, get_hdr_bssid(hdr, len));
	//if (hapd == NULL || hapd == HAPD_BROADCAST)
	//	return;
	
	if(event.tx_status.stype==WLAN_FC_STYPE_ASSOC_RESP)
		WTP_handle_assoc_cb(hapd, mgmt, len, 0, ok);
	else 
		WTP_handle_assoc_cb(hapd, mgmt, len, 1, ok);

}

int isEAPOL_Frame( unsigned char *buf, int len){
	unsigned char rfc1042_header[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	int i;
	
	for(i=0; i<6; i++)if(rfc1042_header[i]!=buf[i + HLEN_80211])return 0;
	return 1;
}

int WTP_get_SubType( unsigned char *buf,int len){
	struct ieee80211_hdr *hdr;
	u16 fc;
	
	hdr = (struct ieee80211_hdr *) buf;
	fc = le_to_host16(hdr->frame_control);
	
	return WLAN_FC_GET_STYPE(fc);
}

int WTP_get_Type( unsigned char*buf,int len){
	struct ieee80211_hdr *hdr;
	u16 fc;
	
	hdr = (struct ieee80211_hdr *) buf;
	fc = le_to_host16(hdr->frame_control);
	
	return WLAN_FC_GET_TYPE(fc);
}

