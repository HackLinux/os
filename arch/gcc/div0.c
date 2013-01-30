/*!
 * @file コンパイラ依存部(arm-none-eabi-gcc)
 * @brief 乗算、除算関連
 * @attention gcc4.5.x以外は試していない
 * @note info参照
 */


/* Replacement (=dummy) for GNU/Linux division-by zero handler */
void __div0 (void)
{
	while (1)
		;
}
