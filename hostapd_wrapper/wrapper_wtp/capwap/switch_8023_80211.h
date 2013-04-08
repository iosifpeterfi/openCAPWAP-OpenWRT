
int add_8022_header( unsigned char *inbuf, int inlen,  unsigned char *outbuf);

int add_8023_header( unsigned char *mac_src, unsigned char *mac_dst, unsigned char *inbuf, int inlen,  unsigned char *outbuf);

int add_80211_Data_header( unsigned char *da,  unsigned char *sa,  unsigned char *bssid_add, int toDS, int FromDS,  unsigned char *inbuf, int inlen,  unsigned char *outbuf);

int from_8023_to_80211( unsigned char *inbuffer,int inlen, unsigned char *outbuffer, unsigned char *own_addr);

int from_80211_to_8023( unsigned char *inbuffer,int inlen, unsigned char *outbuffer);
