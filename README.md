# nachos
一个简单的操作系统内核
## 1.nachos系统图解
![image](https://github.com/chenjiadragon/nachos/assets/61454134/3d299691-84dd-443d-8924-d2ac403485da)

## 2.预期收获
![image](https://github.com/chenjiadragon/nachos/assets/61454134/6d4390e0-4bb3-4c9f-ab4b-6e32e25ac87b)

## 3.nachos文件目录
![image](https://github.com/chenjiadragon/nachos/assets/61454134/1fe6a089-23f6-4cde-bab6-8ed8642eabc7)

系统目录：
（1）c++example：该目录包含几个C++的示例程序。
（2）code/bin：用于将Nachos应用程序交叉编译成Nachos可执行文件（.noff）。
（3）code/filesys：nachos的文件系统。
（4）code/machine：模拟机器的硬件，Nachos作为一个操作系统运行在这些硬件上。
（5）code.monitor：实现nachos使用的锁、条件变量、信号量、管程等同步机制。
（6）code/network：实现网络的功能。
（7）code/test：包含几个测试nachos系统调用的应用程序（基于nachos的c程序），可在nachos上运行（.noff文件格式）。
（8）code/threads：线程的管理，包括线程的创建、睡眠、终止、调度，以及信号量等功能。
（9）code/userprog：Nachos应用进程的管理，加载一个Nachos应用程序，创建相应的进程，将进程映射到一个核心线程，然后运行。
（10）code/vm：虚拟存储管理。
