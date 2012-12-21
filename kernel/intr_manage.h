#ifndef _INTR_MANAGE_H_INCLUDED_
#define _INTR_MANAGE_H_INCLUDED_

/* os/kernel */
#include "defines.h"


/*! システムコールの処理(def_inh():割込みハンドラの定義) */
extern ER def_inh_isr(INTRPT_TYPE type, IR_HANDL handler);

/*! 割込みハンドラ */
extern IR_HANDL g_exter_handlers[EXTERNAL_INTERRUPT_NUM];


#endif
