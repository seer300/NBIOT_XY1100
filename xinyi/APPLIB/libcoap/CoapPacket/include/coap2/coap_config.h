#ifndef COAP_CONFIG_H_
#define COAP_CONFIG_H_

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/def.h> /* provide ntohs, htons */

#include "softap_macro.h"
//#define WITH_LWIP 1

#ifndef COAP_CONSTRAINED_STACK
#define COAP_CONSTRAINED_STACK 1
#endif

#define PACKAGE_NAME "libcoap-lwip"
#define PACKAGE_VERSION "?"
#define PACKAGE_STRING PACKAGE_NAME PACKAGE_VERSION

/* it's just provided by libc. i hope we don't get too many of those, as
 * actually we'd need autotools again to find out what environment we're
 * building in */
#define HAVE_STRNLEN 1

#define HAVE_LIMITS_H

#endif /* COAP_CONFIG_H_ */
