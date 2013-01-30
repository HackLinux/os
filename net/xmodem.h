/*!
 * @file ターゲット非依存部
 * @brief xmodem転送プロトコル(送信)インターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _XMODEM_H_INCLUDED_
#define _XMODEM_H_INCLUDED_


/* os/kernel */
#include "kernel/defines.h"


/*! XMODEMでの送信制御 */
extern BOOL send_xmodem(UINT8 *bufp, UINT32 size);


#endif
