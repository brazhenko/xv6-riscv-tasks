#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void) {
    printf("free pages: %l\n", freemem());
    printf("free bytes: %l\n", freemem() * 4096);
    printf("free mbytes: %l\n", freemem() * 4096 / 1024 / 1024);

}