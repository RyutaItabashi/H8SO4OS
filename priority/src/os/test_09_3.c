#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test09_3_main(int argc, char *argv[]){
    puts("test_3 started\n");

    puts("test_3 wake 01 >>>\n");
    kz_wakeup(test09_1_id);
    puts(">>> test_3 waked 01\n");

    puts("test_3 wake 02 >>>\n");
    kz_wakeup(test09_2_id);
    puts(">>> test_3 waked 02\n");

    puts("test_3 wait >>>\n");
    kz_wait();
    puts(">>> test_3 wait\n");

    puts("test_3 exit.\n");

    return 0;
}
