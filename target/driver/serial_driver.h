/*!
 * @file ターゲット依存部<モジュール:serial_driver.o>
 * @brief UARTドライバのインターフェース
 * @attention gcc4.5.x以外は試していない
 * @note DM3730CPUマニュアル参照
 */


#ifndef _SERIAL_DRIVER_H_INCLUDED_
#define _SERIAL_DRIVER_H_INCLUDED_


#define	UART3_BASE_ADR		0x49020000																			/*! UART3のポートアドレス(メモリマップアドレス) */

#define	UIIR			(UART3_BASE_ADR + 0x08)																	/*! まだ未使用 */
#define UFCR UIIR																													/*! レジスタリネーム．送受信FIFOバッファ無効有効化とクリア制御 */


/* デバイス初期化 */
extern void uart3_init(void);

/*! １文字送信 */
extern void send_serial_byte(unsigned char c);

/*! １文字受信 */
extern unsigned char recv_serial_byte(void);

/* 送信割込みが有効か検査 */
extern int serial_intr_is_send_enable(void);

/* 送信割込みを有効化する */
extern void serial_intr_send_enable(void);

/* 送信割込みを無効化する */
extern void serial_intr_send_disable(void);

/* 受信割込みが有効か検査 */
extern int serial_intr_is_recv_enable(void);

/* 受信割込みを有効化する */
extern void serial_intr_recv_enable(void);

/* 受信割込みを無効化する */
extern void serial_intr_recv_disable(void);


#endif
