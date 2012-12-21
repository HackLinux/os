/* os/kernel */
#include "ready.h"
#include "kernel.h"
#include "memory.h"
#include "scheduler.h"
/* os/c_lib */
#include "c_lib/lib.h"


/*! キュー構造のレディーブロックの初期化 */
static void rquecb_init(RQUECB *rcb, int len);

/*! 下位4ビットのパターンを記録するlsb_4bits_tableの初期化 */
static void bitmap_init(void);

/*! カレントタスク(実行状態TCB)を単一のレディーキュー先頭または，レディーキュー情報ブロックinit_queから抜き出す */
static ER get_current_singleque(void);

/*! カレントタスク(実行状態TCB)を単一のレディーキューの末尾または，レディーキュー情報ブロックinit_queへ繋げる */
static ER put_current_singleque(void);

/*! 指定されたTCBを単一のレディーキューまたは，レディーキュー情報ブロックinit_queから抜き取る */
static void get_tsk_singleque(TCB *worktcb);

/*! カレントタスク(実行状態TCB)を優先度レベルのレディーキュー先頭または，レディーキュー情報ブロックinit_queから抜き出す */
static ER get_current_prique(void);

/*! カレントタスク(実行状態TCB)を優先度レベルのレディーキューの末尾または，レディーキュー情報ブロックinit_queへ繋げる */
static ER put_current_prique(void);

/*! 指定されたTCBを優先度レベルのレディーキューまたは，レディーキュー情報ブロックinit_queから抜き取る */
static void get_tsk_prique(TCB *worktcb);


/*! レディーキュー情報 */
READY_INFO g_ready_info = {0, NULL, NULL};


/*!
* キュー構造のレディーブロックの初期化
* *rcb : 初期化するキュー構造のレディーブロックポインタ
* len : 長さ(配列の場合使用)
*/
static void rquecb_init(RQUECB *rcb, int len)
{
	int i;
	
	for (i = 0; i <= len; i++) {
		(rcb + i)->head = (rcb + i)->tail = NULL;
	}
}


/*!
* レディーキューの初期化
* -typeはenumでやっているので，パラメータチェックはいらない
* type : スケジューラのタイプ
* (返却値)E_NOMEM : メモリ不足
* (返却値)E_OK : 正常終了
*/
ER ready_init(void)
{
	RDYCB *rqcb;
	SCHDUL_TYPE type = g_schdul_info.type;
	
	rqcb = (RDYCB *)get_mpf_isr(sizeof(*rqcb)); /* 動的メモリ取得 */
	if (rqcb == NULL) {
		return E_NOMEM;
	}
	memset(rqcb, 0, sizeof(*rqcb));
	
	g_ready_info.entry = rqcb; /* レディー情報ブロックの設定 */
	
	/* First Come First Sarvedとラウンドロビンスケジューリングの時は単一のレディーキュー */
	if (type == FCFS_SCHEDULING) {
		g_ready_info.type = SINGLE_READY_QUEUE;
		/* キュー構造のレディーブロックのポインタ */
		rquecb_init(&g_ready_info.entry->un.single.ready, 0);
		/* ビットマップの処理はない */
	}
	else {
		g_ready_info.type = PRIORITY_READY_QUEUE;
		/* キュー構造のレディーブロックのポインタ */
		rquecb_init(g_ready_info.entry->un.pri.ready.que, PRIORITY_NUM);
		bitmap_init(); /*! ビットマップの初期化 */
	}
	
	return E_OK;
}


/*!
* 下位4ビットのパターンを記録するlsb_4bits_tableの初期化
* -優先度レベルごとにレディーキューを持つスケジューリング法のみ初期化をする
*/
static void bitmap_init(void)
{
	int i, *p;
	READY_TYPE type = g_ready_info.type;

	/* 優先度レベルごとのレディーキューか */
	if (type == PRIORITY_READY_QUEUE) {
		p = g_ready_info.entry->un.pri.lsb_4bits_table;
	}
	/* 以外 */
	else {
		return;
	}

	*p = -PRIORITY_NUM; /* エラー処理として使用 */

	/* ここから初期化処理 */
	for (i = 1; i < LSB_4BIT_PATTERN; i++) {
		if (i & BIT_SERCH_NUNBER) {
			*(p + i) = 0;
		}
		else if (i & BIT_SERCH_NUNBER << 1) {
			*(p + i) = 1;
		}
		else if (i & BIT_SERCH_NUNBER << 2) {
			*(p + i) = 2;
		}
		else {
			*(p + i) = 3;
		}
	}
}


/*!
* カレントタスク(実行状態TCB)をどのタイプのレディーキューから抜き取るか分岐
* typeはenumでやっているので，パラメータチェックはいらない
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(レディーに存在しない．つまり対象タスクが実行状態ではない)
* (返却値)E_OK : 正常終了
*/
ER getcurrent(void)
{
	READY_TYPE type = g_ready_info.type;
	
	/* process_init_tsk()の呼び出し時のみ評価される */
	if (g_current == NULL) {
		return E_ILUSE;
	}
	/* First Come First Sarved */
	else if (type == SINGLE_READY_QUEUE) {
		return get_current_singleque();
	}
	/*
	* 優先度スケジューリング
	* Rate Monotonic,Deadline Monotonic
	*/
	else {
		return get_current_prique();
	}
}

/*!
* カレントタスク(実行状態TCB)をどのタイプのレディーキューへつなげるか分岐
* -typeはenumでやっているので，パラメータチェックはいらない
* -flag : どのレディーキューか(※タイムアウトと優先度レベルのレディーキューをもっているものにしか適用しない)
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(すでにレディーに存在している．つまり対象タスクが実行状態となっている)
* (返却値)E_OK : 正常終了
*/
ER putcurrent(void)
{
	READY_TYPE type = g_ready_info.type;
	
	/* process_init_tsk()の呼び出し時のみ評価される */
	if (g_current == NULL) {
		return E_ILUSE;
	}
	/* First Come First Sarved */
	else if (type == SINGLE_READY_QUEUE) {
		return put_current_singleque();
	}
	/*
	* 優先度スケジューリングとラウンドロビン×優先度スケジューリング，Muletilevel Feedback Queue,
	* Rate Monotonic,Deadline Monotonic
	*/
	else {
		return put_current_prique();
	}
}


/*!
* 指定されたTCBをどのタイプのレディーキューから抜き取るか分岐
* -typeはenumでやっているので，パラメータチェックはいらない
* -抜き取られた後はg_currentに設定される
* -O(1)スケジューリングの時はactivキューかexpiredキューかのtypeを求める(※優先度とタイムアウトキューをもっている場合のみに使用)
* worktcb : 抜き取るTCB
* (返却値)E_OK : 正常終了
* (返却値)E_PAR : パラメータエラー
*/
ER get_tsk_readyque(TCB *worktcb)
{
	READY_TYPE type = g_ready_info.type;

	/* First Come First Sarved */
	if (type == SINGLE_READY_QUEUE) {
		get_tsk_singleque(worktcb);
		return E_OK;
	}
	/*
	* 優先度スケジューリング
	* Rate Monotonic,Deadline Monotonic
	*/
	else {
		get_tsk_prique(worktcb);
		return E_OK;
	}
}


/*!
* カレントタスク(実行状態TCB)を単一のレディーキュー先頭または，レディーキュー情報ブロックinit_queから抜き出す
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(レディーに存在しない．つまり対象タスクが実行状態ではない)
* (返却値)E_OK : 正常終了
*/
static ER get_current_singleque(void)
{
	TCB **next = &g_current->ready_info.un.que_ready.ready_next;
	TCB **prev = &g_current->ready_info.un.que_ready.ready_prev;
	RQUECB *p = &g_ready_info.entry->un.single.ready;

	/* 非タスクコンテキスト用のシステムコールか，またはサービスコールか */
  if ((g_current->syscall_info.flag != MZ_VOID) && (g_current->syscall_info.flag != MZ_SYSCALL)) {
    return E_ILUSE;
  }
  /* すでにレディーキューに存在するTCBか */
  else if (!(g_current->state & TASK_READY)) {
    /* すでに無い場合は無視 */
    return E_OBJ;
  }
  /* initタスクの場合(レディーキュー情報ブロックのinit_queから抜く) */
	else if (g_current->init.tskid == INIT_TASK_ID) {
		g_ready_info.init_que = NULL;
		g_current->state &= ~TASK_READY;
		return E_OK;
	}
  /* レディキューから抜き取り */
	else {
  	/* カレントスレッドは必ず先頭にあるはずなので，先頭から抜き出す */
  	p->head = *next;
  	p->head->ready_info.un.que_ready.ready_prev = NULL;
		/* レディーキュータスクがない場合 */
  	if (p->head == NULL) {
    	p->tail = NULL;
			/* ビットマップの処理はない */
  	}
  	g_current->state &= ~TASK_READY;
  	*next = *prev = NULL;
  	return E_OK;
  }
}


/*!
* カレントタスク(実行状態TCB)を単一のレディーキューの末尾または，レディーキュー情報ブロックinit_queへ繋げる
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(すでにレディーに存在している．つまり対象タスクが実行状態となっている)
* (返却値)E_OK : 正常終了
*/
static ER put_current_singleque(void)
{
	RQUECB *p = &g_ready_info.entry->un.single.ready;

	/* 非タスクコンテキスト用のシステムコールか，またはサービスコールか */
  if ((g_current->syscall_info.flag != MZ_VOID) && (g_current->syscall_info.flag != MZ_SYSCALL)) {
    return E_ILUSE;
  }
  /* すでにレディーキューに存在するTCBか */
  else if (g_current->state & TASK_READY) {
    /*すでにある場合は無視*/
    return E_OBJ;
  }
  /* initタスクの場合(レディーキュー情報ブロックのinit_queへつなぐ) */
	else if (g_current->init.tskid == INIT_TASK_ID) {
		g_ready_info.init_que = g_current;
		g_current->state |= TASK_READY;
		return E_OK;
	}
  /* レディーキュー末尾に繋げる */
	else {
  	if (p->tail) {
  		g_current->ready_info.un.que_ready.ready_prev = p->tail;
    	p->tail->ready_info.un.que_ready.ready_next = g_current;
  	}
  	else {
    	p->head = g_current;
  	}
  	p->tail = g_current;

		/* ビットマップ処理はいらない */
  	g_current->state |= TASK_READY;

		return E_OK;
	}
}


/*!
* 指定されたTCBを単一のレディーキューまたは，レディーキュー情報ブロックinit_queから抜き取る
* -単一のレディーキューの場合，chg_pri()からは呼ばれないが(この場合はE_NOSPT)，
* -ter_tsk()などからも呼ばれるためこの関数を追加した
* -抜き取られた後はg_currentに設定される
* worktcb : 抜き取るTCB
*/
static void get_tsk_singleque(TCB *worktcb)
{
	TCB **next = &worktcb->ready_info.un.que_ready.ready_next;
	TCB **prev = &worktcb->ready_info.un.que_ready.ready_prev;
	RQUECB *p = &g_ready_info.entry->un.single.ready;

	/* すでにレディーに存在しないTCBか */
  if (!(worktcb->state & TASK_READY)) {
    /* すでに無い場合は無視 */
    return;
  }
	/* レディーキューの先頭を抜き取る */
	else if (worktcb == p->head) {
		p->head = *next;
		p->head->ready_info.un.que_ready.ready_prev = NULL;
		/* レディーにタスクが一つの場合 */
		if (worktcb == p->tail) {
			/* ビットマップ処理はいらない */
			p->tail = NULL;
		}
	}
	/* レディーキューの最後から抜き取る */
	else if (worktcb == p->tail) {
		p->tail = *prev;
		(*prev)->ready_info.un.que_ready.ready_next = NULL;
	}
	/* レディーキューの中間から抜き取る */
	else {
		(*prev)->ready_info.un.que_ready.ready_next = *next;
		(*next)->ready_info.un.que_ready.ready_prev = *prev;
	}
	g_current = worktcb; /* 優先度変更するタスクはそれぞれの待ち行列の後に挿入するのでg_currentにしておく */
  *next = *prev = NULL;
  g_current->state &= ~TASK_READY; /* スレッドの状態をスリープ状態にしておく */
}


/*!
* カレントタスク(実行状態TCB)を優先度レベルのレディーキュー先頭または，レディーキュー情報ブロックinit_queから抜き出す
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(レディーに存在しない．つまり対象タスクが実行状態ではない)
* (返却値)E_OK : 正常終了
*/
static ER get_current_prique(void)
{
	TCB **next = &g_current->ready_info.un.que_ready.ready_next;
	TCB **prev = &g_current->ready_info.un.que_ready.ready_prev;
	RQUECB *p = &g_ready_info.entry->un.pri.ready.que[g_current->priority];

	/* 非タスクコンテキスト用のシステムコールか，またはサービスコールか */
  if ((g_current->syscall_info.flag != MZ_VOID) && (g_current->syscall_info.flag != MZ_SYSCALL)) {
    return E_ILUSE;
  }
  /* すでにレディーキューに存在しないTCBか */
  else if (!(g_current->state & TASK_READY)) {
    /* すでに無い場合は無視 */
    return E_OBJ;
  }
  /* initタスクの場合(レディーキュー情報ブロックのinit_queから抜く) */
	else if (g_current->init.tskid == INIT_TASK_ID) {
		g_ready_info.init_que = NULL;
		g_current->state &= ~TASK_READY;
		return E_OK;
	}
  /* レディキューから抜き取り */
	else {
  	/* カレントスレッドは必ず先頭にあるはずなので，先頭から抜き出す */
  	p->head = *next;
		/* NULLチェック(NULLポインタを経由してデータを参照しようとするとデータアボートとなる) */
		if (p->head != NULL) {
  		p->head->ready_info.un.que_ready.ready_prev = NULL;
		}
		/* レディーキュータスクがない場合 */
  	else {
    	p->tail = NULL;
			/* ビットマップの対象ビット(優先度ビット)を落とす */
			g_ready_info.entry->un.pri.ready.bitmap &= ~(1 << g_current->priority);
  	}
  	g_current->state &= ~TASK_READY;
  	*next = *prev = NULL;
  	return E_OK;
  }
}


/*!
* カレントタスク(実行状態TCB)を優先度レベルのレディーキューの末尾または，レディーキュー情報ブロックinit_queへ繋げる
* (返却値)E_ILUSE : 不正使用(サービスコールの時はE_ILUSEが返却される)
* (返却値)E_OBJ : オブジェクトエラー(すでにレディーに存在している．つまり対象タスクが実行状態となっている)
* (返却値)E_OK : 正常終了
*/
static ER put_current_prique(void)
{
	RQUECB *p = &g_ready_info.entry->un.pri.ready.que[g_current->priority];
	
	/* 非タスクコンテキスト用のシステムコールか，またはサービスコールか */
  if ((g_current->syscall_info.flag != MZ_VOID) && (g_current->syscall_info.flag != MZ_SYSCALL)) {
    return E_ILUSE;
  }
  /* すでにレディーキューに存在するTCBか */
  else if (g_current->state & TASK_READY) {
    /*すでにある場合は無視*/
    return E_OBJ;
  }
  /* initタスクの場合(レディーキュー情報ブロックのinit_queへつなぐ) */
	else if (g_current->init.tskid == INIT_TASK_ID) {
		g_ready_info.init_que = g_current;
		g_current->state |= TASK_READY;
		return E_OK;
	}
  /* レディーキュー末尾に繋げる */
	else {
  	if (p->tail) {
  		g_current->ready_info.un.que_ready.ready_prev = p->tail;
    	p->tail->ready_info.un.que_ready.ready_next = g_current;
  	}
  	else {
    	p->head = g_current;
  	}
  	p->tail = g_current;
		/* ビットマップの対象ビット(優先度ビット)をセット */
		g_ready_info.entry->un.pri.ready.bitmap |= (1 << g_current->priority);
  	g_current->state |= TASK_READY;
		return E_OK;
	}
}


/*!
* 指定されたTCBを優先度レベルのレディーキューまたは，レディーキュー情報ブロックinit_queから抜き取る
* -抜き取られた後はg_currentに設定される
* worktcb : 抜き取るTCB
* p : 抜き取る対象であるレディーキューのポインタ
*/
static void get_tsk_prique(TCB *worktcb)
{
	TCB **next = &worktcb->ready_info.un.que_ready.ready_next;
	TCB **prev = &worktcb->ready_info.un.que_ready.ready_prev;
	RQUECB *p = &g_ready_info.entry->un.pri.ready.que[worktcb->priority];

	/* すでにレディーに存在しないTCBか */
  if (!(worktcb->state & TASK_READY)) {
    /* すでに無い場合は無視 */
    return;
  }
	/* レディーキューの先頭を抜き取る */
	else if (worktcb == p->head) {
		p->head = *next;
		p->head->ready_info.un.que_ready.ready_prev = NULL;
		/* レディーにタスクが一つの場合 */
		if (worktcb == p->tail) {
			/* ビットマップの対象ビット(優先度ビット)を落とす */
			g_ready_info.entry->un.pri.ready.bitmap &= ~(1 << worktcb->priority);
			p->tail = NULL;
		}
	}
	/* レディーキューの最後から抜き取る */
	else if (worktcb == p->tail) {
		p->tail = *prev;
		(*prev)->ready_info.un.que_ready.ready_next = NULL;
	}
	/* レディーキューの中間から抜き取る */
	else {
		(*prev)->ready_info.un.que_ready.ready_next = *next;
		(*next)->ready_info.un.que_ready.ready_prev = *prev;
	}
	g_current = worktcb; /* 優先度変更するタスクはそれぞれの待ち行列の後に挿入するのでg_currentにしておく */
  *next = *prev = NULL;
  g_current->state &= ~TASK_READY; /* スレッドの状態をスリープ状態にしておく */
}
