#define __CAPWAP_WTPBroadcomDriverInteraction_HEADER__


/* check this magic number */
#define WLC_IOCTL_MAGIC		0x14e46c77

/* bump this number if you change the ioctl interface */
#define WLC_IOCTL_VERSION	1

#define	WLC_IOCTL_MAXLEN	8192		/* max length ioctl buffer required */
#define	WLC_IOCTL_SMLEN		256		/* "small" length ioctl buffer required */

/* common ioctl definitions */
#define WLC_GET_MAGIC				0
#define WLC_GET_VERSION				1
#define WLC_UP					2
#define WLC_DOWN				3
#define WLC_DUMP				6
#define WLC_GET_MSGLEVEL			7
#define WLC_SET_MSGLEVEL			8
#define WLC_GET_PROMISC				9
#define WLC_SET_PROMISC				10
#define WLC_GET_RATE				12
/* #define WLC_SET_RATE				13 */ /* no longer supported */
#define WLC_GET_INSTANCE			14
/* #define WLC_GET_FRAG				15 */ /* no longer supported */
/* #define WLC_SET_FRAG				16 */ /* no longer supported */
/* #define WLC_GET_RTS				17 */ /* no longer supported */
/* #define WLC_SET_RTS				18 */ /* no longer supported */
#define WLC_GET_INFRA				19
#define WLC_SET_INFRA				20
#define WLC_GET_AUTH				21
#define WLC_SET_AUTH				22
#define WLC_GET_BSSID				23
#define WLC_SET_BSSID				24
#define WLC_GET_SSID				25
#define WLC_SET_SSID				26
#define WLC_RESTART				27
#define WLC_GET_CHANNEL				29
#define WLC_SET_CHANNEL				30
#define WLC_GET_SRL				31
#define WLC_SET_SRL				32
#define WLC_GET_LRL				33
#define WLC_SET_LRL				34
#define WLC_GET_PLCPHDR				35
#define WLC_SET_PLCPHDR				36
#define WLC_GET_RADIO				37
#define WLC_SET_RADIO				38
#define WLC_GET_PHYTYPE				39
/* #define WLC_GET_WEP				42 */ /* no longer supported */
/* #define WLC_SET_WEP				43 */ /* no longer supported */
#define WLC_GET_KEY				44
#define WLC_SET_KEY				45
#define WLC_GET_REGULATORY			46
#define WLC_SET_REGULATORY			47
#define WLC_GET_PASSIVE 			48
#define WLC_SET_PASSIVE 			49
#define WLC_SCAN				50
#define WLC_SCAN_RESULTS			51
#define WLC_DISASSOC				52
#define WLC_REASSOC				53
#define WLC_GET_ROAM_TRIGGER			54
#define WLC_SET_ROAM_TRIGGER			55
#define WLC_GET_TXANT				61
#define WLC_SET_TXANT				62
#define WLC_GET_ANTDIV				63
#define WLC_SET_ANTDIV				64
/* #define WLC_GET_TXPWR			65 */ /* no longer supported */
/* #define WLC_SET_TXPWR			66 */ /* no longer supported */
#define WLC_GET_CLOSED				67
#define WLC_SET_CLOSED				68
#define WLC_GET_MACLIST				69
#define WLC_SET_MACLIST				70
#define WLC_GET_RATESET				71
#define WLC_SET_RATESET				72
#define WLC_GET_LOCALE				73
#define WLC_LONGTRAIN				74
#define WLC_GET_BCNPRD				75
#define WLC_SET_BCNPRD				76
#define WLC_GET_DTIMPRD				77
#define WLC_SET_DTIMPRD				78
#define WLC_GET_SROM				79
#define WLC_SET_SROM				80
#define WLC_GET_WEP_RESTRICT			81
#define WLC_SET_WEP_RESTRICT			82
#define WLC_GET_COUNTRY				83
#define WLC_SET_COUNTRY				84
#define WLC_GET_PM				85
#define WLC_SET_PM				86
#define WLC_GET_WAKE				87
#define WLC_SET_WAKE				88
#define WLC_GET_D11CNTS				89
#define WLC_GET_FORCELINK			90	/* ndis only */
#define WLC_SET_FORCELINK			91	/* ndis only */
#define WLC_FREQ_ACCURACY			92
#define WLC_CARRIER_SUPPRESS			93
#define WLC_GET_PHYREG				94
#define WLC_SET_PHYREG				95
#define WLC_GET_RADIOREG			96
#define WLC_SET_RADIOREG			97
#define WLC_GET_REVINFO				98
#define WLC_GET_UCANTDIV			99
#define WLC_SET_UCANTDIV			100
#define WLC_R_REG				101
#define WLC_W_REG				102
#define WLC_DIAG_LOOPBACK			103
#define WLC_RESET_D11CNTS			104
#define WLC_GET_MACMODE				105
#define WLC_SET_MACMODE				106
#define WLC_GET_MONITOR				107
#define WLC_SET_MONITOR				108
#define WLC_GET_GMODE				109
#define WLC_SET_GMODE				110
#define WLC_GET_LEGACY_ERP			111
#define WLC_SET_LEGACY_ERP			112
#define WLC_GET_RX_ANT				113
#define WLC_GET_CURR_RATESET			114	/* current rateset */
#define WLC_GET_SCANSUPPRESS			115
#define WLC_SET_SCANSUPPRESS			116
#define WLC_GET_AP				117
#define WLC_SET_AP				118
#define WLC_GET_EAP_RESTRICT			119
#define WLC_SET_EAP_RESTRICT			120
#define WLC_SCB_AUTHORIZE			121
#define WLC_SCB_DEAUTHORIZE			122
#define WLC_GET_WDSLIST				123
#define WLC_SET_WDSLIST				124
#define WLC_GET_ATIM				125
#define WLC_SET_ATIM				126
#define WLC_GET_RSSI				127
#define WLC_GET_PHYANTDIV			128
#define WLC_SET_PHYANTDIV			129
#define WLC_AP_RX_ONLY				130
#define WLC_GET_TX_PATH_PWR			131
#define WLC_SET_TX_PATH_PWR			132
#define WLC_GET_WSEC				133
#define WLC_SET_WSEC				134
#define WLC_GET_PHY_NOISE			135
#define WLC_GET_BSS_INFO			136
#define WLC_GET_PKTCNTS				137
#define WLC_GET_LAZYWDS				138
#define WLC_SET_LAZYWDS				139
#define WLC_GET_BANDLIST			140
#define WLC_GET_BAND				141
#define WLC_SET_BAND				142
#define WLC_SCB_DEAUTHENTICATE			143
#define WLC_GET_SHORTSLOT			144
#define WLC_GET_SHORTSLOT_OVERRIDE		145
#define WLC_SET_SHORTSLOT_OVERRIDE		146
#define WLC_GET_SHORTSLOT_RESTRICT		147
#define WLC_SET_SHORTSLOT_RESTRICT		148
#define WLC_GET_GMODE_PROTECTION		149
#define WLC_GET_GMODE_PROTECTION_OVERRIDE	150
#define WLC_SET_GMODE_PROTECTION_OVERRIDE	151
#define WLC_UPGRADE				152
/* #define WLC_GET_MRATE			153 */ /* no longer supported */
/* #define WLC_SET_MRATE			154 */ /* no longer supported */
#define WLC_GET_ASSOCLIST			159
#define WLC_GET_CLK				160
#define WLC_SET_CLK				161
#define WLC_GET_UP				162
#define WLC_OUT					163
#define WLC_GET_WPA_AUTH			164
#define WLC_SET_WPA_AUTH			165
#define WLC_GET_PROTECTION_CONTROL		178
#define WLC_SET_PROTECTION_CONTROL		179
#define WLC_GET_PHYLIST				180
#define WLC_GET_KEY_SEQ				183
/* #define WLC_GET_GMODE_PROTECTION_CTS		198 */ /* no longer supported */
/* #define WLC_SET_GMODE_PROTECTION_CTS		199 */ /* no longer supported */
#define WLC_GET_PIOMODE				203
#define WLC_SET_PIOMODE				204
#define WLC_SET_LED				209
#define WLC_GET_LED				210
#define WLC_GET_CHANNEL_SEL			215
#define WLC_START_CHANNEL_SEL			216
#define WLC_GET_VALID_CHANNELS			217
#define WLC_GET_FAKEFRAG			218
#define WLC_SET_FAKEFRAG			219
#define WLC_GET_WET				230
#define WLC_SET_WET				231
#define WLC_GET_KEY_PRIMARY			235
#define WLC_SET_KEY_PRIMARY			236
#define WLC_GET_RADAR				242
#define WLC_SET_RADAR				243
#define WLC_SET_SPECT_MANAGMENT			244
#define WLC_GET_SPECT_MANAGMENT			245
#define WLC_WDS_GET_REMOTE_HWADDR		246	/* handled in wl_linux.c/wl_vx.c */
#define WLC_SET_CS_SCAN_TIMER			248
#define WLC_GET_CS_SCAN_TIMER			249
#define WLC_SEND_PWR_CONSTRAINT			254
#define WLC_CURRENT_PWR				256
#define WLC_GET_CHANNELS_IN_COUNTRY		260
#define WLC_GET_COUNTRY_LIST			261
#define WLC_GET_VAR				262	/* get value of named variable */
#define WLC_SET_VAR				263	/* set named variable to value */
#define WLC_NVRAM_GET				264	/* deprecated */
#define WLC_NVRAM_SET				265
#define WLC_SET_WSEC_PMK			268
#define WLC_GET_AUTH_MODE			269
#define WLC_SET_AUTH_MODE			270
#define WLC_NDCONFIG_ITEM			273	/* currently handled in wl_oid.c */
#define WLC_NVOTPW				274
#define WLC_OTPW				275
#define WLC_SET_LOCALE				278
#define WLC_LAST				279	/* do not change - use get_var/set_var */


/* ** driver/apps-shared section ** */

#define BCME_STRLEN 		64	/* Max string length for BCM errors */
#define VALID_BCMERROR(e)  ((e <= 0) && (e >= BCME_LAST))


/*
 * error codes could be added but the defined ones shouldn't be changed/deleted
 * these error codes are exposed to the user code
 * when ever a new error code is added to this list
 * please update errorstring table with the related error string and
 * update osl files with os specific errorcode map
*/

#define BCME_OK				0	/* Success */
#define BCME_ERROR			-1	/* Error generic */
#define BCME_BADARG			-2	/* Bad Argument */
#define BCME_BADOPTION			-3	/* Bad option */
#define BCME_NOTUP			-4	/* Not up */
#define BCME_NOTDOWN			-5	/* Not down */
#define BCME_NOTAP			-6	/* Not AP */
#define BCME_NOTSTA			-7	/* Not STA  */
#define BCME_BADKEYIDX			-8	/* BAD Key Index */
#define BCME_RADIOOFF 			-9	/* Radio Off */
#define BCME_NOTBANDLOCKED		-10	/* Not  band locked */
#define BCME_NOCLK			-11	/* No Clock */
#define BCME_BADRATESET			-12	/* BAD Rate valueset */
#define BCME_BADBAND			-13	/* BAD Band */
#define BCME_BUFTOOSHORT		-14	/* Buffer too short */
#define BCME_BUFTOOLONG			-15	/* Buffer too long */
#define BCME_BUSY			-16	/* Busy */
#define BCME_NOTASSOCIATED		-17	/* Not Associated */
#define BCME_BADSSIDLEN			-18	/* Bad SSID len */
#define BCME_OUTOFRANGECHAN		-19	/* Out of Range Channel */
#define BCME_BADCHAN			-20	/* Bad Channel */
#define BCME_BADADDR			-21	/* Bad Address */
#define BCME_NORESOURCE			-22	/* Not Enough Resources */
#define BCME_UNSUPPORTED		-23	/* Unsupported */
#define BCME_BADLEN			-24	/* Bad length */
#define BCME_NOTREADY			-25	/* Not Ready */
#define BCME_EPERM			-26	/* Not Permitted */
#define BCME_NOMEM			-27	/* No Memory */
#define BCME_ASSOCIATED			-28	/* Associated */
#define BCME_RANGE			-29	/* Not In Range */
#define BCME_NOTFOUND			-30	/* Not Found */
#define BCME_WME_NOT_ENABLED		-31	/* WME Not Enabled */
#define BCME_TSPEC_NOTFOUND		-32	/* TSPEC Not Found */
#define BCME_ACM_NOTSUPPORTED		-33	/* ACM Not Supported */
#define BCME_NOT_WME_ASSOCIATION	-34	/* Not WME Association */
#define BCME_SDIO_ERROR			-35	/* SDIO Bus Error */
#define BCME_DONGLE_DOWN		-36	/* Dongle Not Accessible */
#define BCME_LAST			BCME_DONGLE_DOWN

/* These are collection of BCME Error strings */
#define BCMERRSTRINGTABLE {		\
	"OK",				\
	"Undefined error",		\
	"Bad Argument",			\
	"Bad Option",			\
	"Not up",			\
	"Not down",			\
	"Not AP",			\
	"Not STA",			\
	"Bad Key Index",		\
	"Radio Off",			\
	"Not band locked",		\
	"No clock",			\
	"Bad Rate valueset",		\
	"Bad Band",			\
	"Buffer too short",		\
	"Buffer too long",		\
	"Busy",				\
	"Not Associated",		\
	"Bad SSID len",			\
	"Out of Range Channel",		\
	"Bad Channel",			\
	"Bad Address",			\
	"Not Enough Resources",		\
	"Unsupported",			\
	"Bad length",			\
	"Not Ready",			\
	"Not Permitted",		\
	"No Memory",			\
	"Associated",			\
	"Not In Range",			\
	"Not Found",			\
	"WME Not Enabled",		\
	"TSPEC Not Found",		\
	"ACM Not Supported",		\
	"Not WME Association",		\
	"SDIO Bus Error",		\
	"Dongle Not Accessible"		\
}


/* Types definition*/
/*
 * Inferred Typedefs
 *
 */

/* Infer the compile environment based on preprocessor symbols and pramas.
 * Override type definitions as needed, and include configuration dependent
 * header files to define types.
 */

#ifdef __cplusplus

#define TYPEDEF_BOOL
#ifndef FALSE
#define FALSE	false
#endif
#ifndef TRUE
#define TRUE	true
#endif

#endif	/* __cplusplus */

#if defined(_NEED_SIZE_T_)
typedef long unsigned int size_t;
#endif

#define TYPEDEF_UINT
#define TYPEDEF_USHORT
#define TYPEDEF_ULONG


/* Do not support the (u)int64 types with strict ansi for GNU C */
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
#define TYPEDEF_INT64
#define TYPEDEF_UINT64
#endif

/* pick up ushort & uint from standard types.h */
#if defined(linux) && defined(__KERNEL__)
#include <linux/types.h>	/* sys/types.h and linux/types.h are oil and water */
#else
#include <sys/types.h>
#endif

/* use the default typedefs in the next section of this file */
#define USE_TYPEDEF_DEFAULTS

/*
 * Default Typedefs
 *
 */

#ifdef USE_TYPEDEF_DEFAULTS
#undef USE_TYPEDEF_DEFAULTS

#ifndef TYPEDEF_BOOL
typedef	/* @abstract@ */ unsigned char	bool;
#endif

/* define uchar, ushort, uint, ulong */

#ifndef TYPEDEF_UCHAR
typedef unsigned char	uchar;
#endif

#ifndef TYPEDEF_USHORT
typedef unsigned short	ushort;
#endif

#ifndef TYPEDEF_UINT
typedef unsigned int	uint;
#endif

#ifndef TYPEDEF_ULONG
typedef unsigned long	ulong;
#endif

/* define [u]int8/16/32/64, uintptr */

#ifndef TYPEDEF_UINT8
typedef unsigned char	uint8;
#endif

#ifndef TYPEDEF_UINT16
typedef unsigned short	uint16;
#endif

#ifndef TYPEDEF_UINT32
typedef unsigned int	uint32;
#endif

#ifndef TYPEDEF_UINT64
typedef unsigned long long uint64;
#endif

#ifndef TYPEDEF_UINTPTR
typedef unsigned int	uintptr;
#endif

#ifndef TYPEDEF_INT8
typedef signed char	int8;
#endif

#ifndef TYPEDEF_INT16
typedef signed short	int16;
#endif

#ifndef TYPEDEF_INT32
typedef signed int	int32;
#endif

#ifndef TYPEDEF_INT64
typedef signed long long int64;
#endif

/* define float32/64, float_t */

#ifndef TYPEDEF_FLOAT32
typedef float		float32;
#endif

#ifndef TYPEDEF_FLOAT64
typedef double		float64;
#endif

/*
 * abstracted floating point type allows for compile time selection of
 * single or double precision arithmetic.  Compiling with -DFLOAT32
 * selects single precision; the default is double precision.
 */

#ifndef TYPEDEF_FLOAT_T

#if defined(FLOAT32)
typedef float32 float_t;
#else /* default to double precision floating point */
typedef float64 float_t;
#endif

#endif /* TYPEDEF_FLOAT_T */

/* define macro values */

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1  /* TRUE */
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef OFF
#define	OFF	0
#endif

#ifndef ON
#define	ON	1  /* ON = 1 */
#endif

#define	AUTO	(-1) /* Auto = -1 */

/* define PTRSZ, INLINE */

#ifndef PTRSZ
#define	PTRSZ	sizeof(char*)
#endif

#ifndef INLINE

#ifdef _MSC_VER

#define INLINE __inline

#elif __GNUC__

#define INLINE __inline__

#else

#define INLINE

#endif /* _MSC_VER */

#endif /* INLINE */

#undef TYPEDEF_BOOL
#undef TYPEDEF_UCHAR
#undef TYPEDEF_USHORT
#undef TYPEDEF_UINT
#undef TYPEDEF_ULONG
#undef TYPEDEF_UINT8
#undef TYPEDEF_UINT16
#undef TYPEDEF_UINT32
#undef TYPEDEF_UINT64
#undef TYPEDEF_UINTPTR
#undef TYPEDEF_INT8
#undef TYPEDEF_INT16
#undef TYPEDEF_INT32
#undef TYPEDEF_INT64
#undef TYPEDEF_FLOAT32
#undef TYPEDEF_FLOAT64
#undef TYPEDEF_FLOAT_T

/* WME Access Category Indices (ACIs) */
#define AC_BE			0	/* Best Effort */
#define AC_BK			1	/* Background */
#define AC_VI			2	/* Video */
#define AC_VO			3	/* Voice */
#define AC_COUNT		4	/* number of ACs */


struct edcf_acparam {
	uint8	ACI;
	uint8	ECW;
	uint16  TXOP;		/* stored in network order (ls octet first) */
} PACKED;
typedef struct edcf_acparam edcf_acparam_t;


/* Linux network driver ioctl encoding */
typedef struct wl_ioctl {
	uint cmd;	/* common ioctl definition */
	void *buf;	/* pointer to user buffer */
	uint len;	/* length of user buffer */
	bool set;	/* get or set request (optional) */
	uint used;	/* bytes read or written (optional) */
	uint needed;	/* bytes needed (optional) */
} wl_ioctl_t;




/*
 * Pass a wlioctl request to the specified interface.
 * @param	name	interface name
 * @param	cmd	WLC_GET_MAGIC <= cmd < WLC_LAST
 * @param	buf	buffer for passing in and/or receiving data
 * @param	len	length of buf
 * @return	>= 0 if successful or < 0 otherwise
 */
extern int wl_ioctl(char *name, int cmd, void *buf, int len);

extern int wl_iovar_set(char *ifname, char *iovar, void *param, int paramlen);
extern int wl_iovar_get(char *ifname, char *iovar, void *bufptr, int buflen);
/*
 * Set/Get named variable.
 * @param	ifname		interface name
 * @param	iovar		variable name
 * @param	param		input param value/buffer
 * @param	paramlen	input param value/buffer length
 * @param	bufptr		io buffer
 * @param	buflen		io buffer length
 * @param	val		val or val pointer for int routines
 * @return	success == 0, failure != 0
 */
/*
 * set named driver variable to int value
 * calling example: wl_iovar_setint(ifname, "arate", rate)
*/
static inline int
wl_iovar_setint(char *ifname, char *iovar, int val)
{
	return wl_iovar_set(ifname, iovar, &val, sizeof(val));
}

/*
 * get named driver variable to int value and return error indication
 * calling example: wl_iovar_getint(ifname, "arate", &rate)
 */
static inline int
wl_iovar_getint(char *ifname, char *iovar, int *val)
{
	return wl_iovar_get(ifname, iovar, val, sizeof(int));
}

#endif
