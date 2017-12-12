#include "defrag.h"

size_t inodeInitial;
size_t dataInitial;
size_t swapInitial;

inode* inodeFreeHead;
inode* dataFreeHead;

superblock* superBlock;
size_t blockSize = 512;
size_t inodeSize = sizeof(inode); // size = 100

void dumpBootBlock(const char* bootBlock) {
    int i;
    printf("Boot Block Status:\n");
    for (i = 0; i < blockSize; i++) {
        putchar(*(bootBlock + i));
    }
    putchar('\n');
    putchar('\n');
}

void dumpSuperBlock(superblock* superBlock) {
    printf("Super Block Status:\n");
    printf("Size:         %d\n", superBlock->size);
    printf("Inode offset: %d\n", superBlock->inode_offset);
    printf("Data offset:  %d\n", superBlock->data_offset);
    printf("Swap offset:  %d\n", superBlock->swap_offset);
    printf("Free inode:   %d\n", superBlock->free_inode);
    printf("Free iblock:  %d\n", superBlock->free_iblock);
    printf("\n");
}

void dumpInode(inode* inode) {
    int i;
    printf("Inode Status:\n");
    printf("Next inode:   %d\n", inode->next_inode);
    printf("Protect:      %X\n", inode->next_inode);
    printf("Number Link:  %d\n", inode->nlink);
    printf("File Size:    %d\n", inode->size);
    printf("User Id:      %d\n", inode->uid);
    printf("Group Id:     %d\n", inode->gid);
    printf("Change Time:  %d\n", inode->ctime);
    printf("Mordify Time: %d\n", inode->mtime);
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

void dumpDataBlock(const char* blk) {
    int i;
    printf("Data Block Content:\n");
    for (i = 0; i < blockSize; i++) {
        putchar(*(blk + i));
    }
    putchar('\n');
    putchar('\n');
}

void dumpInodeFreeList(FILE* in) {
    printf("Inode Free List:\n");

    inode* next = malloc(inodeSize);
    fseek(in, inodeInitial + superBlock->free_inode * inodeSize, SEEK_SET);
    fread(next, 1, inodeSize, in);
    while (next->next_inode != -1) {
        dumpInode(next);
        fseek(in, inodeInitial + next->next_inode * inodeSize, SEEK_SET);
        fread(next, 1, inodeSize, in);
    }
    dumpInode(next);

    free(next);
}

void dumpDataFreeList(FILE* in) {
    printf("Data Free List:\n");

    void* next = malloc(blockSize);
    fseek(in, dataInitial + superBlock->free_iblock * blockSize, SEEK_SET);
    fread(next, 1, blockSize, in);
    int value = *((int*)next);
    while (value >= 0) {
        fseek(in, dataInitial + value * blockSize, SEEK_SET);
        fread(next, 1, blockSize, in);
        printf("Next index: %d\n", value);
        value = *((int*)next);
    }
}

/**
 * This function hold place for superblock and inode area
 * Notice this function should called after boot block have been written into out file
 * Also note that superblock must be initialized before calling it
 * @param out The output file pointer
 */
void placeholder(FILE* out) {
    int i;
    // create a zero-filled block
    void* zero = malloc(blockSize);
    memset(zero, 0, blockSize);

    // placeholder for superblock
    fwrite(zero, 1, blockSize, out);

    // placeholder for inodes
    for (i = superBlock->free_inode; i < superBlock->data_offset; i++) {
        fwrite(zero, 1, blockSize, out);
    }
}

int defragmenter(FILE* inFile, FILE* outFile) {
    void* buffer;
    buffer = malloc(blockSize);

    // simply copy boot block
    fread(buffer, 1, blockSize, inFile);
    dumpBootBlock(buffer);
    fwrite(buffer, 1, blockSize, outFile);

    // read in the super block
    superBlock = malloc(blockSize);
    fread(superBlock, 1, blockSize, inFile);
    dumpSuperBlock(superBlock);

    // calculate initial address of three areas
    inodeInitial = (2 + superBlock->inode_offset) * blockSize;
    dataInitial  = (2 + superBlock->data_offset) * blockSize;
    swapInitial  = (2 + superBlock->swap_offset) * blockSize;

    // calculate the header pointer to two free lists
    fseek(inFile, inodeInitial + superBlock->free_inode * inodeSize, SEEK_SET);
    fread(buffer, 1, blockSize, inFile);
    inodeFreeHead = (inode*)buffer;
    dumpInode(inodeFreeHead);

    fseek(inFile, dataInitial + superBlock->free_iblock * blockSize, SEEK_SET);
    fread(buffer, 1, blockSize, inFile);
    dataFreeHead = (inode*)buffer;
    dumpDataBlock((char*)dataFreeHead);
    fseek(inFile, dataInitial, SEEK_SET);
    fread(buffer, 1, blockSize, inFile);
    dumpDataBlock(buffer);

    fseek(inFile, inodeInitial, SEEK_SET);
    int i;
    for (i = 0; i < 20; i++) {
        fseek(inFile, inodeInitial + i*inodeSize, SEEK_SET);
        fread(buffer, 1, blockSize, inFile);
        dumpInode(buffer);
    }

    dumpInodeFreeList(inFile);
    dumpDataFreeList(inFile);

    // hold place for superblock and inodes
    placeholder(outFile);

    return 0;
}