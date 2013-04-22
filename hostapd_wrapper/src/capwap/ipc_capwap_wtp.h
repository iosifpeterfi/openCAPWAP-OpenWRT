#ifndef __IPC_CAPWAP_WTP_H
#define __IPC_CAPWAP_WTP_H

typedef void (* WTP_frame_inject)(void *, unsigned char *, int);

void ipc_send_80211_to_ac(int fd, u8 *buf, int len);
void flush_SET_TXQ_handle(int fd, void *hapd);
int start_ipc(void *hapd, unsigned char *, int *, void * inject_func, unsigned char *);
int end_ipc(int fd);

#endif
