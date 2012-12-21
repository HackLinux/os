/* os/kernel */
#include "intr_manage.h"
#include "kernel.h"
#include "memory.h"
#include "ready.h"
#include "scheduler.h"
#include "task_manage.h"
#include "task_sync.h"
#include "multi_timer.h"
/* os/arch */
#include "arch/cpu/intr.h"
/* os/c_lib */
#include "c_lib/lib.h"
/* os_kernel_svc */
#include "kernel_svc/log_manage.h"
/* target/driver */
#include "target/driver/timer_driver.h"


/*! タスク生成パラメータチェック関数 */
static ER check_cre_tsk(TSK_FUNC func, int priority, int stacksize, int rate, int rel_exetim, int deadtim, int floatim);

/*! tskid変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_tskid_table(int tskids);

/*! task IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_tskid_table(void);

/*! task IDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_tsk(TCB **tsktbl_tmp, TCB *dtsk_atmp, TCB *dtsk_ftmp);

/*! task IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dtsk(TCB *dtsk_atmp, TCB *dtsk_ftmp, int tskids);

/*! システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動) */
static void kernelrte_run_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理はいらない(ext_tsk():自タスクの終了) */
static void kernelrte_ext_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理はいらない(exd_tsk():自スレッドの終了と排除) */
static void kernelrte_exd_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(chg_pri():スレッドの優先度変更) */
static void kernelrte_chg_pri(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理はいらない(slp_tsk():自タスクの起床待ち) */
static void kernelrte_slp_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(wup_tsk():タスクの起床) */
static void kernelrte_wup_tsk(SYSCALL_PARAMCB *p);

/*! tskid変換テーブル設定処理(rel_wai():待ち状態強制解除) */
static void kernelrte_rel_wai(SYSCALL_PARAMCB *p);

/*! システムコール処理(get_mpf():動的メモリ獲得) */
static void kernelrte_get_mpf(SYSCALL_PARAMCB *p);

/*! システムコール処理(rel_mpf():動的メモリ解放) */
static void kernelrte_rel_mpf(SYSCALL_PARAMCB *p);

/*! ディスパッチャの初期化 */
static void dispatch_init(void);

/*! デフォルトのスケジューラを登録する関数 */
static void set_schdul_init_tsk(void);


/*! カレントタスク(実行状態のタスクの事) */
TCB *g_current = NULL;

/*! ディスパッチャ情報 */
DSP_INFO g_dsp_info = {0, NULL};

/*! タスク情報 */
TSK_INFO g_tsk_info = {NULL, NULL, NULL, 0, 0, TASK_ID_NUM};


/*! タスクコンテキスト用のISRハンドラ */
static void (*sg_isr_handlers[ISR_NUM])(SYSCALL_PARAMCB *p) =
{
		kernelrte_acre_tsk, 	kernelrte_del_tsk, 	kernelrte_sta_tsk, 	kernelrte_run_tsk,
		kernelrte_ext_tsk, 	kernelrte_exd_tsk, 	kernelrte_ter_tsk, 	kernelrte_get_pri,
		kernelrte_chg_pri, 	kernelrte_slp_tsk, 	kernelrte_wup_tsk, 	kernelrte_rel_wai,
		kernelrte_get_mpf, 	kernelrte_rel_mpf,
		kernelrte_def_inh, 	kernelrte_sel_schdul,
};

/*! 非タスクコンテキスト用のISRハンドラ */
static void (*sg_isr_ihandlers[ISR_INUM])(SYSCALL_PARAMCB *p) =
{
	kernelrte_acre_tsk, kernelrte_sta_tsk,
};


/*!
* タスク生成パラメータチェック関数
* func : タスクのメイン関数
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* rate : 周期
* rel_exetim : 実行時間(仮想)
* deadtim : デッドライン時刻
* floatim : 余裕時間
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_OK : 生成可能
*/
static ER check_cre_tsk(TSK_FUNC func, int priority, int stacksize, int rate, int rel_exetim, int deadtim, int floatim)
{
	SCHDUL_TYPE schdul_type = g_schdul_info.type;

	/* システムコールの引数は正しいか(スタックサイズの最低0x48分を使用) */
  if (!func || priority >= PRIORITY_NUM || stacksize < 0x48) {
  	return E_PAR;
  }
  
  /* Deadline Monotonic時のパラメータチェック(initタスクは省く) */
  else if (schdul_type == DM_SCHEDULING && g_tsk_info.counter > INIT_TASK_ID) {
  	if (rate <= 0 || rel_exetim <= 0 || deadtim <= 0 || rate < rel_exetim || deadtim < rel_exetim || rate < deadtim) {
  		return E_PAR; /* 生成不可 */
  	}
  	else {
  		return E_OK; /* 生成可能 */
  	}
  }
  
  /* RMとDM,EDF,LLF以外の時の優先度チェック */
  else if (priority < -1) {
  		return E_PAR;
  }

	else {
		return E_OK;
	}
}


/*!
* tskid変換テーブル設定処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* -この関数からmz_acre_tsk()のISRを呼ぶ
* -IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(タスクがdeleteされている事があるため)
* *p : システムコールバッファポインタ
*/
void kernelrte_acre_tsk(SYSCALL_PARAMCB *p)
{
	int tskids;

	/* システムコールパラメータ設定(効率化) */
	TSK_FUNC func = p->un.acre_tsk.func;
	char *name = p->un.acre_tsk.name;
	int priority = p->un.acre_tsk.priority;
	int stacksize = p->un.acre_tsk.stacksize;
	int rate = p->un.acre_tsk.rate;
	int rel_exetim = p->un.acre_tsk.rel_exetim;
	int deadtim = p->un.acre_tsk.deadtim;
	int floatim = p->un.acre_tsk.floatim;
	int argc = p->un.acre_tsk.argc;
	char **argv = p->un.acre_tsk.argv;
	
	/* タスク生成可能状態かチェックルーチン呼び出し */
	if (E_PAR == check_cre_tsk(func, priority, stacksize, rate, rel_exetim, deadtim, floatim)) {
		p->un.acre_tsk.ret = E_PAR; /* システムコールのエラーコードを設定(システムコールのパラメータエラー) */
		return;
	}
	/* 次の割付けIDが存在しない場合(ID不足)，または次の割付けIDが使用されている場合(タスクがdeleteされている) */
	else if ((tskids = g_tsk_info.tskid_num) <= g_tsk_info.counter || g_tsk_info.id_table[g_tsk_info.counter] != NULL) {
		/* 割付け可能なIDを検索(存在しない場合は，取得してくる) */
		if (0 > (g_tsk_info.counter = (int)serch_tskid_table(tskids))) {
			p->un.acre_tsk.ret = E_NOID; /* システムコールのエラーコードを設定(割付け可能なIDが存在しない) */
			return;
		}
	}
	/* 以外 */
	else {
		/* 処理なし */
	}

	/* ISRの呼び出し(mz_acre_tsk())とID変換テーブルの設定 */
	g_tsk_info.id_table[g_tsk_info.counter] = (TCB *)acre_tsk_isr(func, name, priority, 
																																stacksize, rate, rel_exetim, deadtim,floatim, argc, argv);
	/* システムコールのリターンパラメータ型へ変換 */
	p->un.acre_tsk.ret = (ER_ID)g_tsk_info.counter; /* 生成したタスクIDを設定 */
	g_tsk_info.counter++;

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* tskid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOID) : 割付け可能なIDが存在しない(メモリ不足)
*/
static ER_ID serch_tskid_table(int tskids)
{
	ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* tskid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < tskids; srh_cuntr++) {
		if (g_tsk_info.id_table[srh_cuntr] == NULL) {
			DEBUG_LEVEL1_OUTVLE(srh_cuntr, 0);
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_tskid_table() != E_OK) { /* tskidが足らなくなった場合 */
		srh_cuntr = E_NOID;
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = tskids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_LEVEL1_OUTVLE(srh_cuntr, 0);
  DEBUG_LEVEL1_OUTMSG(" next tsk counter : serch_tskid_table()\n");
  
	return srh_cuntr;
}


/*!
* task IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_tskid_table(void)
{
	TCB **tsktbl_tmp, *dtsk_atmp, *dtsk_ftmp;
	
	g_tsk_info.power_count++;
	g_tsk_info.tskid_num = TASK_ID_NUM << g_tsk_info.power_count;
	
	/* 現在使用していた領域すべてを一時退避 */
	tsktbl_tmp = g_tsk_info.id_table;
	dtsk_atmp = g_tsk_info.alochead;
	dtsk_ftmp = g_tsk_info.freehead; /* この時，必ずg_tsk_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (tsk_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_tsk(tsktbl_tmp, dtsk_atmp, dtsk_ftmp);
	
	/* 退避していた領域を解放(動的型alarm handlerリストは解放しない) */
	rel_mpf_isr(tsktbl_tmp);
	
	return E_OK;
}


/*!
* task IDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **tsktbl_tmp : 古いtask ID変換テーブル
* *dtsk_atmp : 古い動的型task aloc list head
* *dtsk_ftmp : 古い動的型task free list head
*/
static void copy_all_tsk(TCB **tsktbl_tmp, TCB *dtsk_atmp, TCB *dtsk_ftmp)
{
	int tskids;
	
	tskids = TASK_ID_NUM << (g_tsk_info.power_count - 1); /* 古いtask ID資源数 */
	memcpy(g_tsk_info.id_table, tsktbl_tmp, sizeof(&(**tsktbl_tmp)) * tskids); /* 可変長配列なのでコピー */
	
	copy_dtsk(dtsk_atmp, dtsk_ftmp, tskids); /* 動的型taskリストの連結と更新(コピーはしない) */
}


/*!
* task IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
* tskids : 古いtask ID資源数
* *dtsk_atmp : 古い動的型task aloc list head
* *dtsk_ftmp : 古い動的型task free list head
*/
static void copy_dtsk(TCB *dtsk_atmp, TCB *dtsk_ftmp, int tskids)
{
	TCB *oldtcb;
	
	/*
	* 動的型taskはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型task freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dtsk_ftmp == NULL) {
		oldtcb = dtsk_atmp;
	}
	else {
		oldtcb = dtsk_ftmp;
	}
	for (; oldtcb->free_next != NULL; oldtcb = oldtcb->free_next) {
		;
	}
	/*
	* old task freeリストと new task freeリストの連結する
	* コピーはしない
	*/
	oldtcb->free_next = g_tsk_info.freehead;
	g_tsk_info.freehead->free_prev = dtsk_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dtsk_ftmp != NULL) {
		g_tsk_info.freehead = dtsk_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dtsk_atmp != NULL) {
		g_tsk_info.alochead = dtsk_atmp;
	}
}


/*!
* tskid変換テーブル設定処理(del_tsk():スレッドの排除)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクがすでに未登録状態)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
void kernelrte_del_tsk(SYSCALL_PARAMCB *p)
{
	ER ercd;

	ER_ID tskid;
	tskid = p->un.del_tsk.tskid;
	
	/* 作成であるacre_tsk()でE_NOIDを返していたならばsta_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.del_tsk.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.del_tsk.ret = E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_tsk_isr(g_tsk_info.id_table[tskid]);
		/* 排除できたならば，taskID変換テーブルを初期化 */
		if (ercd == E_OK) {
			g_tsk_info.id_table[tskid] = NULL;
		}
		
		p->un.del_tsk.ret = ercd;
	}

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブル設定処理(sta_tsk():スレッドの起動)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
void kernelrte_sta_tsk(SYSCALL_PARAMCB *p)
{
	
	ER_ID tskid;
	tskid = p->un.sta_tsk.tskid;

	/*
	* ・ログの出力(ISR呼び出しでg_currentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばsta_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.sta_tsk.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.sta_tsk.ret = E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.sta_tsk.ret = sta_tsk_isr(g_tsk_info.id_table[tskid]);
	}
}


/*!
* システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動)
* *p : システムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)rcd : 自動割付したID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(対象タスクが未登録)
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
static void kernelrte_run_tsk(SYSCALL_PARAMCB *p)
{

	/* 割込みサービスルーチンの呼び出し */
	p->un.run_tsk.ret = run_tsk_isr(p);
}


/*!
* tskid変換テーブル設定処理はいらない(ext_tsk():自タスクの終了)
* リターンパラメータはあってはならない
* この関数は他のシステムコールと同様に一貫性を保つために追加した
*/
static void kernelrte_ext_tsk(SYSCALL_PARAMCB *p)
{
	getcurrent(); /* システムコール発行タスクをレディーから抜き取る */

	ext_tsk_isr(); /* 割込みサービスルーチンの呼び出し */

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブル設定処理はいらない(exd_tsk():自スレッドの終了と排除)
* TCBが排除されるので返却値はなしにする
* この関数は他のシステムコールと同様に一貫性を保つために追加した
*/
static void kernelrte_exd_tsk(SYSCALL_PARAMCB *p)
{
	getcurrent(); /* システムコール発行タスクをレディーから抜き取る */

	exd_tsk_isr(); /* 割込みサービスルーチンの呼び出し */

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブル設定処理(ter_tsk():スレッドの強制終了)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
void kernelrte_ter_tsk(SYSCALL_PARAMCB *p)
{
	ER_ID tskid;
	tskid = p->un.ter_tsk.tskid;

	/* ログの出力(ISR呼び出しでg_currentは切り替わる事があるので，ここの位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばter_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.ter_tsk.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.ter_tsk.ret = E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.ter_tsk.ret = ter_tsk_isr(g_tsk_info.id_table[tskid]);
	}
}


/*!
* tskid変換テーブル設定処理(get_pri():スレッドの優先度取得)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
void kernelrte_get_pri(SYSCALL_PARAMCB *p)
{
	ER_ID tskid = p->un.get_pri.tskid;
	int *p_tskpri = p->un.get_pri.p_tskpri;

	READY_TYPE type = g_ready_info.type;

	/* 作成であるacre_tsk()でE_NOIDを返していたならばget_pri()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.get_pri.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.get_pri.ret = E_NOEXS;
	}
	/* スケジューラによって認めているか */
	else if (type == SINGLE_READY_QUEUE) {
		p->un.get_pri.ret = E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.get_pri.ret = get_pri_isr(g_tsk_info.id_table[tskid], p_tskpri);
	}

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブル設定処理(chg_pri():スレッドの優先度変更)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
static void kernelrte_chg_pri(SYSCALL_PARAMCB *p)
{
	ER_ID tskid = p->un.chg_pri.tskid;
	int tskpri = p->un.chg_pri.tskpri;
	
	READY_TYPE r_type = g_ready_info.type;
	SCHDUL_TYPE s_type = g_schdul_info.type;

	/* 作成であるacre_tsk()でE_NOIDを返していたならばchg_pri()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.chg_pri.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.chg_pri.ret = E_NOEXS;
	}
	/* スケジューラによって認めているか(スケジュール属性作った方がいいかな～) */
	else if (r_type == SINGLE_READY_QUEUE || s_type >= RM_SCHEDULING) {
		p->un.chg_pri.ret = E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.chg_pri.ret = chg_pri_isr(g_tsk_info.id_table[tskid], tskpri);
	}
}


/*!
* tskid変換テーブル設定処理はいらない(slp_tsk():自タスクの起床待ち)
* *p : システムコールバッファポインタ
* (返却値)E_NOSPT : 未サポート
* (返却値)E_OK : 正常終了
*/
static void kernelrte_slp_tsk(SYSCALL_PARAMCB *p)
{
	SCHDUL_TYPE type = g_schdul_info.type;
	
	/* スケジューラによって認めているか */
	if (type >= RM_SCHEDULING) {
		p->un.slp_tsk.ret = E_NOSPT;
	}
	/* 割込みサービスルーチン呼び出し */
	else {
		getcurrent(); /* システムコール発行タスクをレディーから抜き取る */
		p->un.slp_tsk.ret = slp_tsk_isr(); /* 割込みサービスルーチンの呼び出し */
	}

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* tskid変換テーブル設定処理(wup_tsk():タスクの起床)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
static void kernelrte_wup_tsk(SYSCALL_PARAMCB *p)
{
	ER_ID tskid = p->un.wup_tsk.tskid;

	/* ログの出力(ISR呼び出しでg_currentは切り替わる事があるので，ここの位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばwup_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.wup_tsk.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.wup_tsk.ret = E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.wup_tsk.ret = wup_tsk_isr(g_tsk_info.id_table[tskid]);
	}
}


/*!
* tskid変換テーブル設定処理(rel_wai():待ち状態強制解除)
* *p : システムコールバッファポインタ
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが待ち状態ではない
* (返却値)E_OK : 正常終了
*/
static void kernelrte_rel_wai(SYSCALL_PARAMCB *p)
{
	ER_ID tskid = p->un.rel_wai.tskid;

	/* ログの出力(ISR呼び出しでg_currentは切り替わる事があるので，ここの位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばrel_wai()ではE_IDを返却 */
	if (tskid == E_NOID || g_tsk_info.tskid_num <= tskid) {
		p->un.rel_wai.ret = E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (g_tsk_info.id_table[tskid] == NULL) {
		p->un.rel_wai.ret = E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		p->un.rel_wai.ret = rel_wai_isr(g_tsk_info.id_table[tskid]);
	}
}


/*!
* システムコール処理(get_mpf():動的メモリ獲得)
* *p : システムコールバッファポインタ
*/
static void kernelrte_get_mpf(SYSCALL_PARAMCB *p)
{
	int size = p->un.get_mpf.size;

  p->un.get_mpf.ret = get_mpf_isr(size);

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*! 
*システムコール処理(rel_mpf():動的メモリ解放)
* *p : システムコールバッファポインタ
*/
static void kernelrte_rel_mpf(SYSCALL_PARAMCB *p)
{
	char *ptr = p->un.rel_mpf.p;

  rel_mpf_isr(ptr);
  p->un.rel_mpf.ret = 0;

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* 変換テーブル設定処理はいらない(def_inh():割込みハンドラの定義)
* *p : システムコールバッファポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
void kernelrte_def_inh(SYSCALL_PARAMCB *p)
{
	INTRPT_TYPE type = p->un.def_inh.type;
	IR_HANDL handler = p->un.def_inh.handler;

	p->un.def_inh.ret = def_inh_isr(type, handler);

	/* ログの出力(ISR呼び出しでg_currentは切り替わらないため，この位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);
}


/*!
* 変換テーブル設定処理はいらない(sel_schdul():スケジューラの切り替え)
* *p : システムコールバッファポインタ
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
void kernelrte_sel_schdul(SYSCALL_PARAMCB *p)
{
	SCHDUL_TYPE type = p->un.sel_schdul.type;
	long param = p->un.sel_schdul.param;

	/* ログの出力(ISR呼び出しでg_currentは切り替わる事があるので，ここの位置でログ出力) */
	DEBUG_LEVEL2_LOG_CONTEXT(g_current);

	p->un.sel_schdul.ret = sel_schdul_isr(type, param); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 非タスクコンテキスト用システムコール呼び出しライブラリ関数
* これはタスクコンテキスト用システムコール呼び出しと一貫性を保つため追加した
* トラップの発行は行わない
*/
void isyscall_intr(ISR_ITYPE type, SYSCALL_PARAMCB *p)
{
	ER *ercd;

	g_current->intr_info.type = SYSCALL_INTERRUPT; /* システムコール割込み実行を記録 */	
	
	/* ISRが登録されている場合 */
	if ((*sg_isr_ihandlers[type])) {
		/* ディスパッチ禁止状態の場合 */
		if (g_dsp_info.flag == FALSE && type != (ISR_ITYPE)ISR_TYPE_ENA_DSP) {
			ercd = (ER *)g_current->syscall_info.ret;
			*ercd = E_CTX; /* システムコール発行タスクにディスパッチ禁止状態(E_CTX)を返却 */
		}
		g_current->syscall_info.flag = MZ_ISYSCALL; /* システムコールタイプを記録 */
		/*
		* システムコールのラッパーでシステムコール発行タスクをレディーへ戻す処理が
		* があるので，不整合が起こる(非タスクコンテキスト用システムコールはタスクを
		* スイッチングしない)ここで，g_currentのシステムコールフラグを非タスクコンテ
		*キスト用にしておく事によってputcurrent()及びgetcurrent()ではじかれるようになる
		* 非タスクコンテキスト用システムコールは外部割込みなどの延長上(単なる関数呼び出し)
		* で呼ばれるので，外部割込みが終了した時にスケジューラが起動される事となる
		*/
    (*sg_isr_ihandlers[type])(p); /* 割込みハンドラ起動 */
	}
	/* ISRが未登録の場合 */
	else {
		ercd = (ER *)g_current->syscall_info.ret;
			*ercd = EV_NORTE;
	}
}


/*!
 * スケジューラとディスパッチャの呼び出し
 * (返却値)E_OK : 正常終了
 * ・実質上記のエラーコードは返却されない
 * ・システムコールの場合は，システムコントロールブロックのretをユーザタスク側の返却値とする
 */
void context_switching(INTR_TYPE type)
{
	schedule(); /* スケジューラ呼び出し */
  
	if (type == SYSCALL_INTERRUPT) {
			DEBUG_LEVEL2_LOG_CONTEXT(g_current); /* 次に実行されるタスクのログを出力 */
	}
	(*g_dsp_info.func)(&g_current->intr_info.sp); /* ディスパッチャの呼び出し */

	/* ここには返ってこない */

}


/*!
* 外部割込みハンドラを呼び出す準備
* ・外部割込み(シリアル割込み，タイマ割込み)はカレントタスクのスタックへコンテキストを保存し，
* 　スケジューラ→ディスパッチャという順となる
* type : 外部割込みのタイプ
* sp : タスクコンテキストのポインタ
* (返却値)E_OK : 正常終了(実質上記のエラーコードは返却されない)
* (返却値)EV_NORTE : ハンドラが未登録
*/
ER external_intr(INTR_TYPE type, UINT32 sp)
{
	
	/*! カレントタスクのコンテキストを保存 */
  g_current->intr_info.sp = sp;
	g_current->intr_info.type = type;

  if ((*g_exter_handlers[type])) {
    (*g_exter_handlers[type])(); /* 割込みハンドラ起動 */

		return E_OK;
	}
	/* ハンドラが未登録の場合 */
	else {
		return EV_NORTE;
	}
}


/*!
* システムコール割込みハンドラ(ISR)を呼び出す準備
* ・カレントタスクのスタックへコンテキストを保存し，スケジューラ→ディスパッチャという順となる
* type : ISRのタイプ
* sp : タスクコンテキストのポインタ
*/
void syscall_intr(ISR_TYPE type, UINT32 sp)
{
	ER *ercd;

  g_current->intr_info.sp = sp; /* カレントタスクのコンテキストを保存 */
	g_current->intr_info.type = SYSCALL_INTERRUPT; /* システムコール割込み実行を記録 */

	/* ISRが登録されている場合 */
	if ((*sg_isr_handlers[type])) {
		/* ディスパッチ禁止状態の場合 */
		if (g_dsp_info.flag == FALSE && type != ISR_TYPE_ENA_DSP) {
			ercd = (ER *)g_current->syscall_info.ret;
			*ercd = E_CTX; /* システムコール発行タスクにディスパッチ禁止状態(E_CTX)を返却 */
		}
		g_current->syscall_info.flag = MZ_SYSCALL; /* システムコールタイプを記録 */
    (*sg_isr_handlers[type])(g_current->syscall_info.param); /* 割込みハンドラ起動 */
	}
	/* ISRが未登録の場合 */
	else {
		ercd = (ER *)g_current->syscall_info.ret;
			*ercd = EV_NORTE;
	}
}


/*!
* トラップ発行(システムコール)
* type : システムコールのタイプ
* *param : システムコールパケットへのポインタ
* ret : システムコール返却値格納ポインタ
*/
void issue_trap_syscall(ISR_TYPE type, SYSCALL_PARAMCB *param, OBJP ret)
{
  g_current->syscall_info.type  = type;
  g_current->syscall_info.param = param;
  g_current->syscall_info.ret = ret;

}


/*! ディスパッチャの初期化 */
static void dispatch_init(void)
{
	g_dsp_info.flag = TRUE;
	g_dsp_info.func = dispatch;
}


/*! デフォルトのスケジューラを登録する関数 */
static void set_schdul_init_tsk(void)
{
	/* 割込みサービスルーチンを直接呼ぶ */
	sel_schdul_isr(PRI_SCHEDULING, 0);
	/* rol_sys()のISRは発行しない */
}


/*! kernelオブジェクトの初期化を行う */
void kernel_obj_init(void)
{
  dispatch_init(); /* ディスパッチャの初期化 */
  mem_init(); /* 動的メモリの初期化 */
  /* スケジューラの初期化 */
  if (schdul_init() != E_OK) {
		KERNEL_OUTMSG("error: schdul_init() \n");
    down_system();
  }
  /* レディーの初期化 */
  if (ready_init() != E_OK) {
		KERNEL_OUTMSG("error: ready_init() \n");
    down_system();
  }
  /* タスク周りの初期化(静的，動的) */
  if (tsk_init() != E_OK) {
		KERNEL_OUTMSG("error: tsk_init() \n");
    down_system(); /* メモリが取得できない場合はOSをスリープさせる */
  }

	/* 以下のhandlerはstartup時にセットする */
	KERNEL_OUTMSG("　undefined handler ok\n");
	KERNEL_OUTMSG(" swi handler ok\n");
  KERNEL_OUTMSG(" prefetch abort handler ok\n");
	KERNEL_OUTMSG(" data abort handler ok\n");
	KERNEL_OUTMSG(" irq handler(serial, timer) ok\n");

}


/*!
 * initタスクの生成と起動を行う
 * -initタスクを起動する時にはシステムコールは使用できないため，生成と起動は直接内部関数を呼ぶ
 * func : タスクのメイン関数
 * *name : タスクの名前
 * priority : タスクの優先度
 * stacksize : ユーザスタック(タスクのスタック)のサイズ
 * argc : タスクのメイン関数の第一引数
 * argv[] : タスクのメイン関数の第二引数
 */
void start_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize,
		      int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk_param1, tsk_param2;

	tsk_param1.un.acre_tsk.func = func;
	tsk_param1.un.acre_tsk.name = name;
	tsk_param1.un.acre_tsk.priority = priority;
	tsk_param1.un.acre_tsk.stacksize = stacksize; /* 最低0x40必要 */
	tsk_param1.un.acre_tsk.argc = 0;
	tsk_param1.un.acre_tsk.argv = NULL;
	
  /* システムコール発行不可なので直接内部関数を呼んでタスクを生成する(initに周期，実行時間，デッドライン時刻は設定しない) */
  kernelrte_acre_tsk(&tsk_param1);

	tsk_param2.un.sta_tsk.tskid = tsk_param1.un.acre_tsk.ret;
  kernelrte_sta_tsk(&tsk_param2); /* タスクの起動 */
	
  /* 最初のタスクの起動 */
  (*g_dsp_info.func)(&g_current->intr_info.sp); /* ディスパッチャの呼び出し */

  /* ここには返ってこないこない */
}


/*!
 * kernelの初期化を行う
 * -具体的には，スケジューリングの設定，kernelオブジェクトの初期化，initタスクの生成と起動を行う
 * func : タスクのメイン関数
 * *name : タスクの名前
 * priority : タスクの優先度
 * stacksize : ユーザスタック(タスクのスタック)のサイズ
 * argc : タスクのメイン関数の第一引数
 * argv[] : タスクのメイン関数の第二引数
 */
void kernel_init(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[])
{
	set_schdul_init_tsk(); /* デフォルトでのスケジューラ設定 */
	
	kernel_obj_init(); /* kernelオブジェクトの初期化 */

	start_init_tsk(func, name, priority, stacksize, argc, argv); /* initタスクの生成と起動 */

  /* ここには返ってこないこない */
}


/*! OSの致命的エラー時 */
void down_system(void)
{
  KERNEL_OUTMSG("system error! kernel freeze!\n");
  /* システムをとめる */
  while (1) {
    ;
	}
}
