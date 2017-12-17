# Operating System Project 05 - File Defragmentation

## Introduction
In this project, we are required to design a file system defragmenter, which improve file system performance by laying out all the blocks of a file in sequential order on disk. Our defragmenter is for a quiet ancient unix-liked file system, called AFS (Andrew File System). By completing this project, we got a sense of file system structure.

## Usage
Our submit version only contains defragmenter source files, you should build execution first, just type following commands on terminal:
```
% cd <src folder>
% make
```
After that, you will get a executable file called `defrag`, you can use it like:
```
% ./defrag <fragmented disk file>
```
Our output file should be named with `-defrag` suffix concatenated after the input file name but before its extension name (if any).

If you want to verify the correctness of our output result, you only need to change the **39th line** in `main.c`, delete `//` before ``validation`` and save it. Again, you need to re-build and execute file `defrag`, this time the argument is the file name need to be verified. After that, you'll get all files in this file system in `./unpacked` folder, and get debug infos on your terminal.

## Output Format
Our defragmenter will put all **data blocks** of a file continiously, while its indirect blocks after **all** data blocks in this file system. After **all** indirect blocks, there will be free blocks which form a linked-list in ascending order of indexes.
E.g. Assume the sample file system containing three files:
- File1: 2 direct blocks
- File2: 10 direct blocks and 1 indirect block
- File3: 10 direct blocks, 4 indirect blocks and 1 second indirect block
The data region of output file should like this:

File1 data blocks|File2 data blocks|File3 data blocks|File2 I1 block|4 File3 I1 blocks|File3 I2 block|a few(1~128) File3 I1 blocks
-|-|-|-|-|-|-

## Algorithm
