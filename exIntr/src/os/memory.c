#include "defines.h"
#include "kozos.h"
#include "lib.h"
#include "memory.h"

/* メモリブロック 構造体 */
typedef struct _kzmem_block{
	struct _kzmem_block *next;
	int size;
} kzmem_block;

/* メモリプール */
typedef struct _kzmem_pool{
	int size;
	int num;
	kzmem_block *free;
} kzmem_pool;

/* メモリプールの定義 */
static kzmem_pool pool[] = {
	{ 16, 8, NULL}, {32, 8, NULL}, {64, 4, NULL},
};

#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))

/* メモリプールの初期化 */
static int kzmem_init_pool(kzmem_pool *p){
	int i;
	kzmem_block *mp;
	kzmem_block **mpp;
	extern char freearea;
	static char *area = &freearea;

	mp = (kzmem_block *)area;

	mpp = &p->free;
	for(i=0;i<p->num;i++){
		*mpp = mp;
		memset(mp, 0, sizeof(*mp));
		mp->size = p->size;
		mpp = &(mp->next);
		mp = (kzmem_block *)((char *)mp + p->size);
		area += p->size;
	}

	return 0;
}

/* 動的メモリの初期化 */
int kzmem_init(void){
	int i;
	for(i=0;i<MEMORY_AREA_NUM;i++){
		kzmem_init_pool(&pool[i]);
	}
	return 0;
}

/* 動的メモリの取得 */
void *kzmem_alloc(int size){
	int i;
	kzmem_block *mp;
	kzmem_pool *p;

	for(i=0;i<MEMORY_AREA_NUM;i++){
		p = &pool[i];
		if(size <= p->size - sizeof(kzmem_block)) {
			if(p->free == NULL) {
				kz_sysdown();
				return NULL;
			}

			mp = p->free;
			p->free = p->free->next;
			mp->next = NULL;

			return mp + 1;
		}
	}

	kz_sysdown();
	return NULL;
}

void kzmem_free(void *mem){
	int i;
	kzmem_block *mp;
	kzmem_pool *p;

	mp = ((kzmem_block *)mem - 1);

	for(i=0;i<MEMORY_AREA_NUM;i++){
		p = &pool[i];
		if (mp->size == p->size){
			mp->next = p->free;
			p->free = mp;
			return;
		}
	}

	kz_sysdown();
}

