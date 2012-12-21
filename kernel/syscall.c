/* os/kernel */
#include "defines.h"
#include "kernel.h"
#include "syscall.h"
/* os/c_lib */
#include "c_lib/lib.h"


/* システムコール */
/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* -acre_tsk()システムコールはパラメータ数が多いので,とりあえず構造体×共用体でやった(他のもやったほうがいいのかな～).
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 割付可能なIDがない(割付可能なIDが存在しない)
* (返却値)E_NOMEM : タスクスタックが確保できない
* (返却値)tskid : 割付たタスクID
*/
ER_ID mz_acre_tsk(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/*
	* パラメータ退避
	* ここでローカル変数に退避させることによって，ユーザ空間の変数にアクセスしない
	*/
  param.un.acre_tsk.func = par->un.acre_tsk.func;
  param.un.acre_tsk.name = par->un.acre_tsk.name;
  param.un.acre_tsk.priority = par->un.acre_tsk.priority;
  param.un.acre_tsk.stacksize = par->un.acre_tsk.stacksize;
  param.un.acre_tsk.rate = par->un.acre_tsk.rate;
  param.un.acre_tsk.rel_exetim = par->un.acre_tsk.rel_exetim;
	param.un.acre_tsk.deadtim = par->un.acre_tsk.deadtim;
	param.un.acre_tsk.floatim = par->un.acre_tsk.floatim;
  param.un.acre_tsk.argc = par->un.acre_tsk.argc;
  param.un.acre_tsk.argv = par->un.acre_tsk.argv;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_TSK, &param, (OBJP)(&(param.un.acre_tsk.ret)));
	asm volatile ("swi #0");
	
	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_tsk():スレッドの排除)
* tskid : 排除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクがすでに未登録状態)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
ER mz_del_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.del_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_DEL_TSK, &param, (OBJP)(&(param.un.del_tsk.ret)));
	asm volatile ("swi #1");

	/* 割込み復帰後はここへもどってくる */

	return param.un.del_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sta_tsk():スレッドの起動)
* tskid : 起動するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER mz_sta_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.sta_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_STA_TSK, &param, (OBJP)(&(param.un.sta_tsk.ret)));
	asm volatile ("swi #2");

	/* 割込み復帰後はここへもどってくる */

	return param.un.sta_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_run_tsk():タスクコントロールブロックの生成(ID自動割付)と起動)
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 割付可能なIDが存在しない(メモリが取得できない)
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)rcd : 自動割付したタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(対象タスクが未登録)
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER_ID mz_run_tsk(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/*
	* パラメータ退避
	* ここでローカル変数に退避させることによって，ユーザ空間の変数にアクセスしない
	*/
  param.un.run_tsk.func = par->un.run_tsk.func;
  param.un.run_tsk.name = par->un.run_tsk.name;
  param.un.run_tsk.priority = par->un.run_tsk.priority;
  param.un.run_tsk.stacksize = par->un.run_tsk.stacksize;
  param.un.run_tsk.rate = par->un.run_tsk.rate;
  param.un.run_tsk.rel_exetim = par->un.run_tsk.rel_exetim;
	param.un.run_tsk.deadtim = par->un.run_tsk.deadtim;
	param.un.run_tsk.floatim = par->un.run_tsk.floatim;
  param.un.run_tsk.argc = par->un.run_tsk.argc;
  param.un.run_tsk.argv = par->un.run_tsk.argv;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_RUN_TSK, &param, (OBJP)(&(param.un.run_tsk.ret)));
	asm volatile ("swi #3");
	
	/* 割込み復帰後はここへもどってくる */

  return param.un.run_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_ext_tsk():自タスクの終了)
* リターンパラメータはあってはならない
*/
void mz_ext_tsk(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_EXT_TSK, NULL, WAIT_ERCD_NOCHANGE);
	asm volatile ("swi #4");

	/* 割込み復帰後はここへもどってこない */

}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_exd_tsk():自スレッドの終了と排除)
* リターンパラメータはあってはならない
*/
void mz_exd_tsk(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_EXD_TSK, NULL, WAIT_ERCD_NOCHANGE);
	asm volatile ("swi #5");

	/* 割込み復帰後はここへもどってこない */

}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_ter_tsk():スレッドの強制終了)
* tskid : 強制終了するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
ER mz_ter_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.ter_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_TER_TSK, &param, (OBJP)(&(param.un.ter_tsk.ret)));
	asm volatile ("swi #6");

	/* 割込み復帰後はここへもどってくる */

	return param.un.ter_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_get_pri():スレッドの優先度取得)
* tskid : 優先度を参照するタスクID
* *p_tskpri : 参照優先度を格納するポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_get_pri(ER_ID tskid, int *p_tskpri)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.get_pri.tskid = tskid;
	param.un.get_pri.p_tskpri = p_tskpri;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_GET_PRI, &param, (OBJP)(&(param.un.get_pri.ret)));
	asm volatile ("swi #7");

	/* 割込み復帰後はここへもどってくる */

	return param.un.get_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_chg_pri():スレッドの優先度変更)
* tskid : 優先度を変更するタスクID
* tskpri : 変更する優先度
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_chg_pri(ER_ID tskid, int tskpri)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.chg_pri.tskid = tskid;
	param.un.chg_pri.tskpri = tskpri;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_CHG_PRI, &param, (OBJP)(&(param.un.chg_pri.ret)));
	asm volatile ("swi #8");

	/* 割込み復帰後はここへもどってくる */

	return param.un.chg_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_slp_tsk():自タスクの起床待ち)
* (返却値)E_NOSPT : 未サポート
* (返却値)E_OK : 正常終了
*/
ER mz_slp_tsk(void)
{
	SYSCALL_PARAMCB param;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_SLP_TSK, &param, (OBJP)(&(param.un.slp_tsk.ret)));
	asm volatile ("swi #9");

	/* 割込み復帰後はここへもどってくる */

	return param.un.slp_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_wup_tsk():タスクの起床)
* tskid : タスクの起床するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
ER mz_wup_tsk(ER_ID tskid)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.wup_tsk.tskid = tskid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_WUP_TSK, &param, (OBJP)(&(param.un.wup_tsk.ret)));
	asm volatile ("swi #10");

	/* 割込み復帰後はここへもどってくる */

  return param.un.wup_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_rel_wai():待ち状態強制解除)
* tskid : 待ち状態強制解除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが待ち状態ではない
* (返却値)E_OK : 正常終了
*/
ER mz_rel_wai(ER_ID tskid)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.rel_wai.tskid = tskid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_REL_WAI, &param, (OBJP)(&(param.un.rel_wai.ret)));
	asm volatile ("swi #11");

	/* 割込み復帰後はここへもどってくる */

  return param.un.rel_wai.ret;
}


/* システムコール処理(get_mpf():動的メモリ獲得) */
void* mz_get_mpf(int size)
{
  SYSCALL_PARAMCB param;

  param.un.get_mpf.size = size;
  issue_trap_syscall(ISR_TYPE_GET_MPF, &param, (OBJP)(&(param.un.get_mpf.ret)));
	asm volatile ("swi #12");

  return param.un.get_mpf.ret;
}


/* システムコール処理(rel_mpf():動的メモリ解放) */
int mz_rel_mpf(void *p)
{
  SYSCALL_PARAMCB param;

  param.un.rel_mpf.p = p;
  issue_trap_syscall(ISR_TYPE_REL_MPF, &param, (OBJP)(&(param.un.rel_mpf.ret)));
	asm volatile ("swi #13");

  return param.un.rel_mpf.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_def_inh():割込みハンドラの定義)
* type : ソフトウェア割込みベクタ番号
* handler : 登録するハンドラポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
ER mz_def_inh(INTRPT_TYPE type, IR_HANDL handler)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.def_inh.type = type;
	param.un.def_inh.handler = handler;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEF_INH, &param, (OBJP)(&(param.un.def_inh.ret)));
	asm volatile ("swi #14");

	/* 割込み復帰後はここへもどってくる */

  return param.un.def_inh.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sel_schdul():スケジューラの切り替え)
* システムコールはタスクをスイッチするため，mz_sel_schdul()は認めていない
* (initロールバックシステムコールを発行しやすくするため)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_NOSPT : 未サポート
*/
ER mz_sel_schdul(SCHDUL_TYPE type, long param)
{
  return E_NOSPT;
}


/*
* interrput syscall
* 非タスクコンテキストから呼び出すシステムコール(タスクの切り替えは行わない)
*/
/*
* パラメータ類の退避割込みは使用しない(mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* acre_tsk()システムコールはパラメータ数が多いので,とりあえず構造体×共用体でやった(他のもやったほうがいいのかな～).
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : ユーザスタックが確保できない
* (返却値)tskid : 正常終了
*/
ER mz_iacre_tsk(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;
	/* iacre_tskの延長でget_tsk_readyque()が呼ばれるとg_currentが書き換えられるので一時退避 */
	TCB *tmptcb = g_current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = g_current->syscall_info.flag;
	
	/* パラメータ退避 */
  param.un.acre_tsk.func = par->un.acre_tsk.func;
  param.un.acre_tsk.name = par->un.acre_tsk.name;
  param.un.acre_tsk.priority = par->un.acre_tsk.priority;
  param.un.acre_tsk.stacksize = par->un.acre_tsk.stacksize;
  param.un.acre_tsk.rate = par->un.acre_tsk.rate;
  param.un.acre_tsk.rel_exetim = par->un.acre_tsk.rel_exetim;
	param.un.acre_tsk.deadtim = par->un.acre_tsk.deadtim;
	param.un.acre_tsk.floatim = par->un.acre_tsk.floatim;
  param.un.acre_tsk.argc = par->un.acre_tsk.argc;
  param.un.acre_tsk.argv = par->un.acre_tsk.argv;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_IACRE_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	g_current = tmptcb;
	g_current->syscall_info.flag = tmp_flag;

	return param.un.acre_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sta_tsk():スレッドの起動)
* tskid : 起動するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER mz_ista_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;
	/* ichg_priの延長でget_tsk_readyque()が呼ばれるとg_currentが書き換えられるので一時退避 */
	TCB *tmptcb = g_current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = g_current->syscall_info.flag;
	
	/* パラメータ退避 */
	param.un.sta_tsk.tskid = tskid;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_ISTA_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	g_current = tmptcb;
	g_current->syscall_info.flag = tmp_flag;

	return param.un.sta_tsk.ret;
}


/*!
* パラメータ類の退避割込みは使用しない(mz_ichg_pri():スレッドの優先度変更)
* tskid : 優先度を変更するタスクID
* tskpri : 変更する優先度
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_ichg_pri(ER_ID tskid, int tskpri)
{
  SYSCALL_PARAMCB param;
	/* ichg_priの延長でget_tsk_readyque()が呼ばれるとg_currentが書き換えられるので一時退避 */
	TCB *tmptcb = g_current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = g_current->syscall_info.flag;
 
	/* パラメータ退避 */
  param.un.chg_pri.tskid = tskid;
  param.un.chg_pri.tskpri = tskpri;
  /* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_CHG_PRI, &param);

	/* 実行状態タスクを前の状態へ戻す */
	g_current = tmptcb;
	g_current->syscall_info.flag = tmp_flag;
  
  return param.un.chg_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_iwup_tsk():タスクの起床)
* tskid : タスクの起床するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
ER mz_iwup_tsk(ER_ID tskid)
{
  SYSCALL_PARAMCB param;
	/* iwup_tskの延長でget_tsk_readyque()が呼ばれるとg_currentが書き換えられるので一時退避 */
	TCB *tmptcb = g_current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = g_current->syscall_info.flag;

	/* パラメータ退避 */
  param.un.wup_tsk.tskid = tskid;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_WUP_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	g_current = tmptcb;
	g_current->syscall_info.flag = tmp_flag;

  return param.un.wup_tsk.ret;
}
