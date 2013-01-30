/*!
 * @file ターゲット非依存部
 * @brief タスク管理インターフェース
 * @attention gcc4.5.x以外は試していない
 * @note μITRON4.0仕様参考
 */


#ifndef _TASK_MANAGE_H_INCLUDED_
#define _TASK_MANAGE_H_INCLUDED_


/* os/kernel */
#include "task.h"


/*! タスクの初期化(task ID変換テーブルの領域確保と初期化) */
extern ER tsk_init(void);

/*! システムコールの処理(acre_tsk():タスクの生成(ID自動割付)) */
extern OBJP acre_tsk_isr(TSK_FUNC func, char *name, int priority, int stacksize, 
										int rate, int exetim, int deadtim, int floatim, int argc, char *argv[]);
				 
/*! システムコール処理(del_tsk():タスクの排除) */
extern ER del_tsk_isr(TCB *tcb);

/*! システムコール処理(sta_tsk():タスクの起動) */
extern ER sta_tsk_isr(TCB *tcb);

/*! システムコールの処理(run_tsk():タスクの生成(ID自動割付)と起動) */
extern ER_ID run_tsk_isr(SYSCALL_PARAMCB *p);

/*! システムコールの処理(ext_tsk():自タスクの終了) */
extern void ext_tsk_isr(void);

/*! システムコールの処理(exd_tsk():自タスクの終了と排除) */
extern void exd_tsk_isr(void);

/*! システムコール処理(ter_tsk():タスクの強制終了) */
extern ER ter_tsk_isr(TCB *tcb);

/*! システムコールの処理(get_pri():タスクの優先度取得) */
extern ER get_pri_isr(TCB *tcb, int *p_tskpri);

/*! システム・コールの処理(chg_pri():タスクの優先度変更) */
extern ER chg_pri_isr(TCB *tcb, int tskpri);

/*! タスクの優先度を変更する関数 */
extern void chg_pri_tsk(TCB *tcb, int tskpri);

/*! システムコールの処理(get_id():タスクID取得) */
extern ER_ID get_tid_isr(void);


#endif
