/*!
 * @file ターゲット非依存部
 * @brief 割込み管理機能インターフェース
 * @attention gcc4.5.x以外は試していない
 * @note μITRON4.0仕様参考
 */


#ifndef _INTR_MANAGE_H_INCLUDED_
#define _INTR_MANAGE_H_INCLUDED_

/* os/kernel */
#include "defines.h"


/*! システムコールの処理(def_inh():割込みハンドラの定義) */
extern ER def_inh_isr(INTRPT_TYPE type, IR_HANDL handler);

/*! 割込みハンドラ */
extern IR_HANDL g_exter_handlers[EXTERNAL_INTERRUPT_NUM];


#endif
