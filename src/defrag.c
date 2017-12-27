#include "defrag.h"

/**** Following variables and constant are used to store general information of the target file system ****/
int d_error; // error type descriptor, use bitwise operation

// initial offset in byte of three regions
size_t inodeInitial;
size_t dataInitial;
size_t swapInitial;

superblock *superBlock; // pointer to super-block
size_t blockSize = 512; // default block size, always true for boot and super block
size_t inodeSize = 100; // i-node size, not use sizeof operator to avoid cross-platform problem

// the following three indexes are relative to data region
// used to trace the next location to be filled into output file
int dataBlockIndex = 0;
int freeBlockIndex = 0;

/******* Following functions are used for debug purpose, not necessarily as a part of defragmenter *******/

void dumpBootBlock(const char *bootBlock) {
    int i;
    printf("Boot Block Status:\n");
    for (i = 0; i < blockSize; i++) {
        putchar(*(bootBlock + i));
    }
    putchar('\n');
    putchar('\n');
}

void dumpSuperBlock(superblock *superBlock) {
    printf("Super Block Status:\n");
    printf("Size:         %d\n", superBlock->size);
    printf("Inode offset: %d\n", superBlock->inode_offset);
    printf("Data offset:  %d\n", superBlock->data_offset);
    printf("Swap offset:  %d\n", superBlock->swap_offset);
    printf("Free inode:   %d\n", superBlock->free_inode);
    printf("Free iblock:  %d\n", superBlock->free_iblock);
    printf("\n");
}

void dumpInode(inode *inode) {
    int i;
    printf("Inode Status:\n");
    printf("Next inode:   %d\n", inode->next_inode);
    printf("Protect:      %X\n", inode->protect);
    printf("Number Link:  %d\n", inode->nlink);
    printf("File Size:    %d\n", inode->size);
    printf("User Id:      %d\n", inode->uid);
    printf("Group Id:     %d\n", inode->gid);
    printf("Change Time:  %d\n", inode->ctime);
    printf("Modify Time:  %d\n", inode->mtime);
    printf("Access Time:  %d\n", inode->atime);
    printf("I2 Pointer:   %d\n", inode->i2block);
    printf("I3 Pointer:   %d\n", inode->i3block);
    printf("Direct Blocks:\n");
    for (i = 0; i < N_DBLOCKS; i++) {
        printf("%d ", inode->dblocks[i]);
    }
    printf("\n");
    printf("Indirect Blocks:\n");
    for (i = 0; i < N_IBLOCKS; i++) {
        printf("%d ", inode->iblocks[i]);
    }
    printf("\n");
    printf("\n");
}

void dumpInodeFreeList(FILE *in) {
    printf("Inode Free List:\n");

    inode *next = malloc(inodeSize);
    fseek(in, inodeInitial + superBlock->free_inode * inodeSize, SEEK_SET);
    fread(next, 1, inodeSize, in);
    while (next->next_inode >= 0) {
        dumpInode(next);
        fseek(in, inodeInitial + next->next_inode * inodeSize, SEEK_SET);
        fread(next, 1, inodeSize, in);
    }
    dumpInode(next);

    free(next);
}

void dumpDataFreeList(FILE *in) {
    printf("Data Free List:\n");
    printf("Head index: %d\n", superBlock->free_iblock);

    void *next = malloc(blockSize);
    fseek(in, dataInitial + superBlock->free_iblock * blockSize, SEEK_SET);
    fread(next, blockSize, 1, in);
    int value = *((int *) next);
    while (value >= 0) {
        fseek(in, dataInitial + value * blockSize, SEEK_SET);
        fread(next, blockSize, 1, in);
        printf("Next index: %d\n", value);
        value = *((int *) next);
    }
    printf("Next index: %d\n", value);

    free(next);
}

/**
 * This function print boot, super and all inode block content
 * Also print full free data and inode list
 * If the result is legal, at least the format of input file image is correct
 * @param inFile Pointer to a file need to be validated
 */
void validator(FILE *inFile) {
    rewind(inFile);
    void *buffer = malloc(blockSize);

    // dump boot block
    fread(buffer, blockSize, 1, inFile);
    dumpBootBlock(buffer);

    // dump super block
    superBlock = malloc(blockSize);
    fread(superBlock, blockSize, 1, inFile);
    dumpSuperBlock(superBlock);
    blockSize = (size_t) superBlock->size;

    // calculate initial address of three areas
    inodeInitial = (2 + superBlock->inode_offset) * blockSize;
    dataInitial = (2 + superBlock->data_offset) * blockSize;
    swapInitial = (2 + superBlock->swap_offset) * blockSize;

    // dump free lists
    dumpInodeFreeList(inFile);
    dumpDataFreeList(inFile);

    // dump all inodes
    printf("\nInode List:\n");
    int i;
    inode *inodeBuffer = malloc(inodeSize);
    size_t inodeCount = (superBlock->data_offset - superBlock->inode_offset) * blockSize / inodeSize;
    for (i = 0; i < inodeCount; i++) {
        fseek(inFile, inodeInitial + i * inodeSize, SEEK_SET);
        fread(inodeBuffer, 1, inodeSize, inFile);
        dumpInode(inodeBuffer);
    }
    free(inodeBuffer);

    free(buffer);
    free(superBlock);
}

/**
 * This function print separated files from input file image
 * Output files will be placed in sub-folder ./unpacked of execution directory
 * If the input file image is correct, output files can be normally open in Ubuntu system
 * Note that this function **cannot** handle files with second or third indirect blocks for convenience
 * @param inFile Pointer to a input file image
 */
void printFiles(FILE *inFile) {
    rewind(inFile);
    void *buffer = malloc(blockSize);

    // read boot block
    fread(buffer, blockSize, 1, inFile);

    // read super block
    superBlock = malloc(blockSize);
    fread(superBlock, blockSize, 1, inFile);
    blockSize = (size_t) superBlock->size;

    // calculate initial address of three areas
    inodeInitial = (2 + superBlock->inode_offset) * blockSize;
    dataInitial = (2 + superBlock->data_offset) * blockSize;
    swapInitial = (2 + superBlock->swap_offset) * blockSize;

    // dump inode free lists
    dumpInodeFreeList(inFile);

    int i, j, k;
    inode *inodeBuffer = malloc(inodeSize);
    size_t inodeCount = (superBlock->data_offset - superBlock->inode_offset) * blockSize / inodeSize;
    FILE *outFile;
    char name[50];
    int *inBuffer = malloc(blockSize);
    for (i = 0; i < inodeCount; i++) {
        fseek(inFile, inodeInitial + i * inodeSize, SEEK_SET);
        fread(inodeBuffer, 1, inodeSize, inFile);
        dumpInode(inodeBuffer);
        mkdir("unpacked", S_IRWXU);
        if (inodeBuffer->nlink > 0) {
            sprintf(name, "unpacked/file-%d", i);
            outFile = fopen(name, "wt");
            size_t dataCount = ((inodeBuffer->size - 1) / blockSize) + 1;
            for (j = 0; j < N_DBLOCKS; j++) {
                if (dataCount <= 0) {
                    break;
                }
                fseek(inFile, dataInitial + inodeBuffer->dblocks[j] * blockSize, SEEK_SET);
                fread(buffer, blockSize, 1, inFile);
                if (dataCount > 1) {
                    fwrite(buffer, blockSize, 1, outFile);
                } else {
                    fwrite(buffer, inodeBuffer->size % blockSize, 1, outFile);
                }
                dataCount--;
            }
            for (j = 0; j < N_IBLOCKS; j++) {
                if (dataCount <= 0) {
                    break;
                }
                fseek(inFile, dataInitial + inodeBuffer->iblocks[j] * blockSize, SEEK_SET);
                fread(inBuffer, blockSize, 1, inFile);
                for (k = 0; k < (blockSize / sizeof(int)); k++) {
                    if (dataCount <= 0) {
                        break;
                    }
                    fseek(inFile, dataInitial + inBuffer[k] * blockSize, SEEK_SET);
                    fread(buffer, blockSize, 1, inFile);
                    if (dataCount > 1) {
                        fwrite(buffer, blockSize, 1, outFile);
                    } else {
                        fwrite(buffer, inodeBuffer->size % blockSize, 1, outFile);
                    }
                    dataCount--;
                }
            }
            fclose(outFile);
        }
    }

    free(inBuffer);
    free(inodeBuffer);

    free(buffer);
    free(superBlock);
}

/*********************** From there, functions are parts of our defragmenter ***********************/

/**
 * This function read origin superblock and inode,
 * then copy them to output file as a kind of placeholder
 * After operation, the cursor of output file should at the beginning of data area
 * Notice this function should be called just after boot block have been written into out file
 * Also note that superblock must be initialized before calling it
 * @param out The output file pointer
 */
void copyInodes(FILE *in, FILE *out) {
    int i;
    void *buffer = malloc(blockSize);

    // copy origin superblock as placeholder
    fwrite(superBlock, blockSize, 1, out);

    // copy origin inodes to output file, also as placeholder
    for (i = superBlock->inode_offset; i < superBlock->data_offset; i++) {
        fseek(in, inodeInitial + i * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, in);
        fseek(out, inodeInitial + i * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, out);
    }

    free(buffer);
}

/**
 * This function sort free data block indexes,
 * and write them in ascending order into output file
 * @param inFile The input file pointer
 * @param outFile The output file pointer
 */
void writeFreeDataBlock(FILE *inFile, FILE *outFile) {
    size_t cnt = 0; // number of data region free blocks

    // count free data block number
    int i, next;
    fseek(inFile, dataInitial + superBlock->free_iblock * blockSize, SEEK_SET);
    fread(&next, 1, sizeof(int), inFile);
    cnt++;
    while (next >= 0) {
        fseek(inFile, dataInitial + next * blockSize, SEEK_SET);
        fread(&next, 1, sizeof(int), inFile);
        cnt++;
    }
    int *arr = malloc(sizeof(int) * (cnt + 1));

    // read inFile free block indexes
    cnt = 0;
    fseek(inFile, dataInitial + superBlock->free_iblock * blockSize, SEEK_SET);
    arr[cnt++] = superBlock->free_iblock;
    fread(arr + cnt, sizeof(int), 1, inFile);
    while (arr[cnt] >= 0) {
        fseek(inFile, dataInitial + arr[cnt] * blockSize, SEEK_SET);
        cnt++;
        fread(arr + cnt, sizeof(int), 1, inFile);
    }

    // write free blocks to output file in ascending order
    freeBlockIndex = dataBlockIndex;
    superBlock->free_iblock = freeBlockIndex;
    fseek(outFile, dataInitial + freeBlockIndex * blockSize, SEEK_SET);
    void *buffer = malloc(blockSize);
    memset(buffer, 0, blockSize);
    for (i = 0; i < cnt - 1; i++) {
        fseek(inFile, dataInitial + arr[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        *(int *) buffer = ++freeBlockIndex;
        fwrite(buffer, blockSize, 1, outFile);
    }

    fseek(inFile, dataInitial + arr[cnt - 1] * blockSize, SEEK_SET);
    fread(buffer, blockSize, 1, inFile);
    *(int *) buffer = -1;
    fwrite(buffer, blockSize, 1, outFile);
    freeBlockIndex++;

    free(arr);
    free(buffer);
}

/**
 * This function simply copy swap region from input to output file
 */
void writeSwapRegion(FILE *inFile, FILE *outFile) {
    if (fseek(outFile, swapInitial, SEEK_SET)) { // cannot direct to swap region
        d_error |= ERROR_CORRUPTED_SWAP_REGION;
        return;
    }

    void *buffer = malloc(blockSize);
    fseek(inFile, swapInitial, SEEK_SET);
    while (fread(buffer, blockSize, 1, inFile)) {
        fwrite(buffer, blockSize, 1, outFile);
    }

    free(buffer);
}

/**
 * This function read a I1 block, and print all blocks indexed by it to output file
 * The I1 block indicated by blk will be updated
 * Also decrease dataCount, which count for remaining data blocks for this file
 */
void writeIndirectBlock(int *blk, FILE *inFile, FILE *outFile, size_t *dataCount) {
    int i;
    void *buffer = malloc(blockSize);
    for (i = 0; i < (blockSize / sizeof(int)); i++) {
        if (*dataCount <= 0) {
            break;
        }

        fseek(inFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        blk[i] = dataBlockIndex++;
        fseek(outFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
        (*dataCount)--;
    }
    free(buffer);
}

/**
 * This function read a I2 block, and print all blocks indexed by it to output file
 * The I2 block indicated by blk will be updated
 * Also decrease dataCount, which count for remaining data blocks for this file
 */
void writeSecondIndirectBlock(int *blk, FILE *inFile, FILE *outFile, size_t *dataCount) {
    int i;
    void* buffer = malloc(blockSize);
    for (i = 0; i < (blockSize / sizeof(int)); i++) {
        if (*dataCount <= 0) {
            break;
        }

        fseek(inFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        blk[i] = dataBlockIndex++;
        writeIndirectBlock(blk, inFile, outFile, dataCount);
        fseek(outFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
    }
    free(buffer);
}

/**
 * This function read a I3 block, and print all blocks indexed by it to output file
 * The I3 block indicated by blk will be updated
 * Also decrease dataCount, which count for remaining data blocks for this file
 */
void writeThirdIndirectBlock(int *blk, FILE *inFile, FILE *outFile, size_t *dataCount) {
    int i;
    void* buffer = malloc(blockSize);
    for (i = 0; i < (blockSize / sizeof(int)); i++) {
        if (*dataCount <= 0) {
            break;
        }

        fseek(inFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        blk[i] = dataBlockIndex++;
        writeSecondIndirectBlock(blk, inFile, outFile, dataCount);
        fseek(outFile, dataInitial + blk[i] * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
    }
    free(buffer);
}

/**
 * This function read a inode, and print all blocks of this file to output
 * Also modify pointer fields in input inode
 */
void writeSingleFile(inode *inode, FILE *inFile, FILE *outFile) {
    int i;
    void *buffer = malloc(blockSize);
    size_t dataCount = ((inode->size - 1) / blockSize) + 1;

    // write direct blocks
    for (i = 0; i < N_DBLOCKS; i++) {
        if (dataCount <= 0) {
            break;
        }
        fseek(inFile, dataInitial + inode->dblocks[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        inode->dblocks[i] = dataBlockIndex++;
        fseek(outFile, dataInitial + inode->dblocks[i] * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
        dataCount--;
    }

    // write indirect blocks
    for (i = 0; i < N_IBLOCKS; i++) {
        if (dataCount <= 0) {
            break;
        }
        fseek(inFile, dataInitial + inode->iblocks[i] * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        //dumpIndexBlock(buffer);
        inode->iblocks[i] = dataBlockIndex++;
        writeIndirectBlock(buffer, inFile, outFile, &dataCount);
        fseek(outFile, dataInitial + inode->iblocks[i] * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
    }

    if (dataCount > 0) {
        d_error |= ERROR_DATA_BLOCK_LOST;
        perror("Not all blocks written!");
    }

    if (inode->i2block != 0) {
        fseek(inFile, dataInitial + inode->i2block, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        writeSecondIndirectBlock(buffer, inFile, outFile, &dataCount);
    }

    if (inode->i3block != 0) {
        fseek(inFile, dataInitial + inode->i3block, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        writeThirdIndirectBlock(buffer, inFile, outFile, &dataCount);
    }

    free(buffer);
}

/**
 * This function write all files into output file in defragmented manner one by one
 * Note the inodeCount should be given as argument
 */
void writeAllFiles(FILE *inFile, FILE *outFile, size_t inodeCount) {
    int i;
    inode *inodeBuffer = malloc(inodeSize);

    for (i = 0; i < inodeCount; i++) {
        fseek(inFile, inodeInitial + i * inodeSize, SEEK_SET);
        fread(inodeBuffer, inodeSize, 1, inFile);
        //dumpInode(inodeBuffer);
        if (inodeBuffer->nlink > 0) {
            writeSingleFile(inodeBuffer, inFile, outFile);

            fseek(outFile, inodeInitial + i * inodeSize, SEEK_SET);
            fwrite(inodeBuffer, inodeSize, 1, outFile);
        }
    }

    free(inodeBuffer);
}

/**
 * This function works as a remedy for sample file error
 * Specifically, it copy all blocks remained (not data block nor free block) into output file
 * If this function is called, an error tag will be set
 */
void dataRegionMender(FILE *inFile, FILE *outFile) {
    int i;
    int dataRegion = superBlock->swap_offset - superBlock->data_offset;
    void *buffer = malloc(blockSize);

    // 10135-10238 in test sample
    for (i = freeBlockIndex; i < dataRegion; i++) {
        d_error |= ERROR_CORRUPTED_FREE_DATA;
        fseek(inFile, dataInitial + i * blockSize, SEEK_SET);
        fread(buffer, blockSize, 1, inFile);
        fseek(outFile, dataInitial + i * blockSize, SEEK_SET);
        fwrite(buffer, blockSize, 1, outFile);
    }

    free(buffer);
}

int defragmenter(FILE *inFile, FILE *outFile) {
    void *buffer;
    d_error = ERROR_ALL_GREEN;
    buffer = malloc(blockSize);

    // simply copy boot block
    fread(buffer, blockSize, 1, inFile);
    //dumpBootBlock(buffer);
    fwrite(buffer, blockSize, 1, outFile);

    // read in the super block
    superBlock = malloc(blockSize);
    fread(superBlock, blockSize, 1, inFile);
    dumpSuperBlock(superBlock);
    blockSize = (size_t) superBlock->size;

    // calculate initial address of three regions
    inodeInitial = 1024 + superBlock->inode_offset * blockSize;
    dataInitial = 1024 + superBlock->data_offset * blockSize;
    swapInitial = 1024 + superBlock->swap_offset * blockSize;

    //dumpInodeFreeList(inFile);
    //dumpDataFreeList(inFile);

    // hold place for superblock and inodes
    copyInodes(inFile, outFile);

    // read used inode and write data blocks
    size_t inodeCount = (superBlock->data_offset - superBlock->inode_offset) * blockSize / inodeSize;
    dataBlockIndex = 0;
    writeAllFiles(inFile, outFile, inodeCount);

    writeFreeDataBlock(inFile, outFile);
    dataRegionMender(inFile, outFile);

    fseek(outFile, 512, SEEK_SET);
    fwrite(superBlock, 1, 512, outFile);

    writeSwapRegion(inFile, outFile);

    free(buffer);
    free(superBlock);

    return (d_error == ERROR_ALL_GREEN);
}
