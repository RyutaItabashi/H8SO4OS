#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

#define THREAD_NUM 6
#define THREAD_NAME_SIZE 15

/* スレッドコンテクスト */
typedef struct _kz_context {
    uint32 sp;
} kz_context;

/* タスクコントロールブロック(TCB) */
typedef struct _kz_thread {
    struct _kz_thread *next;
    char name[THREAD_NAME_SIZE + 1];
    char *stack;

    struct {
        kz_func_t func;
        int argc;
        char **argv;
    } init;

    struct {
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;
} kz_thread;

/* スレッドのレディーキュー */
static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque;

static kz_thread *current;
static kz_thread threads[THREAD_NUM];
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];

void dispatch(kz_context *context);

/* レディーキューの先頭を次のスレッドに変える */
static int getcurrent(void){
    if (current == NULL){
        return -1;
    }

    /* カレントスレッドは先頭にあるはずなので，先頭から抜き出す */
    readyque.head = current->next;
    if(readyque.head == NULL){
        readyque.tail = NULL;
    }
    current->next = NULL;

    return 0;

}

/* カレントスレッドをレディーキューにつなげる */
static int putcurrent(void){
    if(current == NULL){
        return -1;
    }

    /* レディーキューの末尾に接続する */
    if(readyque.tail) {
        readyque.tail->next = current;
    } else {
        readyque.head = current;
    }
    readyque.tail = current;

    return 0;
}

static int thread_end(void){
    kz_exit();
}

static void thread_init(kz_thread *thp){
    /* スレッドのメイン関数を呼ぶ */
    thp->init.func(thp->init.argc, thp->init.argv);
    thread_end();
}

/* システムコールの処理(1.thread_runされたとき) */
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[]){
    int i;
    kz_thread *thp;
    uint32 *sp;
    extern char userstack; /* ld.scrで定義されているユーザースタック領域のアドレス */
    static char *thread_stack = &userstack;

    /* 空いているタスクコントロールブロックを検索 */
    for (i=0;i<THREAD_NUM;i++){
        thp = &threads[i];
        if(!thp->init.func){
            break;
        }
    }
    if(i == THREAD_NUM){
        return -1;
    }
    memset(thp, 0, sizeof(*thp));

    /* タスクコントロールブロックの設定 */
    strcpy(thp->name, name);
    thp->next = NULL;
    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;

    /* スタック領域の確保 */
    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize;

    thp->stack = thread_stack;

    /* スタックの初期化 */
    sp = (uint32 *)thp->stack;
    *(--sp) = (uint32)thread_end;

    /*
     * プログラムカウンタを設定する
     */
    *(--sp) = (uint32)thread_init;
    
    *(--sp) = 0; /* ER6 */
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0; /* ER1 */

    /* スレッドのスタートアップ */
    *(--sp) = (uint32)thp; /* ER0 */

    /* スレッドのコンテクスト設定 */
    thp->context.sp = (uint32)sp;

    /* システムコールを呼び出したスレッドをレディーキューに渡す */
    putcurrent();

    /* 新しいスレッドをレディーキューへ接続する */
    current = thp;
    putcurrent();

    return (kz_thread_id_t)current;
}

/* システムコールの処理(2.thread_exitされたとき) */
static int thread_exit(void){
    /* スタック開放は未実装 */
    puts(current->name);
    puts(" EXIT\n");
    memset(current, 0, sizeof(*current));
    return 0;
}

/* 割り込みハンドラへの登録 */
static int setintr(softvec_type_t type, kz_handler_t handler){
    static void thread_intr(softvec_type_t type, unsigned long sp);

    softvec_setintr(type, thread_intr);
    handlers[type] = handler; /* OS側から呼び出すハンドラを登録 */

    return 0;
}

static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *p){
    /* 実行中に current が書き換わる */
    switch (type) {
        case KZ_SYSCALL_TYPE_RUN: /* kz_run(); */
            p->un.run.ret = thread_run(p->un.run.func, p->un.run.name, p->un.run.stacksize, p->un.run.argc, p->un.run.argv);
            break;
        case KZ_SYSCALL_TYPE_EXIT: /* :kz_exit(); */
            thread_exit();
            break;
        default:
            break;
    }
}

/* システムコールの処理 */
static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p){
    getcurrent();
    call_functions(type, p);
}

static void schedule(void){
    if(!readyque.head){
        kz_sysdown();
    }
    current = readyque.head;
}

static void syscall_intr(void){
    syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void){
    puts(current->name);
    puts(" DOWN\n");
    getcurrent();
    thread_exit();
}

/* 割り込み関数の入り口 */
static void thread_intr(softvec_type_t type, unsigned long sp){
    current->context.sp = sp;

    if(handlers[type])
        handlers[type]();

    schedule();

    dispatch(&current->context);
    /* ここには返らない */
}

/* 初期スレッドのスタート */
/* これがなきゃ始まらない!!!!! */
void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[]){
    current = NULL;

    readyque.head = readyque.tail = NULL;
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));

    /* 割り込みハンドラ登録 */
    setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
    setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

    current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);

    dispatch(&current->context);

    /* ここには返らない */
}

void kz_sysdown(void){
    puts("system error!\n");
    while(1)
        ;
}

/* システムコールを発行するライブラリ関数 */
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param){
    current->syscall.type = type;
    current->syscall.param = param;
    asm volatile("trapa #0"); /* 割り込み発行 */
}