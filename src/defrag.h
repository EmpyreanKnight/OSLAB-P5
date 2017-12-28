#ifndef P5_DEFRAG_H
#define P5_DEFRAG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern int d_error;
#define ERROR_ALL_GREEN             0
#define ERROR_DATA_BLOCK_LOST       1
#define ERROR_CORRUPTED_FREE_DATA   2
#define ERROR_CORRUPTED_SWAP_REGION 4

#define DEFAULT_BLOCK_SIZE          512
typedef struct {
    int size;         /* size of blocks in bytes */
    int inode_offset; /* offset of inode region in blocks */

    // All offsets in the superblock start at 1024 bytes into the disk and are given as blocks.
    int data_offset;  /* data region offset in blocks */
    int swap_offset;  /* swap region offset in blocks */

    // These values are indices.
    // So, if free_inode is 12,
    // then the head of the free inode list is the 13th inode in the inode region.
    int free_inode;   /* head of free inode list */
    int free_iblock;  /* head of free block list */
} superblock;

#define N_DBLOCKS 10
#define N_IBLOCKS 4
// All block pointers are relative to the start of the data region.
typedef struct {
    int next_inode;         /* list for free inodes */
    int protect;            /* protection field */
    int nlink;              /* number of links to this file */
    int size;               /* numer of bytes in file */
    int uid;                /* owner's user ID */
    int gid;                /* owner's group ID */
    int ctime;              /* time field */
    int mtime;              /* time field */
    int atime;              /* time field */
    int dblocks[N_DBLOCKS]; /* pointers to data blocks */
    int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
    int i2block;            /* pointer to doubly indirect block */
    int i3block;            /* pointer to triply indirect block */
} inode;

int defragmenter(FILE* inFile, FILE* outFile);

void validator(FILE* inFile);

void printFiles(FILE* inFile);

#endif //P5_DEFRAG_H
