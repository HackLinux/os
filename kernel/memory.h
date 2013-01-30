/*!
 * @file ターゲット非依存部
 * @brief heap管理(動的メモリ)インターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _MEMORY_H_INCLUDED_
#define _MEMORY_H_INCLUDED_


/*! 動的メモリの初期化 */
extern void mem_init(void);

/*! 動的メモリの獲得 */
extern void* get_mpf_isr(int size);

/*! メモリの解放 */
extern void rel_mpf_isr(void *mem);


#endif
