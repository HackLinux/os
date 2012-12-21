/* os/kernel */
#include "defines.h"
#include "kernel.h"
#include "memory.h"
/* os/c_lib */
#include "c_lib/lib.h"


/*!
 * メモリ・ブロック構造体
 * (獲得された各領域は，先頭に以下の構造体を持っている)
 */
typedef struct _mem_block {
  struct _mem_block *next;
  int size;
} MEM_BLOCK;

/*! メモリプール */
typedef struct _mem_pool {
  int size;
  int num;
  MEM_BLOCK *free;
} MEM_POOL;


/*! メモリプールの初期化 */
static void mem_init_pool(MEM_POOL *p);

/*! メモリプールの定義(個々のサイズ(2のべき乗)と個数) */
/* ターゲットのメモリサイズを考える事 */
static MEM_POOL sg_pool[] = {
  { 16, 128, NULL }, { 32, 128, NULL }, { 64, 128, NULL }, { 128, 128, NULL }, {256, 128, NULL}, {512, 8, NULL}, {1024, 8, NULL}
};

/* メモリプールサイズ */
#define MEMORY_AREA_NUM (sizeof(sg_pool) / sizeof(*sg_pool))


/*!
 * メモリプールの初期化
 * *p : 指定されたメモリプールポインタ
 */
static void mem_init_pool(MEM_POOL *p)
{
  int i;
  MEM_BLOCK *mp;
  MEM_BLOCK **mpp;
  extern char _heap; /* リンカスクリプトで定義される空き領域 */
  static char *area = &_heap;

  mp = (MEM_BLOCK *)area;

  /* 個々の領域をすべて解放済みリンクリストに繋ぐ */
  mpp = &p->free;
  for (i = 0; i < p->num; i++) {
    *mpp = mp;
    memset(mp, 0, sizeof(*mp));
    mp->size = p->size;
    mpp = &(mp->next);
    mp = (MEM_BLOCK *)((char *)mp + p->size);
    area += p->size;
  }
}

/*! 動的メモリの初期化 */
void mem_init(void)
{
  int i;
  
  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    mem_init_pool(&sg_pool[i]); /* 各メモリプールを初期化する */
  }
}

/*!
 * 動的メモリの獲得
 * size : 要求サイズ
 */
void* get_mpf_isr(int size)
{
  int i;
  MEM_BLOCK *mp;
  MEM_POOL *p;

	DEBUG_LEVEL1_OUTVLE(size, 0);
	DEBUG_LEVEL1_OUTMSG(" out memory size : get_mpf_isr().\n");
  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    p = &sg_pool[i];
    if (size <= p->size - sizeof(MEM_BLOCK)) {
      if (p->free == NULL) { /* 解放済み領域が無い(メモリブロック不足) */
				KERNEL_OUTMSG("error: get_mpf_isr()1 \n");
				down_system();
				return NULL;
      }
      /* 解放済みリンクリストから領域を取得する */
      mp = p->free;
      p->free = p->free->next;
      mp->next = NULL;

      /*
       * 実際に利用可能な領域は，メモリブロック構造体の直後の領域に
       * なるので，直後のアドレスを返す．
       */
      return mp + 1;
    }
  }

  /* 指定されたサイズの領域を格納できるメモリプールが無い */
	KERNEL_OUTMSG("error: get_mpf_isr2() \n");
  down_system();
  
  return NULL;
}

/*! 
 * メモリの解放
 * *mem : 解放ブロック先頭ポインタ
 */
void rel_mpf_isr(void *mem)
{
  int i;
  MEM_BLOCK *mp;
  MEM_POOL *p;

  /* 領域の直前にある(はずの)メモリ・ブロック構造体を取得 */
  mp = ((MEM_BLOCK *)mem - 1);

  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    p = &sg_pool[i];
    if (mp->size == p->size) {
      /* 領域を解放済みリンクリストに戻す */
      mp->next = p->free;
      p->free = mp;
      return;
    }
  }

	KERNEL_OUTMSG("error: rel_mpf_isr() \n");
  down_system();
}
