#include "defines.h"

extern void start(void);
extern void intr_softerr(void);
extern void intr_syscall(void);
extern void intr_serintr(void);

/* どこに割り込みビットが立つとどこを呼び出すかを宣言している */
/* vectors[0]のビットが立てば、リンク後の関数の中で_startという関数を呼び出す */

void (*vectors[])(void) = {
  start, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_syscall, intr_softerr, intr_softerr, intr_softerr,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
};
