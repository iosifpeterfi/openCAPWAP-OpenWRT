/*
 * Documentation/networking/tuntap.txt
 *
 */

#include "CWAC.h"
#include "CWVendorPayloads.h"
#include "CWFreqPayloads.h"
#include "WUM.h"


#include <linux/if_tun.h>



#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     CWLog("Err to creater tap device");
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}


int init_AC_tap_interface(int WTPIndex)
{
      sprintf(gWTPs[WTPIndex].tap_name,"wtp%d", WTPIndex);
     
      gWTPs[WTPIndex].tap_fd = tun_alloc(gWTPs[WTPIndex].tap_name, IFF_TAP | IFF_NO_PI);

      CWDebugLog("gWTPs[%d].tap_name %s,tap_fd %d",WTPIndex,gWTPs[WTPIndex].tap_name,gWTPs[WTPIndex].tap_fd);
      
      return 0;
}




