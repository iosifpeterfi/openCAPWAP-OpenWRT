void ipc_send_80211_to_ac(int fd, u8 *buf, int len);

int start_ipc(void *hapd,unsigned char*,int*,void *inject_func,char*);

int end_ipc(int fd);
