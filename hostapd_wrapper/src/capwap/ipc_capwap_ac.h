#ifndef __IPC_CAPWAP_AC_H
#define __IPC_CAPWAP_AC_H

typedef void (* WTP_frame_inject)(void *, unsigned char *, int);

void ipc_send_80211_to_wtp(int fd, u8 *buf, int len);
int start_ipc(void *hapd, void *inject_func);
int end_ipc(int fd);

int capability_is_B();
int capability_get_num_modes();
int capability_get_num_channels();
int capability_get_rates(int *rate_arr);
void capability_get_mac(unsigned char *buf);
void prep_beacon(int fd,struct hostapd_data *hapd,struct wpa_driver_ap_params *params);

void ipc_send_DEL_WLAN(int fd);
void ipc_send_add_station(int fd, u8 *buf, int len);
void ipc_send_del_station(int fd, u8 *buf, int len);

void send_response(int fd, u8 code, u8 *buf, int len);

#endif
