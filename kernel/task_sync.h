#ifndef _TASK_SYNC_H_INCLUDED_
#define _TASK_SYNC_H_INCLUDED_


/* os/kernel */
#include "defines.h"
#include "task.h"


/*! システムコールの処理(slp_tsk():自タスクの起床待ち) */
extern ER slp_tsk_isr(void);

/*! システムコールの処理(wup_tsk():タスクの起床) */
extern ER wup_tsk_isr(TCB *tcb);

/*! システムコールの処理(rel_wai():待ち状態強制解除) */
extern ER rel_wai_isr(TCB *tcb);


#endif
