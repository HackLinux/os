#ifndef _TIMER_DRIVER_H_INCLUDED_
#define _TIMER_DRIVER_H_INCLUDED_


/* os/kernel */
#include "kernel/defines.h"


/*! 周期タイマスタート */
extern void start_cycle_timer(int index, int usec);

/*! ワンショットタイマスタート */
extern void start_oneshot_timer(int index, int usec);

/*! 周期タイマ割込み満了処理 */
extern void expire_cycle_timer(int index);

/*! ワンショットタイマ割込み満了処理 */
extern void expire_oneshot_timer(int index);

/*! タイマの停止 */
extern void stop_timer(int index);

/*! タイマキャンセル */
extern void cancel_timer(int index);

/*! タイマ動作中か検査する関数 */
extern ER_VLE get_timervalue(int index);


#endif
