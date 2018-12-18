#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test09_1_main(int argc, char *argv[]){
    puts("test_1 started\n");

    puts("test_1 sleep >>>\n");
    kz_sleep();
    puts(">>> test_1 sleep\n");

    puts("test_1 chpri >>>\n");
    kz_chpri(3);
    puts(">>> test_1 chpri\n");

    puts("test_1 wait >>>\n");
    kz_wait();
    puts(">>> test_1 wait\n");

    puts("test_1 trap to error >>>\n");
    asm volatile("trapa #1");
    puts(">>> test_1 trap to error\n");

    puts("test_1 exit.\n");

    return 0;
}
