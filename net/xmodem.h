#ifndef _XMODEM_H_INCLUDED_
#define _XMODEM_H_INCLUDED_


/* os/kernel */
#include "kernel/defines.h"


/*! XMODEMでの送信制御 */
extern BOOL send_xmodem(UINT8 *bufp, UINT32 size);


#endif
