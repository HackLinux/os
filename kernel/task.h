#ifndef _TASK_H_INCLUDED_
#define _TASK_H_INCLUDED_


/* os/kernel */
#include "defines.h"
#include "syscall.h"


/*! タスク状態管理マクロ */
/*! LSBから3ビット目までをタスクの状態として使用 */
#define STATE_CLEAR										(0 << 0)		/*! タスク状態を初期化 */
#define TASK_WAIT											(0 << 0)		/*! 0なら待ち状態 */
#define TASK_READY										(1 << 0)		/*! 実行可能状態または実行状態 */
#define TASK_DORMANT									(1 << 1)		/*! 休止状態 */

/*! タスク待ち要因管理マクロ */
/*! LSBから4ビット目からタスクの待ち要因として使用 */
#define TASK_WAIT_TIME_SLEEP					(1 << 3)		/*! 起床待ち(kz_tslp_tsk()) */
#define TASK_WAIT_TIME_DELAY					(1 << 4)		/*! 時間経過待ち(kz_dly_tsk()) */
#define TASK_WAIT_SEMAPHORE						(1 << 5)		/*! セマフォ待ち */
#define TASK_WAIT_MUTEX								(1 << 6)		/*! mutex待ち */
#define TASK_WAIT_VIRTUAL_MUTEX				(1 << 7)		/*! virtual mutex待ち */
#define TASK_WAIT_MAILBOX							(1 << 8)		/*! mail box待ち */

#define TASK_STATE_INFO								(0x07 << 0)	/*! タスク状態の抜き取り */
#define TASK_WAIT_ONLY_TIME						(3 << 3)		/*! タイマ要因のみ(tslp_tsk()とdly_tsk()) */


/*! タスクのカーネルオブジェクト取得情報管理マクロ */
#define TASK_GET_SEMAPHORE						(1 << 1)		/*! セマフォ */
#define TASK_GET_MUTEX								(1 << 2)		/*! mutex */
/* メールボックス取得情報はない */

#define TASK_NOT_GET_OBJECT						(0 << 0)		/*! タスクは何も取得していない場合 */


#define TASK_NAME_SIZE								16					/*!  タスク名の最大値! */
#define INIT_TASK_ID									0						/*!  initタスクIDは0とする */


/*!
* タスクのスタート・アップ(thread_init())に渡すパラメータ.実行時に内容が変わらないところ
*/ 
typedef struct _task_init_infomation {
	int tskid;														/*! タスクID */
  TSK_FUNC func; 												/*! タスクのメイン関数 */
  int argc;       											/*! タスクのメイン関数に渡すargc */
  char **argv;    											/*! タスクのメイン関数に渡すargv */
  char name[TASK_NAME_SIZE]		; 				/*! タスク名 */
  int priority; 												/*! 起動時の優先度(優先度変更のシステムコールがあるため) */
} TSK_INITCB;


/*! タスクの待ち情報管理構造体 */
typedef struct _task_wait_infomation {
	struct _task_struct *wait_next;				/*! 待ちオブジェクトへの次ポインタ */
	struct _task_struct *wait_prev;				/*! 待ちオブジェクトへの前ポインタ */
	TMR_OBJP tobjp;												/*! タイマ関連の待ち要因がある時に使用する領域．対象タイマコントロールブロックを記録 */
	WAIT_OBJP wobjp; 											/*! 待ち行列につながれている対象コントロールブロック */
} TSK_WAIT_INFOCB;


/*! タスクのカーネルオブジェクト取得情報(休止状態または未登録状態に遷移する時に使用する) */
typedef struct _task_get_infomation {
	int flags; 														/*! 取得フラグ */
	GET_OBJP gobjp; 											/*! 取得しているカーネルオブジェクトのポインタ */
} TSK_GET_INFOCB;


/*! タスクコンテキスト */
typedef struct _task_interrupt_infomation {
	INTR_TYPE type;												/*! 割込みの種類 */
  UINT32 sp; 														/*! タスクスタックへのポインタ */
} TSK_INTR_INFOCB;


/*! システムコール用バッファ */
typedef struct _task_systemcall_infomation {
	SYSCALL_TYPE flag; 										/*! システムコールかサービスコールかを判別 */
  ISR_TYPE type; 												/*! システムコールのID */
	OBJP ret; 														/*! システムコール返却値を格納しておくポインタ */
  SYSCALL_PARAMCB *param; 							/*! システムコールのパラメータパケットへのポインタ(タスク間通信の受信処理に使用) */
} TSK_SYSCALL_INFOCB;


/*! スケジューラによって依存する情報 */
typedef struct _scheduler_depend_infomation {
	union {
		/*! タイムスライス型スケジューラに依存する情報 */
		struct {
			int tm_slice;											/*! タスクのタイムスライス(タイムスライスが絡まないスケジューリングの時は-1となる) */
		} slice_schdul;
		/* リアルタイム型スケジューラに依存する情報 */
		struct {
			int rel_exetim;										/*! 実行時間(RM専用のメンバで簡単化のため相対デッドライン時間とする) */
			int rate;													/*! 周期 */
			int deadtim;											/*! デッドライン時刻 */
			int floatim;											/*! 余裕時刻 */
			TMR_OBJP tobjp;										/*! スケジューラが使用するソフトタイマオブジェクト(EDF,LLF時使用) */
		} rt_schdul;
	} un;
} SCHDUL_DEP_INFOCB;


/*! レディーによって依存する内容(静的優先度はキュー構造のレディー,動的優先度は2分木と整列リストのレディー) */
typedef struct _ready_depend_infomation {
	union {
		/*! キュー構造のレディー */
		struct {
			struct _task_struct *ready_next; /*! レディーの次ポインタ */
  		struct _task_struct *ready_prev; /*! レディーの前ポインタ */
  	} que_ready;
		struct {
			int dummy;
		} dum;
	} un;
} READY_DEP_INFOCB;


/*!
* タスクコントロールブロック(TCB)
*/
typedef struct _task_struct {
	struct _task_struct *free_next; 	/*! free listの次ポインタ */
  struct _task_struct *free_prev; 	/*! free listの前ポインタ */
	READY_DEP_INFOCB ready_info;			/*! レディーごとに依存する内容 */
  int priority;   									/*! 静的優先度 */
	int stacksize;
  char *stack;    									/*! スタックリンカスクリプトに定義されているユーザスタック領域のポインタ */
  UINT16 state;   									/*! 状態フラグ */
  TSK_INITCB init; 									/*! 実行時に内容が変わらないところ */
  TSK_WAIT_INFOCB wait_info; 				/*! 待ち情報管理 */
	TSK_GET_INFOCB get_info; 					/*! 取得情報管理 */
	TSK_INTR_INFOCB intr_info; 				/*! 割込み情報(ここはつねに変動する) */
	TSK_SYSCALL_INFOCB syscall_info; 	/*! システムコール情報管理 */
  SCHDUL_DEP_INFOCB schdul_info;		/*! スケジューラごとに依存する情報 */
} TCB;


/*! タスク情報 */
typedef struct _task_infomation {
	TCB **id_table; 									/*! task ID変換テーブルへのheadポインタ(可変長配列として使用) */
	TCB *freehead; 										/*! taskリンクドfreeリストのキュー(freeheadポインタを記録) */
	TCB *alochead; 										/*! taskリンクドalocリストのキュー(alocheadポインタを記録) */
	int counter;											/*! 次の割付可能ID */
	int tskid_num;										/*! 現在のタスク資源ID数 */
	int power_count; 									/*! task IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} TSK_INFO;


/*! タスク情報 */
extern TSK_INFO g_tsk_info;


#endif
