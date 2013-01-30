/*!
 * @file ターゲット依存部(ARM-Cortex-A8)<モジュール:intr_cntrl.o>
 * @brief MIR操作
 * @attention gcc4.5.x以外は試していない
 * @note DM3730CPUマニュアル参照,移植性をあげるためインライン関数としている
 */


/* os/arch/cpu */
#include "intr_cntrl.h"
/* os/kernel */
#include "kernel/defines.h"


/*!
 * @brief 割込みコントローラ(MIR有効化)
 * @param[in] irq:IRQ番号
 *	@arg 0~96
 * @param[out] なし
 * @return なし
 */
void intc_enable_irq(INTRPT_TYPE irq)
{
	int n, bit;

	n = irq / 32;
	bit = irq % 32;
	REG32_WRITE(INTCPS_MIR_CLEAR(n), (1<<bit))
}


/*!
 * @brief 割込みコントローラ(MIR無効化)
 * @param[in] irq:IRQ番号
 *	@arg 0~96
 * @param[out] なし
 * @return なし
 */
void intc_disable_irq(INTRPT_TYPE irq)
{
	int n, bit;

	n = irq / 32;
	bit = irq % 32;
	REG32_WRITE(INTCPS_MIR_SET(n), REG32_READ(INTCPS_MIR_SET(n)) & ~(1<<bit));
}


/*!
 * @brief CPSRの外部割込み(IRQとFIQ)有効化チェック
 * @param[in] なし
 * @param[out] なし
 * @return 外部割込みの真偽
 *	@retval FALSE:外部割込み有効,TRUE:外部割込み無効
 */
int is_exter_intr_enable(void)
{
	int state;

	asm volatile("mrs %0, cpsr\n":"=r"(state));
	
	/* irq割込み有効の場合 */
	if (state & 0x84) {
		return FALSE;
	}
	/* irq割込み無効の場合 */
	else {
		return TRUE;
	}
}


/*!
 * @brief CPSRの外部割込み(IRQとFIQ)有効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 */
void enable_exter_intr(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 "bic r0, r0, #0x84\n\t"
			 "msr cpsr, r0\n");
}


/*!
 * @brief CPSRの外部割込み(IRQとFIQ)無効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 */
void disable_ext_intr(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 "orr r0, r0, #0x84\n\t"
			 "msr cpsr, r0\n");
}


/*!
 * @brief CPSRのIRQ割込み有効化チェック
 * @param[in] なし
 * @param[out] なし
 * @return IRQ割込みの真偽
 *	@retval FALSE:IRQ割込み有効,TRUE:IRQ割込み無効
 */
int is_irq_enable(void)
{
	int state;

	asm volatile("mrs %0, cpsr\n":"=r"(state));
	
	/* irq割込み有効の場合 */
	if (state & 0x80) {
		return FALSE;
	}
	/* irq割込み無効の場合 */
	else {
		return TRUE;
	}
}


/*!
 * @brief CPSRのIRQ有効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 * @note CPSRの7ビット目が0だとirq割込み有効化
 */
void enable_irq(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 "bic r0, r0, #0x80\n\t"
			 "msr cpsr, r0\n");
}


/*!
 * @brief CPSRのIRQ無効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 * @note CPSRの7ビット目に1を立てるとirq割込み無効化
 */
void disable_irq(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 "orr r0, r0, #0x80\n\t"
			 "msr cpsr, r0\n");
}


/*!
 * @brief CPSRのFIQ割込み有効化チェック
 * @param[in] なし
 * @param[out] なし
 * @return FIQ割込みの真偽
 *	@retval FALSE:FIQ割込み有効,TRUE:FIQ割込み無効
 */
int is_fiq_enable(void)
{
	int state;

	asm volatile("mrs %0, cpsr\n":"=r"(state));
	
	/* fiq割込み有効の場合 */
	if (state & 0x40) {
		return 0;
	}
	/* fiq割込み無効の場合 */
	else {
		return 1;
	}
}


/*!
 * @brief CPSRのIRQ有効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 * @note CPSRの7ビット目が0だとfiq割込み有効化
 */
void enable_fiq(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 						"bic r0, r0, #0x40\n\t"
			 						"msr cpsr, r0\n");
}


/*!
 * @brief CPSRのFIQ有効化
 * @param[in] なし
 * @param[out] なし
 * @return なし
 * @note CPSRの7ビット目に1を立てるとfiq割込み有効化
 */
void disable_fiq(void)
{
	asm volatile("mrs r0, cpsr\n\t"
			 						"orr r0, r0, #0x40\n\t"
			 						"msr cpsr, r0\n");
}
