/*!
 * @file ターゲット非依存部<モジュール:log_manage.o>
 * @brief ロギング
 * @attention gcc4.5.x以外は試していない
 * @note 時間の対応を行う
 */


/* os/kernel_svc */
#include "log_manage.h"
/* os/kernel */
#include "kernel/kernel.h"
/* os/c_lib */
#include "c_lib/lib.h"


#define LOG_BUFFER_MAX 			4096	/*! ログバッファの最大 */
#define CONTEXT_LOG_ONESIZE 92		/*! コンテキストログ1つあたりの大きさ */

/*! ログの数 */
UINT32 g_log_counter = 0;

/*! ログ格納専用メモリセグメント */
extern volatile unsigned long _logbuffer_start;
static volatile unsigned long *logbuf = &_logbuffer_start;


/*!
 * コンテキストスイッチングのログの出力(Linux jsonログを参考(フォーマット変換は行わない))
 * log_tcb : コンテキストスイッチング対象TCB
 */
void get_log(OBJP log_tcb)
{
	static UINT32 tmp = 0;
	TCB *work = (TCB *)log_tcb;
	UINT32 tskid, priority, state;

	/* 実行可能タスクが決定していない時(init_tsk生成等で呼ばれる) */
	if (work == NULL) {
		return;
	}
	/* ログバッファがオーバーフローした場合 */
	else if (LOG_BUFFER_MAX < ((g_log_counter + 1) * CONTEXT_LOG_ONESIZE)) {
		KERNEL_OUTMSG("error: get_log() \n");
		down_system(); /* kernelのフリーズ */
	}
	else {
		/* 処理なし */
	}

	tskid = (UINT32)work->init.tskid;
	priority = (UINT32)work->priority;
	state = (UINT32)work->state;
	
	/*
	 * prevのログをメモリセグメントへ
	 * ・prevログ
	 *  システムコールを発行したタスクの次(ISR適用後(currentが切り替わるものは，適用前のログとなる))の状態
	 */
	if (g_log_counter == tmp) {
		DEBUG_LEVEL1_OUTMSG(" out prev log : get_log().\n");
		memset((char *)logbuf, 0, TASK_NAME_SIZE * 4);
		strcpy((char *)logbuf, work->init.name);
		logbuf += TASK_NAME_SIZE;
		*logbuf = tskid;
		logbuf++;
		*logbuf = tskid;
		logbuf++;
		*logbuf = priority;
		logbuf++;
		*logbuf = state;
		logbuf++;
		tmp = g_log_counter;
		g_log_counter++;
	}
	/*
	 * nextのログをメモリセグメントへ
	 * ・nextログ
	 *  次にディスパッチされるタスクの状態
	 */
	else {
		DEBUG_LEVEL1_OUTMSG(" out next log : get_log().\n");
		*logbuf = tskid;
		logbuf++;
		*logbuf = priority;
		logbuf++;
		*logbuf = state;
		logbuf++;
		tmp = g_log_counter;
	}
}
