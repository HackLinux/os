/*!
 * @file ターゲット非依存部
 * @brief OSを操作する基本コマンドインターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _COMMAND_H_INCLUDED_
#define _COMMAND_H_INCLUDED_


/* os/kernel */
#include "defines.h"


/*! echoコマンド */
extern void echo_command(char buf[]);

/*! helpコマンド */
extern void help_command(char *buf);

/*! sendlogコマンド */
extern void sendlog_command(void);

#ifdef TSK_LIBRARY
/*! runコマンド */
extern void run_command(char *buf);
#endif


#endif
