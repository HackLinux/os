#ifndef _MULTI_TIMER_H_INCLUDED_
#define _MULTI_TIMER_H_INCLUDED_


/* os/kernel */
#include "defines.h"


/* ソフトタイマの要求種類を操作するフラグ */
#define SCHEDULER_MAKE_TIMER 		(0 << 0)	/*! スケジューラが使用するタイマを記録する */
#define OTHER_MAKE_TIMER 				(1 << 0)	/*! 上記以外が使用するタイマを記録する */


/*! タイマコントロールブロック*/
typedef struct _timer_struct {
	struct _timer_struct *next;							/*! 次ポインタ*/
	struct _timer_struct *prev;							/*! 前ポインタ*/
	short flag;															/*! スケジューラが使用するタイマブロックかその他が使用するタイマブロックかを記録 */
	int usec;																/*! 要求タイマ値*/
	TMRRQ_OBJP rqobjp;											/*! タイマを要求したオブジェクトのポインタ */
	TMR_CALLRTE func;												/*! コールバックルーチン */
	void *argv;															/*! コールバックルーチンへのポインタ */
} TMRCB;

/*! タイマキュー型構造体 */
typedef struct _timer_queue {
	TMRCB *tmrhead;													/*! タイマコントロールブロックの先頭ポインタ */
	int index;															/*! タイマ番号 */
} TMR_INFO;


/*! 周期タイマハンドラ */
extern void cyclic_timer_handler1(void);

/*! ワンショットタイマハンドラ */
extern void oneshot_timer_handler1(void);

/*! 差分のキューのノードを作成 */
extern OBJP create_tmrcb_diffque(short flag, int request_sec, TMRRQ_OBJP rqobjp, TMR_CALLRTE func, void *argv);

/*! 差分のキューのノードを排除 */
extern void delete_tmrcb_diffque(TMRCB *deltbf);

/*! タイマ情報 */
extern TMR_INFO g_timerque;


#endif
