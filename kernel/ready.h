/*!
 * @file ターゲット非依存部
 * @brief レディー状態管理インターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _READY_H_INCLUDED_
#define _READY_H_INCLUDED_


/* os/kernel */
#include "defines.h"
#include "kernel.h"


/*!
 * @brief レディーキュー型
 */
typedef struct _ready_queue_infomation {
  TCB *head; 																	/*! レディー先頭ポインタ */
  TCB *tail; 																	/*! レディー最尾ポインタ */
} RQUECB;


/*!
 * @brief レディーキュー型(優先度付き待ち行列)
 */
typedef struct _priority_ready_queue_infomation {
	UINT32 bitmap; 															/*! ビットマップ(検索高速化のため) */
 	RQUECB que[PRIORITY_NUM]; 									/*! 優先度レベルでキューを配列化 */
} PRIRQUECB;


#define OTHER_READY (1 << 0)									/*! 優先度とタイムアウトごとのレディーキュー以外にタスクを操作する */

/*!
 * @brief レディーキューコントロールブロック(レディー構造ごとにもっておく情報)
 * @note ビットパターンテーブルとビットマップはレディー構造に個別に置いておかなくて大丈夫だが，
 *       拡張性を考慮してつめておく
 */
typedef struct {
	union {
		/*!
		 * @brief 単一レベルのキュー
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
			RQUECB ready; 													/*! 単一レディーキュー */
		} single;
		/*!
		 * @brief 優先度ごとのキュー
		 * @attention unionはメモリ効率が良いが、エンディアンの関係上、移植には注意
		 */
		struct {
#define LSB_4BIT_PATTERN 16 									/*! LSBから下位4ビットのビットパターン */
#define BIT_SERCH_NUNBER 0x01 								/*! ビットサーチする番号(bitmap_init()で使用) */
			int lsb_4bits_table[LSB_4BIT_PATTERN]; 	/*! LSBから下位4ビットのパターンを記録する配列(メモリ効率のためPRIQUEには置いとかない) */
			PRIRQUECB ready; 												/*! 優先度レベルでキュー */
		} pri;
	} un;
} RDYCB;


/*!
 * @brief レディーキュー情報ブロック
 * @note 複数のレディーキュー構造をもっているので(単一のレディーキュー，優先度レディーキュー)，
 *       init(優先度0番)は別途異なるところへつないでおく
 *       (よって，優先度レディーキューの優先度レベルの配列インデックス0には何もつながれない)
 */
typedef struct _readyque_infomation {
	READY_TYPE type; 													/*! レディーキューのタイプ */
	TCB *init_que;														/*! initタスク専用領域 */
	RDYCB *entry; 														/*! 対象レディーキューブロックへのポインタ */
} READY_INFO;


/*! レディーキューの初期化 */
extern ER ready_init(void);

/*! カレントタスク(実行状態TCB)をどのタイプのレディーキューから抜き取るか分岐 */
extern ER getcurrent(void);

/*! カレントタスク(実行状態TCB)をどのタイプのレディーキューへつなげるか分岐 */
extern ER putcurrent(void);

/*! 指定されたTCBをレディーキューから抜き取る */
extern ER get_tsk_readyque(TCB *worktcb);


/*! レディーキュー情報 */
extern READY_INFO g_ready_info;


#endif
