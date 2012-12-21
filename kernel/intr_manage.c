/* os/kernel/ */
#include "intr_manage.h"
#include "kernel.h"
/* os/c_lib */
#include "c_lib/lib.h"
/* os/arch */
#include "arch/cpu/intr.h"


/*! 割込みハンドラ */
IR_HANDL g_exter_handlers[EXTERNAL_INTERRUPT_NUM] = {0};


/*!
 * システムコールの処理(def_inh():割込みハンドラの定義)
 * -属性はない．また登録できる割込みハンドラはシリアル割込みハンドラのみ
 * -ただし，initタスクだけはシステムコールハンドラ及びソフトウェアエラー(例外)，
 * -タイマ割込みハンドラ登録ができる．そのためE_ILUSEエラーを追加した
 * type : ソフトウェア割込みベクタ番号
 * handler : 登録するハンドラポインタ
 * (返却値)E_ILUSE : 不正使用
 * (返却値)E_PAR : パラメータエラー
 * (返却値)E_OK : 登録完了
 */
ER def_inh_isr(INTRPT_TYPE type, IR_HANDL handler)
{

	/* パラメータは正しいか */
	if (type < 0 || EXTERNAL_INTERRUPT_NUM <= type || handler == NULL) {
		return E_PAR;
	}
	/* initタスク生成時でのソフトウェア割込みベクタへ割込みハンドラ定義 */
	else if (g_tsk_info.id_table[INIT_TASK_ID] == NULL) {
  	g_exter_handlers[type] = handler;
		return E_OK;
	}
	/* initタスクでのソフトウェア割込みベクタへ割込みハンドラを定義 */
	else {
		/* シリアル割込みハンドラか */
		if (type == INTERRUPT_TYPE_UART3_IRQ) {
  		g_exter_handlers[type] = handler;
			return E_OK;
		}
		/* それ以外は不正使用 */
		else {
			return E_ILUSE;
		}
	}
}
