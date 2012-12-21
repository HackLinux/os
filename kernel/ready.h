#ifndef _READY_H_INCLUDED_
#define _READY_H_INCLUDED_


/* os/kernel */
#include "defines.h"
#include "kernel.h"


/*! レディーキュー型 */
typedef struct _ready_queue_infomation {
  TCB *head; 																	/*! レディー先頭ポインタ */
  TCB *tail; 																	/*! レディー最尾ポインタ */
} RQUECB;


/*! レディーキュー型(優先度付き待ち行列) */
typedef struct _priority_ready_queue_infomation {
	UINT32 bitmap; 															/*! ビットマップ(検索高速化のため) */
 	RQUECB que[PRIORITY_NUM]; 									/*! 優先度レベルでキューを配列化 */
} PRIRQUECB;


#define OTHER_READY (1 << 0)									/*! 優先度とタイムアウトごとのレディーキュー以外にタスクを操作する */
/*! レディーキューコントロールブロック */
/*
* ～レディーキューごとにもっておく情報～
* -ビットパターンテーブルとビットマップはレディー構造に個別に置いておかなくて大丈夫だが，
*  見やすくするのと，拡張性を考慮してつめておく
*/
typedef struct {
	union {
		/*! 単一レベルのキュー */
		struct {
			RQUECB ready; 													/*! 単一レディーキュー */
		} single;
		/*! 優先度ごとのキュー */
		struct {
#define LSB_4BIT_PATTERN 16 									/*! LSBから下位4ビットのビットパターン */
#define BIT_SERCH_NUNBER 0x01 								/*! ビットサーチする番号(bitmap_init()で使用) */
			int lsb_4bits_table[LSB_4BIT_PATTERN]; 	/*! LSBから下位4ビットのパターンを記録する配列(メモリ効率のためPRIQUEには置いとかない) */
			PRIRQUECB ready; 												/*! 優先度レベルでキュー */
		} pri;
	} un;
} RDYCB;


/*! レディーキュー情報ブロック */
/*
 * ～レディーキューで共通にもっていおく情報～
 * -複数のレディーキュー構造をもっているので(単一のレディーキュー，優先度レディーキュー)，
 *  init(優先度0番)は別途異なるところへつないでおく
 *  (よって，優先度レディーキューの優先度レベルの配列インデックス0には何もつながれない)
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
