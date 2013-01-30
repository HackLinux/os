/*!
 * @file ターゲット非依存部
 * @brief スケジューリングポリシーのインターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _SCHEDULER_H_INCLUDED_
#define _SCHEDULER_H_INCLUDED_


/* os/kernel */
#include "defines.h"
#include "kernel.h"
#include "ready.h"


/*!
 * @brief スケジューラコントロールブロック
 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
 * @note スケジューラが個別に持つ情報
 */
typedef struct {
	union {
		/*!
		 * @brief FCFSスケジューリングエリア
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} fcfs_schdul;
		/*!
		 * @brief 優先度スケジューリングエリア
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} ps_schdul;
		/*!
		 * @brief Rate Monotonicエリア
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
			int unroll_rate;						/*! 周期最小公倍数(create()されたタスクまで) */
			int unroll_exetim;					/*! 周期に沿った最大実行時間(create()されたタスクまで).簡単化のため相対デッドライン時間とする */
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} rms_schdul;
		/*!
		 * @brief Deadline Monotonicエリア
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
			int unroll_dead;						/*! デッドライン最小公倍数(create()されたタスクまで) */
			int unroll_exetim;					/*! 周期に沿った最大実行時間(create()されたタスクまで).簡単化のため相対デッドライン時間とする */
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} dms_schdul;
	} un;
} SCHDULCB;


/*!
 * @brief スケジューリング情報
 * @note スケジューラで共通で持つ情報
 */
typedef struct scheduler_infomation {
	SCHDUL_TYPE type;								/*! スケジューリングタイプ */
	SCHDULCB *entry; 								/*! スケジューラコントロールブロックポインタ */
} SCHDUL_INFO;


/*! システムコール処理(sel_schdul():スケジューラの切り替え) */
extern ER sel_schdul_isr(SCHDUL_TYPE type, long param);

/*! スケジューラの初期化 */
extern ER schdul_init(void);

/*! 指定されたTCBをどのタイプのレディーキューから抜き取るか分岐 */
extern void schedule(void);

/*! Rate Monotonic専用，展開スケジューリングをするための関数 */
extern void set_unrolled_schedule_val(int rate, int exetim);


/*! スケジューリング情報 */
extern SCHDUL_INFO g_schdul_info;


#endif
