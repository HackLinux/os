/*!
 * @file ターゲット依存部(ARM-Cortex-A8)<モジュール:main.o>
 * @brief kernelのmainとUARTハンドラ
 * @attention gcc4.5.x以外は試していない
 * @note DM3730CPUマニュアル参照
 */


/* os/arch/cpu */
#include "arch/cpu/intr.h"
/* os/kernel */
#include "kernel/defines.h"
#include "kernel/kernel.h"
#include "kernel/command.h"
/* os/c_lib */
#include "c_lib/lib.h"
/* os/target/driver */
#include "target/driver/serial_driver.h"


/* BSSセクションの初期化より前に変数を使用しない */

extern void uart_handler(void);
extern unsigned long _bss_start, _end;

/*! 資源ID */
ER_ID idle_id;
#ifdef TSK_LIBRARY
ER_ID sample_tsk1_id;
ER_ID sample_tsk2_id;
ER_ID sample_tsk3_id;
ER_ID sample_tsk4_id;
ER_ID sample_tsk5_id;
ER_ID sample_tsk6_id;
ER_ID sample_tsk7_id;
ER_ID sample_tsk8_id;
#endif


/*!
 * @brief IRQハンドラ
 * @param[in] なし
 * @param[out] なし
 * @return なし
 * @note UART関連のレジスタであるIIRレジスタから割込みタイプの取得
 *       (IIRレジスタは下位5ビットで割込みタイプを保持している)
 *       シリアル受信割込み : 0x2
 *       タイムアウト割込み(シリアル受信割込みを有効化すると同時に有効化される) : 0x6
 */
void uart_handler(void)
{
  unsigned char c;
  static char buf[32];
  static int len;
  int it_type;

	it_type = (REG8_READ(UIIR) & 0x3E) >> 1;

  if (it_type == 2 || it_type == 6) {

		c = getc(); /* 受信FIFOからデータを読み出す事によって，割込み要因をクリア */

		if (c != '\n') {
			buf[len++] = c;
    }
    else {
			buf[len++] = '\0';
			/* echoコマンドの場合 */
    	if (!strncmp(buf, "echo ", 5)) {
      	echo_command(buf); /* echoコマンド(標準出力にテキストを出力する)呼び出し */
    	}
			/* helpコマンドの場合 */
   		else if (!strncmp(buf, "help", 4)) {
     		help_command(&buf[4]); /* helpコマンド呼び出し */
    	}
#ifdef TSK_LIBRARY
			/* runコマンドの場合 */
			else if (!strncmp(buf, "run", 3)) {
				run_command(&buf[3]); /* runコマンド(タスクセットの起動)呼び出し */
			}
#endif
			/* sendlogの場合 */
			else if (!strncmp(buf, "sendlog", 7)) {
      	sendlog_command(); /* sendlogコマンド(xmodem送信モード)呼び出し */
			}
			/* 本システムに存在しないコマンド */
    	else {
      	puts("command unknown.\n");
    	}
			puts("> ");
			len = 0;
    }
  }
	else {
		DEBUG_LEVEL1_OUTMSG(" not uart3 handler : uart_handler().\n");
	}
}


/*!
 * @brief OSメイン関数
 * @attention CPSRの外部割込み無効モードとして起動
 * @param[in] なし
 * @param[out] なし
 * @return 終了値
 * 	@retval 0:OS実効終了
 * @note 正常retvalは返却されない
 */
int main(void)
{
  
  unsigned long *p;

  /* BSSセクションの初期化(BSSセクションの初期化はここでOK) */
  for (p = &_bss_start; p < &_end; p++) {
    *p = 0;
  }

  uart3_init(); /* シリアルの初期化 */

  KERNEL_OUTMSG("kernel boot OK!\n");

  /* OSの動作開始 */
  kernel_init(start_threads, "init tsk", 0, 0x100, 0, NULL); /* initタスク起動 */
  
  /* 正常ならばここには戻ってこない */

  return 0;
}

