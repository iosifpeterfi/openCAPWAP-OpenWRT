#include "drivers/driver.h"

void WTP_handle_assoc_cb(struct hostapd_data *hapd,const struct ieee80211_mgmt *mgmt,size_t len, int reassoc, int ok);

void WTP_ieee802_11_mgmt_cb(struct hostapd_data *hapd, const u8 *buf, size_t len,u16 stype, int ok);

struct hostapd_data * WTP_get_hapd_bssid(struct hostapd_iface *iface,const u8 *bssid);

void WTP_hostapd_mgmt_tx_cb(struct hostapd_data *hapd, const u8 *buf,size_t len, u16 stype, int ok);

void WTP_wpa_supplicant_event(void *ctx, enum wpa_event_type event,union wpa_event_data *data);


int WTP_get_SubType(u8 *buf,int len);

int WTP_get_Type(u8 *buf,int len);

