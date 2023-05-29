// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

#include <string>
#include <vector>
#include <queue>
//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------
// size表明初始的目录项数
Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
    {
        table[i].inUse = FALSE;
        // -1表示不存在
        table[i].parent = -1;
        table[i].leftChild = -1;
        table[i].sibilings = -1;
        table[i].filetype = 0;
    }

    // 设置0目录项为根目录root,并进行初始化
    table[0].parent = -1;
    table[0].inUse = true;
    std::string temp = "root";
    strncpy(table[0].name, temp.c_str(), temp.length() + 1);
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
// 多级目录中name指目录或文件路径
// 目录：/root/dev/
// 文件：/root/dev/small
// 不存在返回-1，存在则返回目录项下标索引
int Directory::FindIndex(char *name)
{
    // 判断删除的是文件还是目录
    std::string path_name = name;
    int ch_num = path_name.size();
    int filetype;
    if (path_name[ch_num - 1] != '/'){
        filetype = 1;           // 寻找的是文件
    }else{
        filetype = 0;           // 目录
    }
    
    
    int ans = -1;
    // 分割路径名，并一一查找
    std::vector<std::string> PathTable; // 逐一存放目录和最终的文件
    std::string str_name = name;

    // 合法路径必有两个'/'
    int begin = str_name.find('/');
    int end = str_name.find('/', begin + 1);
    PathTable.push_back(str_name.substr(begin + 1, end - begin - 1));

    while (1)
    {
        begin = end;
        if(begin == ch_num-1){
            break;
        }

        end = str_name.find('/', begin + 1);
        if (end == -1)
        {
            PathTable.push_back(str_name.substr(begin + 1));
            break;
        }
        
        PathTable.push_back(str_name.substr(begin + 1, end - begin - 1));
    }

    // 搜索目录项
    ans = 0;
    int n = PathTable.size();
    // 寻找文件则 n 必须不小于2
    if (filetype == 1 && n < 2)
    {
        // printf("FilePath is incorrect!\n");
        return -1;
    }
    // 寻找目录则 n 必须不小于1
    if (filetype == 0 && n < 1)
    {
        return -1;
    }
    if (strncmp(table[ans].name, PathTable[0].c_str(), FileNameMaxLen))
    {
        printf("dir %s does not exist!\n", PathTable[0].c_str());
        return -1;
    }

    for (int i = 1; i < n; i++)
    {
        // 合法情况是PathTable前n-1项必是目录
        if (i < n - 1)
        {
            // 遍历当前目录，找到匹配项
            // 遍历左孩子以及左孩子的右兄弟
            int ChildIndex = table[ans].leftChild;
            bool is_finded = false;

            while (ChildIndex != -1)
            {
                if (table[ChildIndex].inUse && table[ChildIndex].filetype == 0 && !strncmp(table[ChildIndex].name, PathTable[i].c_str(), FileNameMaxLen))
                {
                    is_finded = true;
                    ans = ChildIndex; // 找到中间目录
                    break;
                }
                else
                {
                    ChildIndex = table[ChildIndex].sibilings;
                }
            }
            if (!is_finded)
            {
                // printf("dir %s does not exist!\n", PathTable[i].c_str());
                return -1;
            }
        }
        else
        {
            if (filetype == 1)
            {
                // 查找文件
                int ChildIndex = table[ans].leftChild;
                bool is_finded = false;

                while (ChildIndex != -1)
                {
                    if (table[ChildIndex].inUse && table[ChildIndex].filetype == 1 && !strncmp(table[ChildIndex].name, PathTable[i].c_str(), FileNameMaxLen))
                    {
                        is_finded = true;
                        ans = ChildIndex; // 找到中间目录
                        break;
                    }
                    else
                    {
                        ChildIndex = table[ChildIndex].sibilings;
                    }
                }
                if (!is_finded)
                {
                    // printf("file %s does not exist!\n", PathTable[i].c_str());
                    return -1;
                }
            }
            else if (filetype == 0)
            {
                // 最后一个还是目录
                int ChildIndex = table[ans].leftChild;
                bool is_finded = false;

                while (ChildIndex != -1)
                {
                    if (table[ChildIndex].inUse && table[ChildIndex].filetype == 0 && !strncmp(table[ChildIndex].name, PathTable[i].c_str(), FileNameMaxLen))
                    {
                        is_finded = true;
                        ans = ChildIndex; // 找到中间目录
                        break;
                    }
                    else
                    {
                        ChildIndex = table[ChildIndex].sibilings;
                    }
                }
                if (!is_finded)
                {
                    // printf("dir %s does not exist!\n", PathTable[i].c_str());
                    return -1;
                }
            }
        }
    }

    return ans;
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
// 此函数返回的是目录项指向的文件头的所在扇区号
int Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------
// 在目录中为一个文件添加一个目录项
// 单机目录中name是文件名，多级目录中则是文件路径：/root/dev/small
bool Directory::Add(char *name, int newSector)
{
    if (FindIndex(name) != -1) // 已存在
        return FALSE;

    // 分割路径
    // for example: /dir1/dir2/dir3/file  ==>   </dir1/dir2/dir3/, file>
    // 最后必须是文件，除了第一个/root目录，其他中间的目录可能不存在，需要自动创建
    std::string copy_name = name;
    int curpos = copy_name.rfind('/');
    int curdir = 0; // 当前目录项下标
    std::string ParentDir = copy_name.substr(0, curpos + 1);
    std::string Childfile = copy_name.substr(curpos + 1);

    curpos = ParentDir.find('/');
    curpos = ParentDir.find('/', curpos + 1);
    while (curpos != -1)
    {
        char *ch_temp = const_cast<char *>(ParentDir.substr(0, curpos+1).c_str());
        int dir_temp = FindIndex(ch_temp);

        // printf("%s\n", ch_temp);
        // printf("%d\n", dir_temp);

        if (dir_temp != -1)
        {
            curdir = dir_temp;
            curpos = ParentDir.find('/', curpos + 1);
        }
        else
        {
            // 创建目录项为dir
            std::string str1 = ParentDir.substr(0, curpos);
            char *dir_name = const_cast<char *>(str1.substr(str1.rfind('/') + 1).c_str());

            // printf("%s\n", dir_name);

            for (int i = 0; i < tableSize; i++)
            {
                if (!table[i].inUse)
                {
                    table[i].inUse = TRUE;
                    table[i].filetype = 0;
                    strncpy(table[i].name, dir_name, FileNameMaxLen);
                    table[i].parent = curdir;
                    table[i].leftChild = -1;
                    table[i].sibilings = -1;
                    // 加入目录树（孩子兄弟表示法）
                    if (table[curdir].leftChild == -1)
                    {
                        table[curdir].leftChild = i;
                    }
                    else
                    {
                        int child_temp = table[curdir].leftChild;
                        while (table[child_temp].sibilings != -1)
                        {
                            child_temp = table[child_temp].sibilings;
                        }
                        table[child_temp].sibilings = i;
                    }

                    curdir = i;
                    curpos = ParentDir.find('/', curpos + 1);
                    break;
                }
            }
        }
    }

    // 为文件创建目录项
    char *file_temp = const_cast<char *>(Childfile.c_str());
    // printf("%s\n", file_temp);
    for (int i = 0; i < tableSize; i++)
    {
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            table[i].filetype = 1;
            strncpy(table[i].name, file_temp, FileNameMaxLen);
            table[i].parent = curdir;
            table[i].leftChild = -1;
            table[i].sibilings = -1;
            table[i].sector = newSector;
            // 加入目录树（孩子兄弟表示法）
            if (table[curdir].leftChild == -1)
            {
                table[curdir].leftChild = i;
            }
            else
            {
                int child_temp = table[curdir].leftChild;
                while (table[child_temp].sibilings != -1)
                {
                    child_temp = table[child_temp].sibilings;
                }
                table[child_temp].sibilings = i;
            }
            return TRUE;
        }
    }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------
// 删除文件或目录
// 文件：/dir1/dir2/dir3/file
// 目录：/dir1/dir2/dir3/
bool Directory::Remove(char *name)
{
    // 判断删除的是文件还是目录
    std::string path_name = name;
    int ch_num = path_name.size();
    if (path_name[ch_num - 1] != '/')
    { // 删除的是文件
        int i = FindIndex(name);
        if (i == -1)
            return false; // 文件不存在

        if (table[table[i].parent].leftChild == i)
        {
            // 该文件处于父节点的左孩子
            table[table[i].parent].leftChild = table[i].sibilings;
        }
        else
        {
            // 该文件处于某个兄弟的右指针
            int ChildFile = table[table[i].parent].leftChild;
            while (table[ChildFile].sibilings != i)
            {
                ChildFile = table[ChildFile].sibilings;
            }
            table[ChildFile].sibilings = table[i].sibilings;
        }
        table[i].inUse = false;
    }
    else
    {
        // 删除的是目录
        // 根目录/root不能删除
        if (path_name == "/root/")
            return false;
        // 去掉末尾字符'/'
        // name[ch_num - 1] = '\0';
        int i = FindIndex(name);
        if (i == -1)
            return false; // 目录不存在

        // 维护目录树
        if (table[table[i].parent].leftChild == i)
        {
            // 该目录处于父节点的左孩子
            table[table[i].parent].leftChild = table[i].sibilings;
        }
        else
        {
            // 该目录处于某个兄弟的右指针
            int ChildFile = table[table[i].parent].leftChild;
            while (table[ChildFile].sibilings != i)
            {
                ChildFile = table[ChildFile].sibilings;
            }
            table[ChildFile].sibilings = table[i].sibilings;
        }
        // 需要递归的删除该目录下所有的目录和文件
        // 二叉树层次遍历进行删除
        std::queue<int> myQueue;
        myQueue.push(i);

        while (!myQueue.empty())
        {
            // 执行操作
            int curRemove = myQueue.front();
            myQueue.pop();

            table[curRemove].inUse = false;

            int left = table[curRemove].leftChild;
            int right = table[curRemove].sibilings;
            if (left != -1 && table[left].inUse)
            {
                myQueue.push(left);
            }
            if (right != -1 && table[right].inUse)
            {
                myQueue.push(right);
            }
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------
// 输出所有的文件和目录的名称
// 由parent指针找到父目录
void Directory::List()
{
    for (int i = 0; i < tableSize; i++)
    {
        if (table[i].inUse)
        {
            // printf("%s\n", table[i].name);
            std::string path = table[i].name;
            path = '/' + path;
            if(table[i].filetype == 0){
                // 如果是目录，多加个'/'
                path = path + '/';
            }
            int parent_id = table[i].parent;
            while (parent_id != -1)
            {
                std::string str1 = table[parent_id].name;
                path = '/' + str1 + path;
                parent_id = table[parent_id].parent;
            }
            printf("%s\n", path.c_str());
        }
    }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && table[i].filetype == 1)
        {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}
