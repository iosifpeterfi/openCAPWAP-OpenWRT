void ipc_send_80211_to_wtp(int fd, u8 *buf, int len);

int start_ipc(void *hapd,void *inject_func);

int end_ipc(int fd);
