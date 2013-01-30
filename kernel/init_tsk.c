/*!
 * @file ターゲット非依存部<モジュール:init_tsk.o>
 * @brief kernelのinit後、起動するタスク(デーモン)
 * @attention gcc4.5.x以外は試していない
 */


/* os/kernel */
#include "defines.h"
#include "kernel.h"
#include "multi_timer.h"
/* os/arch/cpu */
#include "arch/cpu/intr_cntrl.h"
/* os/c_lib */
#include "c_lib/lib.h"
/* os/target */
#include "target/driver/serial_driver.h"


extern void uart_handler(void);


/*!
 * @brief initタスク
 * @param[in] argc:はargc
 *	@arg 特に関係なし
 * @param[out] *argv[]:は*argv[]
 *	@arg 特に県警なし
 * @return 終値
 *	@retval 0:正常終了
 */
int start_threads(int argc, char *argv[])
{
  KERNEL_OUTMSG("init task started.\n");
  intc_enable_irq(INTERRUPT_TYPE_UART3_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT1_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT2_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT3_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT4_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT5_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT6_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT7_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT8_IRQ); /* MIRの有効化 */
	intc_enable_irq(INTERRUPT_TYPE_GPT9_IRQ); /* MIRの有効化 */
  serial_intr_recv_enable(); 								/* シリアル受信割込み有効化 */
  serial_intr_send_disable();

  mz_def_inh(INTERRUPT_TYPE_UART3_IRQ, uart_handler); /* 割込みハンドラの登録 */

	/* 外部割込み有効化(CPSR) */
  enable_irq();

	while (1) {
		;
	}

  return 0;
}

