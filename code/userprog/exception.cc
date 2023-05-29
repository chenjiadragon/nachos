// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"


#include "machine.h"
#include "openfile.h"
#include "filesys.h"
#include "addrspace.h"
extern Machine *machine;
extern FileSystem *fileSystem;


// 对系统调用进行PC的推进操作
// 否则会一直执行这条系统调用
// 或者将系统调用的return改为break,这里用AdvancePC()
void AdvancePC(){
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}


// 重载StartProcess，作为新建线程执行的代码，
// 并将进程的pid传递给系统，供其它系统调用（如Join()）使用
void StartProcess(int spaceId)
{
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState();  // load page table register

    machine->Run(); // jump to the user progam
    ASSERT(FALSE);  // machine->Run never returns;
                    // the address space exits by doing the syscall "exit"
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
// which指的是中断类型（内中断）
void ExceptionHandler(ExceptionType which)
{
    // 系统调用号保存在MIPS的2号寄存器中
    int type = machine->ReadRegister(2);

    // 增加系统调用处理方法
    if (which == SyscallException)
    {
        switch (type)
        {
        case SC_Halt:{
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

        case SC_Exec:{
            printf("Execute system call of Exec()\n");
            printf("CurrentThreadID: %d Name: %s \n", currentThread->space->getSpaceId(), currentThread->getName());
            // 1.读取参数
            char filename[128];
            // 寄存器存放的是字符串的首字符地址
            int addr = machine->ReadRegister(4);
            int i = 0;
            do
            {
                // 从内存读取文件名
                machine->ReadMem(addr + i, 1, (int *)&filename[i]);
            } while (filename[i++] != '\0');

            printf("Exec(%s):\n", filename);
            // 2.打开可执行文件
            OpenFile *executable = fileSystem->Open(filename);
            if(!executable){
                printf("Unable to open file %s\n", filename);
                return ;
            }

            // 3.为执行文件创建执行地址空间
            AddrSpace *space = new AddrSpace(executable);
            space->Print();
            delete executable;

            // 4.创建内核进程
            Thread *thread = new Thread(filename);
            printf("New Thread SpaceID: %d, Name: %s\n", space->getSpaceId(), filename);
            // 说明一下这里所进行的操作：StartProcess将main主进程的寄存器等初始化，之后运行结束
            // 此时就绪队列中只有刚刚创建的进程，所以会自动执行刚创建的进程
            thread->Fork(StartProcess, (int)space->getSpaceId());
            thread->space = space;

            // 5.将返回值PID存放在2号寄存器
            machine->WriteRegister(2, space->getSpaceId());
            AdvancePC(); // PC增量指向下条指令
            break;
        }

        case SC_Join:{
            printf("Execute system call of Join().\n");
            printf("CurrentThreadId: %d Name: %s \n",(currentThread->space)->getSpaceId(),currentThread->getName());
            
            int Spaceid = machine->ReadRegister(4);
            currentThread->Join(Spaceid);
            machine->WriteRegister(2, currentThread->waitExitCode());
            AdvancePC();
            break;
        }

        case SC_Exit:{
            printf("Execute system call of Exit()\n");
            printf("CurrentThreadId: %d Name: %s\n", (currentThread->space)->getSpaceId(),currentThread->getName());
            // 1.从4号寄存器读取状态信息
            int exitCode = machine->ReadRegister(4);  
            // 2.将状态信息写入2号寄存器（返回值一般都在2号寄存器）
            machine->WriteRegister(2, exitCode);

            currentThread->setExitCode(exitCode);
            // 父进程退出，delete所有终止进程
            if(exitCode==99)
                scheduler->emptyList(scheduler->getTerminatedList());
            delete currentThread->space;
            currentThread->Finish();
            AdvancePC();
            break;
        }

        // Yield the CPU to another runnable thread, 
        // whether in this address space or not.
        case SC_Yield:{
            printf("Execute system call of Yield.\n");
            printf("CurrentThreadId: %d Name: %s \n",(currentThread->space)->getSpaceId(),currentThread->getName());
            currentThread->Yield();
            AdvancePC();
            break;
        }

        default:{
            printf("Unexpected syscall %d %d\n", which, type);
            ASSERT(FALSE);
            break;
        }

        }
    }
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

