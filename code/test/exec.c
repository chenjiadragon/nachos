#include "syscall.h"

int main()
{
    SpaceId pid;
    pid = Exec("../test/halt.noff");     //利用nachos的系统调用
    Halt();
    /* not reached */
}