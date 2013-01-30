/*!
 * @file ターゲット非依存部
 * @brief ログ管理インターフェース
 * @attention gcc4.5.x以外は試していない
 * @note ロギング情報はまとめた方がよい
 */


#ifndef _LOG_MANAGE_H_INCLUDED_
#define _LOG_MANAGE_H_INCLUDED_


/* os/kernel */
#include "kernel/defines.h"


#define CONTEXT_LOG_ONESIZE 92		/* コンテキストログ1つあたりの大きさ */


/* ログの数 */
extern UINT32 g_log_counter;

/*! コンテキストスイッチングのログの出力(Linux jsonログを参考(フォーマット変換は行わない)) */
extern void get_log(OBJP log_tcb);


#endif
