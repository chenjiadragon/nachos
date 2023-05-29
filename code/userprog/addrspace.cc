// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

#include "translate.h"
#include "machine.h"
extern Machine *machine;

#define MAX_USERPROCESS 256 // 最多256个进程
// 静态类成员只初始化一次，在方法文件中初始化
BitMap *AddrSpace::PageBitmap = new BitMap(NumPhysPages);
BitMap *AddrSpace::PidMap = new BitMap(MAX_USERPROCESS);

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
static void SwapHeader(NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    // 分配进程号pid
    spaceId = PidMap->Find() + 100; // 0-100是核心，100-256是用户进程
    // 不存在则返回-1
    if (spaceId == 99 || spaceId > 256)
    {
        printf("Process is Too Much!\n");
        return;
    }

    NoffHeader noffH;
    unsigned int i, size;

    // 读取执行文件的头部数据
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);

    // 判断是否是.noff文件格式
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize; // we need to increase the size
                                                                                          // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages); // check we're not trying
                                      // to run anything too big --
                                      // at least until we have
                                      // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);
    // first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;                   // for now, virtual page # = phys page #
        pageTable[i].physicalPage = PageBitmap->Find(); // 找到空闲页
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; // if the code segment was entirely on
                                       // a separate page, we could set its
                                       // pages to be read-only
        // 清理每一页的数据内存空间
        bzero(&(machine->mainMemory[pageTable[i].physicalPage * PageSize]), PageSize);
    }

    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    // bzero(machine->mainMemory, size);          // 多进程不能从头清理内存

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0)
    {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        // 通过虚拟地址，将程序写入内存对应物理地址
        int phyaddr = pageTable[noffH.code.virtualAddr / PageSize].physicalPage * PageSize;
        int offset = noffH.code.virtualAddr % PageSize;
        executable->ReadAt(&(machine->mainMemory[phyaddr + offset]),
                           noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0)
    {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        int phyaddr = pageTable[noffH.initData.virtualAddr / PageSize].physicalPage * PageSize;
        ;
        int offset = noffH.initData.virtualAddr % PageSize;
        executable->ReadAt(&(machine->mainMemory[phyaddr + offset]),
                           noffH.initData.size, noffH.initData.inFileAddr);
    }
}





//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------
AddrSpace::~AddrSpace()
{
    // 一个程序结束后，回收内存
    for (int i = 0; i < numPages; i++)
    {
        PageBitmap->Clear(pageTable[i].physicalPage);
    }
    delete[] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

// 输出程序页表（页面与帧的映射关系）
void AddrSpace::Print()
{
    printf("page table dump: %d pages in total\n", numPages);
    printf("=============================\n");
    printf("\tVirtPage, \tPhysPage\n");

    for (int i = 0; i < numPages; i++)
    {
        printf("\t %d, \t\t%d\n", pageTable[i].virtualPage, pageTable[i].physicalPage);
    }
    printf("============================================\n\n");
}