/*!
 * @file ターゲット非依存部
 * @brief kernelのinit、カーネルコア、カーネルコアメカニズムインターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _KERNEL_H_INCLUDED_
#define _KERNEL_H_INCLUDED_


/*
 * C++へのプラグイン
 * C++はユーザ空間のみ認める
 */
#ifdef __cplusplus
extern "C" {
#endif


/* os/kernel */
#include "syscall.h"
#include "task.h"


/*!
 * @brief ディスパッチ情報
 */
typedef struct _dispatcher_infomation {
	char flag; 												/*! ディスパッチ状態フラグ */
	void (*func)(UINT32 *context);		/*! ディスパッチルーチン(1つだけしかないからtypedefしない) */
} DSP_INFO;


/* ~kernel.c~ */
/*! tskid変換テーブル設定処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)) */
extern void kernelrte_acre_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(del_tsk():スレッドの排除) */
extern void kernelrte_del_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(sta_tsk():スレッドの起動) */
extern void kernelrte_sta_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(ter_tsk():スレッドの強制終了) */
extern void kernelrte_ter_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(get_pri():スレッドの優先度取得) */
extern void kernelrte_get_pri(SYSCALL_PARAMCB *p);

/*! 変換テーブル設定処理はいらない(def_inh():割込みハンドラの定義) */
extern void kernelrte_def_inh(SYSCALL_PARAMCB *p);

/*! 変換テーブル設定処理はいらない(sel_schdul():スケジューラの切り替え) */
extern void kernelrte_sel_schdul(SYSCALL_PARAMCB *p);

/*! 非タスクコンテキスト用システムコール呼び出しライブラリ関数 */
extern void isyscall_intr(ISR_ITYPE type, SYSCALL_PARAMCB *param);

/*! スケジューラとディスパッチャの呼び出し */
extern void context_switching(INTR_TYPE type);

/*! 外部割込みハンドラを呼び出す準備 */
extern ER external_intr(INTR_TYPE type, UINT32 sp);

/*! システムコール割込みハンドラ(ISR)を呼び出す準備 */
extern void syscall_intr(ISR_TYPE type, UINT32 sp);

/*! トラップ発行(システムコール) */
extern void issue_trap_syscall(ISR_TYPE type, SYSCALL_PARAMCB *param, OBJP ret);

/*! kernelオブジェクトの初期化を行う */
extern void kernel_obj_init(void);

/*! initタスクの生成と起動を行う */
extern void start_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize, int argc, char *argv[]);

/*! kernelの初期化を行う */
extern void kernel_init(TSK_FUNC func, char *name, int priority, int stacksize, int argc, char *argv[]);

/*! OSの致命的エラー時 */
extern void down_system(void);


/* ~startup.sのアセンブララベル~ */
/*! ディスパッチャ */
void dispatch(UINT32 *context);


/* ~syscall.c~ */
/*! mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付) */
ER_ID mz_acre_tsk(SYSCALL_PARAMCB *par);

/*! mz_del_tsk():スレッドの排除 */
ER mz_del_tsk(ER_ID tskid);

/*! mz_sta_tsk():スレッドの起動 */
ER mz_sta_tsk(ER_ID tskid);

/*! mz_run_tsk():スレッドの生成(ID自動割付)と起動 */
ER_ID mz_run_tsk(SYSCALL_PARAMCB *par);

/*! mz_ext_tsk():自タスクの終了 */
void mz_ext_tsk(void);

/*! mz_exd_tsk():自スレッドの終了と排除 */
void mz_exd_tsk(void);

/*! mz_ter_tsk():スレッドの強制終了 */
ER mz_ter_tsk(ER_ID tskid);

/*! mz_get_pri():スレッドの優先度取得 */
ER mz_get_pri(ER_ID tskid, int *p_tskpri);

/*! mz_chg_pri():スレッドの優先度変更 */
ER mz_chg_pri(ER_ID tskid, int tskpri);

/*! chg_slt():タスクタイムスライスの変更 */
ER mz_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice);

/*! get_slt():タスクタイムスライスの取得 */
ER mz_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice);

/*! mz_slp_tsk():自タスクの起床待ち */
ER mz_slp_tsk(void);

/*! mz_wup_tsk():タスクの起床 */
ER mz_wup_tsk(ER_ID tskid);

/*! mz_rel_wai():待ち状態強制解除 */
ER mz_rel_wai(ER_ID tskid);

/*! mz_get_mpf():動的メモリ獲得 */
void* mz_get_mpf(int size);

/*! mz_rel_mpf():動的メモリ解放 */
int mz_rel_mpf(void *p);

/*! mz_def_inh():割込みハンドラの定義 */
ER mz_def_inh(INTRPT_TYPE type, IR_HANDL handler);

/* 非タスクコンテキストから呼ぶシステムコールのプロトタイプ，実体はsyscall.cにある) */
/*! mz_iacre_tsk():タスクの生成 */
ER mz_iacre_tsk(SYSCALL_PARAMCB *par);
/*! mz_ista_tsk():タスクの起動 */
ER mz_ista_tsk(ER_ID tskid);

/*! mz_ichg_pri():タスクの優先度変更 */
ER mz_ichg_pri(ER_ID tskid, int tskpri);

/*! mz_iwup_tsk():タスクの起床 */
ER mz_iwup_tsk(ER_ID tskid);


/* サービスコール(ユーザタスクが呼ぶシステムコールのプロトタイプ，実体はsrvcall.cにある) */
/*! mv_acre_tsk():タスクコントロールブロックの生成(ID自動割付) */
ER_ID mv_acre_tsk(SYSCALL_PARAMCB *par);
		      
/*! mv_del_tsk():スレッドの排除 */
ER mv_del_tsk(ER_ID tskid);

/*! mz_ter_tsk():スレッドの強制終了 */
ER mv_ter_tsk(ER_ID tskid);

/*! mv_get_pri():スレッドの優先度取得 */
ER mv_get_pri(ER_ID tskid, int *p_tskpri);

/*! mv_def_inh():割込みハンドラの定義 */
ER_ID mv_def_inh(INTRPT_TYPE type, IR_HANDL handler);

/*! mv_sel_schdul():スケジューラの切り替え */
ER mv_sel_schdul(SCHDUL_TYPE type, long param);


/*! initタスク */
int start_threads(int argc, char *argv[]);
ER_ID idle_id;

#ifdef TSK_LIBRARY
/*! ユーザタスク及び資源情報 */
int sample_tsk1_main(int argc, char *argv[]);
int sample_tsk2_main(int argc, char *argv[]);
int sample_tsk3_main(int argc, char *argv[]);
int sample_tsk4_main(int argc, char *argv[]);
int sample_tsk5_main(int argc, char *argv[]);
int sample_tsk6_main(int argc, char *argv[]);
int sample_tsk7_main(int argc, char *argv[]);
int sample_tsk8_main(int argc, char *argv[]);
extern ER_ID sample_tsk1_id;
extern ER_ID sample_tsk2_id;
extern ER_ID sample_tsk3_id;
extern ER_ID sample_tsk4_id;
extern ER_ID sample_tsk5_id;
extern ER_ID sample_tsk6_id;
extern ER_ID sample_tsk7_id;
extern ER_ID sample_tsk8_id;
#endif


/*! カレントタスク(実行状態のタスクの事) */
extern TCB *g_current;

/*! ディスパッチャ情報 */
extern DSP_INFO g_dsp_info;


/*
 * C++へのプラグイン
 * C++はユーザ空間のみ認める
 */
#ifdef __cplusplus
}
#endif


#endif
