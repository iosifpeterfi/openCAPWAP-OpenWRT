#include <linux/types.h>

unsigned char ERROR = 0;

unsigned char CLOSE = 1;

unsigned char PING = 2; //  [2]ping
unsigned char PONG = 3;//  [3]pong

unsigned char START_WLAN = 4;
unsigned char START_WLAN_R = 5;

unsigned char STOP_WLAN = 6;
unsigned char STOP_WLAN_R = 7;

unsigned char SET_FREQ = 8; // FREQ sec_channel_offset ht_enabled channel MODE  ex. "[8]2462 0 0 11 0"
unsigned char SET_FREQ_R = 9; // 0 or 1  ex. "[9]0"

unsigned char GET_FREQ = 10; // ex. "[10]"
unsigned char GET_FREQ_R = 11; // ex. "[11]2462 0 0 11 0"

unsigned char SET_FRAG = 12; // Typically the range used for fragmentation threshold is 256-2346 (-1 == off) ex. "[12]2000"
unsigned char SET_FRAG_R = 13;// ex. "[13]0"

unsigned char GET_FRAG = 14; // ex. "[14]"   (-1 == off)
unsigned char GET_FRAG_R = 15; // ex. "[15]2000" 

unsigned char SET_BITRATE = 16; 
unsigned char SET_BITRATE_R = 17;

unsigned char GET_BITRATE = 18; 
unsigned char GET_BITRATE_R = 19; 

unsigned char SET_RTS = 20; // 0-2347 (-1 == off)  ex. "[20]100"  (-1 == off)
unsigned char SET_RTS_R = 21;// ex. "[21]0"

unsigned char GET_RTS = 22; // ex. "]22]"  (-1 == off)
unsigned char GET_RTS_R = 23; // ex. "[23]100"

unsigned char SET_TXPOWER = 24; 
unsigned char SET_TXPOWER_R = 25;

unsigned char GET_TXPOWER = 26; 
unsigned char GET_TXPOWER_R = 27;

/*
 * VO - 0  CWMIN:3  CWMAX:7  AIFS:2
 * VI - 1  CWMIN:7  CWMAX:15  AIFS:2
 * BE - 2  CWMIN:15  CWMAX:1023  AIFS:3
 * BK - 3  CWMIN:15  CWMAX:1023  AIFS:7
 */
unsigned char SET_TXQ = 28; 
unsigned char SET_TXQ_R = 29; 

unsigned char GET_TXQ = 30; 
unsigned char GET_TXQ_R = 31;

unsigned char SET_ADDR = 32;
unsigned char SET_ADDR_R = 33;

unsigned char DEL_ADDR = 34;
unsigned char DEL_ADDR_R = 35;

unsigned char ADD_WLAN = 36;
unsigned char ADD_WLAN_R = 37;

unsigned char DEL_WLAN = 38;
unsigned char DEL_WLAN_R = 39;

unsigned char WTPRINFO = 40;
unsigned char WTPRINFO_R = 41;

unsigned char GET_RATES = 42;
unsigned char GET_RATES_R = 43;

unsigned char GET_MDC = 44;
unsigned char GET_MDC_R = 45;

unsigned char SET_WTPRINFO = 46;
unsigned char SET_WTPRINFO_R = 47;

unsigned char SET_RATES = 48;
unsigned char SET_RATES_R = 49;

unsigned char SET_MDC = 50;
unsigned char SET_MDC_R = 51;

unsigned char GOLIVE = 52;
unsigned char GOLIVE_R = 53;

unsigned char WANT_GOLIVE = 54;
unsigned char HAVE_TO_WAIT = 55;

unsigned char GET_MAC = 56;
unsigned char GET_MAC_R = 57;

unsigned char SET_MAC = 58;
unsigned char SET_MAC_R = 59;

unsigned char DATE_TO_WTP = 100;
unsigned char DATE_TO_AC = 	101;

unsigned char CONNECT = 102;
unsigned char CONNECT_R = 103;

unsigned char GOWAITWLAN = 104;
unsigned char GOWAITWLAN_R = 105;
