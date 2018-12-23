#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test09_2_main(int argc, char *argv[]){
    puts("test_2 started\n");

    puts("test_2 sleep >>>\n");
    kz_sleep();
    puts(">>> test_2 sleep\n");

    puts("test_2 chpri >>>\n");
    kz_chpri(3);
    puts(">>> test_2 chpri\n");

    puts("test_2 wait >>>\n");
    kz_wait();
    puts(">>> test_2 wait\n");

    puts("test_2 exit.\n");

    return 0;
}
