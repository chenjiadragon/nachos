// directory.h
//	Data structures to manage a UNIX-like directory of file names.
//
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 15  // 文件名以及目录最大长度

// 多级目录一个目录项在DISK中占36个字节
class DirectoryEntry
{
public:
  bool inUse;                    // 该目录项是否已经分配
  int sector;                    // 文件头所在的扇区号，文件头指的是FCB或i-node
  char name[FileNameMaxLen + 1]; // 文件名

  // 多级目录增加项
  int parent;    // 目录节点父节点
  int leftChild; // 目录节点左子树
  int sibilings; // 目录节点右兄弟
  int filetype;  // 表示文件类型，0为文件夹，1为文件
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk.

class Directory
{
public:
  Directory(int size); // Initialize an empty directory
                       // with space for "size" files
  ~Directory();        // De-allocate the directory

  void FetchFrom(OpenFile *file); // Init directory contents from disk
  void WriteBack(OpenFile *file); // Write modifications to
                                  // directory contents back to disk

  int Find(char *name); // Find the sector number of the
                        // FileHeader for file: "name"

  bool Add(char *name, int newSector); // Add a file name into the directory

  bool Remove(char *name); // Remove a file from the directory

  void List();  // Print the names of all the files
                //  in the directory
  void Print(); // Verbose print of the contents
                //  of the directory -- all the file
                //  names and their contents.

private:
  int tableSize;         // Number of directory entries
  DirectoryEntry *table; // Table of pairs:
                         // <file name, file header location>

  // Find the index into the directory table corresponding to "name"
  // 可以查找目录，也可以查找文件，返回目录项索引
  int FindIndex(char *name); 
                             
};

#endif // DIRECTORY_H
