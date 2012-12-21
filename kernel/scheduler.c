/* os/kernel */
#include "scheduler.h"
#include "kernel.h"
#include "memory.h"
#include "task_manage.h"
#include "ready.h"
/* os/arch/cpu */
#include "arch/cpu/intr.h"


/*! スケジューラ情報メモリセグメントへ書き込み */
static ER write_schdul(SCHDUL_TYPE type, long param);

/*! スケジューラ情報の読み取り設定 */
static void read_schdul(SCHDULCB *schcb);

/*! First Come First Sarvedスケジューラ */
static void schedule_fcfs(void);

/*! 優先度ビットップを検索する */
static ER_VLE bit_serch(UINT32 bitmap);

/*! 優先度スケジューラ */
static void schedule_ps(void);

/*! RMスケジューラミスハンドラ(条件よりスケジュールできない) */
static void rmschedule_miss_handler(void);

/*! Rate Monotonic */
static void schedule_rms(void);

/*! DMスケジューラミスハンドラ(条件よりスケジュールできない) */
static void dmschedule_miss_handler(void);

/*! Deadline Monotonic */
static void schedule_dms(void);


/*! スケジューリング情報 */
SCHDUL_INFO g_schdul_info = {0, NULL};


/*!
* システムコール処理(sel_schdul():スケジューラの切り替え)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
ER sel_schdul_isr(SCHDUL_TYPE type, long param)
{
	/* initタスク生成ルーチンに戻すため，スケジューラ情報メモリセグメントへ書き込み */
	write_schdul(type, param); /* スケジューラ情報メモリセグメントへ書き込み */

	return E_OK;
}


/*!
* スケジューラ情報メモリセグメントへ書き込み
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
static ER write_schdul(SCHDUL_TYPE type, long param)
{
	extern char _schdul_area;
	char *schdul_info = &_schdul_area;
	UINT32 *p = (UINT32 *)schdul_info;

	*(--p) = type;	/* スケジューラのタイプを退避 */
	
	/* FCFSスケジューリング */
	if (type == FCFS_SCHEDULING) {
		*(--p) = (UINT32)-1;
		*(--p) = (UINT32)schedule_fcfs; /* スケジューラコントロールブロックの設定 */
	}
	
	/* 優先度スケジューリング */
	else if (type == PRI_SCHEDULING) {
		*(--p) = (UINT32)-1;
		*(--p) = (UINT32)schedule_ps; /* スケジューラコントロールブロックの設定 */
	}
	
	/* Rate Monotonic */
	else if (type == RM_SCHEDULING) {
		*(--p) = (UINT32)-1;
		*(--p) = (UINT32)schedule_rms; /* スケジューラコントロールブロックの設定 */
	}
	
	/* Deadline Monotonic */
	else if (type == DM_SCHEDULING) {
		*(--p) = (UINT32)-1;
		*(--p) = (UINT32)schedule_dms; /* スケジューラコントロールブロックの設定 */
	}

	return E_OK;
}


/*!
* スケジューラ情報を読み取り設定
* *schcb : 設定するスケジューラコントロールブロック
*/
static void read_schdul(SCHDULCB *schcb)
{
	SCHDUL_TYPE type;
	extern char _schdul_area;
	char *schdul_info = &_schdul_area;
	UINT32 *p = (UINT32 *)schdul_info;
	UINT32 dummy;
	
	g_schdul_info.type = type = (SCHDUL_TYPE)*(--p);	/* スケジューラタイプを復旧 */;
	
	/* FCFSスケジューリング */
	if (type == FCFS_SCHEDULING) {
		dummy = *(--p);
		schcb->un.fcfs_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* 優先度スケジューリング */
	else if (type == PRI_SCHEDULING) {
		dummy = *(--p);
		schcb->un.ps_schdul.rte = (void(*)(void))(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* Rate Monotonic */
	else if (type == RM_SCHEDULING) {
		dummy = *(--p);
		schcb->un.rms_schdul.unroll_rate = 0;
		schcb->un.rms_schdul.unroll_exetim = 0;
		schcb->un.rms_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* Deadline Monotonic */
	else if (type == DM_SCHEDULING) {
		dummy = *(--p);
		schcb->un.dms_schdul.unroll_dead = 0;
		schcb->un.dms_schdul.unroll_exetim = 0;
		schcb->un.dms_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}

	g_schdul_info.entry = schcb;
}


/*!
* スケジューラの初期化
* -typeはenumでやっているので，パラメータチェックはいらない
* type : スケジューラのタイプ
* *exinf : スケジューラに渡すパラメータ
* (返却値)E_NOMEM : メモリ不足
* (返却値)E_OK : 正常終了
*/
ER schdul_init(void)
{
	SCHDULCB *schcb;
	
	schcb = (SCHDULCB *)get_mpf_isr(sizeof(*schcb)); /* 動的メモリ取得 */
	if (schcb == NULL) {
		return E_NOMEM;
	}
	memset(schcb, 0, sizeof(*schcb));
	
	read_schdul(schcb); /* スケジューラ情報の読み込み */

	return E_OK;
}


/*!
* 有効化されているスケジューラは分岐する
* -typeはenumでやっているので，パラメータチェックはいらない
*/
void schedule(void)
{
	/* 登録しておいたスケジューラを関数ポインタで呼ぶ */
	/* FCFSスケジューリング */
	if (g_schdul_info.type == FCFS_SCHEDULING) {
		(*g_schdul_info.entry->un.fcfs_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* 優先度スケジューリング */
	else if (g_schdul_info.type == PRI_SCHEDULING) {
		(*g_schdul_info.entry->un.ps_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Rate Monotonic */
	else if (g_schdul_info.type == RM_SCHEDULING) {
		(*g_schdul_info.entry->un.rms_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Deadline Monotonic */
	else if (g_schdul_info.type == DM_SCHEDULING) {
		(*g_schdul_info.entry->un.dms_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
}


/*!
* First Come First Sarvedスケジューラ
* -優先度はなく，到着順にスケジューリングする
* p : FCFCスケジューラ使用時のレディーキューのポインタ
*/
static void schedule_fcfs(void)
{
	RQUECB *p = &g_ready_info.entry->un.single.ready;
	TCB **p_ique = &g_ready_info.init_que;

	/* 単一のレディーキューの先頭は存在するか */
	if (!p->head) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			g_current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			KERNEL_OUTMSG("error: schdule_fcfs() \n");
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* 単一のレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
		g_current = p->head;
	}
}


/*!
* 優先度ビットップを検索する
* bitmap : 検索するスケジューラのビットマップ
* (返却値)E_NG : ビットがない(この関数呼出側でOSをスリープさせる)
* priority : 立っているビット(優先度となる)
*/
static ER_VLE bit_serch(UINT32 bitmap)
{
#if PRIORITY_NUM > 32
#error ビットマップを配列化する
#endif
	ER_VLE priority = 0; /* 下の処理で変化する */

	/*
	* ビットサーチアルゴリズム(検索の高速化)
	* ビットサーチ命令を持っていないので適用した
	* 32ビット変数を半分づつシフトし，下位4ビットを残し，削り取っていく方法でO(logn)となる
	* 下位4ビットのビットパターンは配列としてlsb_4bits_table[]で持っておく
	*/
	if (!(bitmap & 0xffff)) {
		bitmap >>= (PRIORITY_NUM / 2);
		priority += (PRIORITY_NUM / 2);
	}
	if (!(bitmap & 0xff)) {
		bitmap >>= (PRIORITY_NUM / 4);
		priority += (PRIORITY_NUM / 4);
	}
	if (!(bitmap & 0xf)) {
		bitmap >>= (PRIORITY_NUM / 8);
		priority += (PRIORITY_NUM / 8);
	}

	/* 下位4ビット分 */
	priority += g_ready_info.entry->un.pri.lsb_4bits_table[bitmap & 0xf];
		
	/* ビットが立っていないならば */
	if (priority < 0) {
		return E_NG;
	}
	/* ビットが立っているならば */
	else {
		return priority;
	}
}


/*!
* 優先度スケジューラ
* -優先度によって高いものから順にスケジューリングする
*/
static void schedule_ps(void)
{
	ER_VLE priority;
	TCB **p = &g_ready_info.init_que;

  priority = bit_serch(g_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			g_current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			KERNEL_OUTMSG("error: schdule_ps() \n");
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	g_current = g_ready_info.entry->un.pri.ready.que[priority].head;
  }
}


/*!
* Rate Monotonic，Deadline Monotonic展開スケジューリングをするための関数
*** もっと効率よく実装できると思う***
* ・ユーグリッド互除法を改良したバージョン
* ・create()システムコール(task_manage.cのacre_isr())で呼ばれ,create()されたタスクまでの周期の最小公倍数,
*   周期の最小公倍数に沿った最大実行時間を求めて,スケジューラコントロールブロックへセットする
* len : タスク一つあたりの周期またはデッドライン
* exetim : タスク一つあたりの仮想実行時間
*/
void set_unrolled_schedule_val(int len, int exetim)
{
	SCHDUL_TYPE type = g_schdul_info.type;
	int work_len;
	int work_exetim;
	int tmp_len; /* 現在の総合周期 */
	int rest; /* 余り */
	int quo; /* 商 */
	int now_len = len; /* 退避用(現在の周期) */
	
	if (type == RM_SCHEDULING) {
		tmp_len = work_len = g_schdul_info.entry->un.rms_schdul.unroll_rate;
		work_exetim = g_schdul_info.entry->un.rms_schdul.unroll_exetim;
	}
	else {
		tmp_len = work_len = g_schdul_info.entry->un.dms_schdul.unroll_dead;
		work_exetim = g_schdul_info.entry->un.dms_schdul.unroll_exetim;
	}
	if (!len) {
	  DEBUG_LEVEL1_OUTMSG(" not division : set_unrolled_schdule_val().\n");
	  return;
	}
	
	/* すでに周期の最小公倍数と最大実行時間がセットされている場合 */			
	if (work_len && work_exetim) {
		/* ユーグリッド互除法改良ver */
		while ((rest = work_len % len) != 0) {
			work_len = len;
			len = rest;
		}
		/* 周期最小公倍数の計算(スケジューラコントロールブロックへセット) */
		if (type == RM_SCHEDULING) {
			g_schdul_info.entry->un.rms_schdul.unroll_rate = work_len = tmp_len * now_len / len;
		}
		else {
			g_schdul_info.entry->un.dms_schdul.unroll_dead = work_len = tmp_len * now_len / len;
		}
		/* 周期に沿った最大実行時間の計算 */
		quo = work_len / tmp_len; /* 乗算する値の決定 */
		work_exetim *= quo; /* すでにセットされているスケジューラコントロールブロックの実行時間メンバの乗算 */
		/* 現在の分を加算(16ビット同士のmsecあたりの乗算となるのでオーバーフローしないように注意) */
		work_exetim += (exetim * (work_len / now_len));
		if (type == RM_SCHEDULING) {
			g_schdul_info.entry->un.rms_schdul.unroll_exetim = work_exetim; /* スケジューラコントロールブロックへセット */
		}
		else {
			g_schdul_info.entry->un.dms_schdul.unroll_exetim = work_exetim; /* スケジューラコントロールブロックへセット */
		}
	}
	/* 初期セット */
	else {
		if (type == RM_SCHEDULING) {
			g_schdul_info.entry->un.rms_schdul.unroll_rate = len;
			g_schdul_info.entry->un.rms_schdul.unroll_exetim = exetim;
		}
		else {
			g_schdul_info.entry->un.dms_schdul.unroll_dead = len;
			g_schdul_info.entry->un.dms_schdul.unroll_exetim = exetim;
		}
	}
}


/*!
* スケジューラのデッドラインミスハンドラ(条件よりスケジュールできない)
* -Rate Monotonicに展開スケジューリング専用
*/
static void rmschedule_miss_handler(void)
{
	KERNEL_OUTMSG(" Rate Monotonic Deadline Miss task set\n");
	KERNEL_OUTMSG(" OS sleep... Please push reset button\n");
	down_system(); /* kernelのフリーズ */
	
}


/*!
* Rate Monotonic Schduler
* -周期的タスクセットに対して起動周期の短いタスクの順にスケジューリングする
* -OS実装レベルでは周期順に優先度スケジューリングを行えばよい
*/
static void schedule_rms(void)
{
	ER_VLE priority;
	TCB **p = &g_ready_info.init_que;
	
	/* Deadline(周期と同じ)ミスをしないかチェック */
  if (g_schdul_info.entry->un.rms_schdul.unroll_rate < g_schdul_info.entry->un.rms_schdul.unroll_exetim) {
		rmschedule_miss_handler(); /* Deadlineミスハンドラを呼ぶ */
	}
	
 	priority = bit_serch(g_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			g_current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			KERNEL_OUTMSG("error: schdule_rms() \n");
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	g_current = g_ready_info.entry->un.pri.ready.que[priority].head;
  }
}


/*!
* スケジューラのデッドラインミスハンドラ(条件よりスケジュールできない)
* Rate Monotonicに展開スケジューリング専用
*/
static void dmschedule_miss_handler(void)
{
	KERNEL_OUTMSG(" Deadline Monotonic Deadline Miss task set\n");
	KERNEL_OUTMSG(" OS sleep... Please push reset button\n");
	down_system(); /* kernelのフリーズ */
	
}


/*!
* Deadline Monotonic Scheduler
* -周期タスクセットに対してデッドラインの短い順にスケジューリングする
* -OS実装レベルではデッドライン順に優先度スケジューリングを行えばよい
*/
static void schedule_dms(void)
{
	ER_VLE priority;
	TCB **p = &g_ready_info.init_que;
	
	/* Deadline(周期と異なる)ミスをしないかチェック */
  if (g_schdul_info.entry->un.dms_schdul.unroll_dead < g_schdul_info.entry->un.dms_schdul.unroll_exetim) {
		dmschedule_miss_handler(); /* Deadlineミスハンドラを呼ぶ */
	}
	
 	priority = bit_serch(g_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			g_current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			KERNEL_OUTMSG("error: schdule_dms() \n");
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	g_current = g_ready_info.entry->un.pri.ready.que[priority].head;
  }
}
