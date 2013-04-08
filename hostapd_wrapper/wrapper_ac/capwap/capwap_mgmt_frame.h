#include "drivers/driver.h"

int GetEapol_Frame( unsigned char *sa,  unsigned char *buf, int len);

int isEAPOL_Frame( unsigned char *buf, int len);

int isCallBackFrame( unsigned char *buf, int len,  unsigned char *own_mac);

int AC_get_SubType( unsigned char *buf, int len);

int AC_get_Type( unsigned char *buf, int len);
