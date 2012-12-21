/* 割り込みハンドラを周り */

#ifndef _INTR_HANDLE_H_INCLUDED_
#define _INTR_HANDLE_H_INCLUDED_


/*!  未定義命令ハンドラ */
extern void intr_und(unsigned long sp);

/*! SVC割込みハンドラ */
extern void intr_swi(unsigned long sp);

/*! プリフェッチアボートハンドラ */
extern void intr_pabort(unsigned long sp);

/*! データアボートハンドラ */
extern void intr_dabort(unsigned long sp);

/*! IRQハンドラ */
extern void intr_irq(unsigned long sp);


#endif
