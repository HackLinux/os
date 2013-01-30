/*!
 * @file ターゲット非依存部
 * @brief 簡易C標準ライブラリインターフェース
 * @attention gcc4.5.x以外は試していない
 */


#ifndef _LIB_H_INCLUDED_
#define _LIB_H_INCLUDED_


/*
 * C++へのプラグイン
 * C++はユーザ空間のみ認める
 */
#ifdef __cplusplus
extern "C" {
#endif



/* ターゲット非依存部 */

/*! メモリセット */
extern void *memset(void *b, int c, long len);
/*! メモリコピー */
extern void *memcpy(void *dst, const void *src, long len);

/*! メモリ内容比較 */
extern int memcmp(const void *b1, const void *b2, long len);

/*! 文字列の長さ */
extern int strlen(const char *s);

/*! 文字列のコピー */
extern char *strcpy(char *dst, const char *src);

/*! 文字列の比較 */
extern int strcmp(const char *s1, const char *s2);

/*! 文字列の比較(文字数制限付き) */
extern int strncmp(const char *s1, const char *s2, int len);

/* 数値へ変換 */
extern int atoi(char str[]);


/* ターゲット依存部 */

/*! １文字送信 */
extern void putc(unsigned char c);

/*! 文字列送信 */
extern int puts(char *str);

/*! 数値の16進表示 */
extern int putxval(unsigned long value, int column);

/*! １文字受信 */
extern unsigned char getc(void);

/*! 文字列受信 */
extern int gets(char *buf);


/*
 * C++へのプラグイン
 * C++はユーザ空間のみ認める
 */
#ifdef __cplusplus
}
#endif


#endif
