#include <unistd.h>
#include "defrag.h"

/**
 * This function concatenate "-defrag" suffix to the file name given by argument src
 * Note that if the source file name have a filename extension, the suffix will be put just before it
 * In addition, the space for result is allocated by malloc, remember to ***free*** it.
 * E.g: <filename>.<extension> -> <filename>-defrag.<extension>
 * @param src The source file name
 * @return A pointer to the result filename, need to be freed after use
 */
char* generateFileName(char* src) {
    char* suffix = "-defrag";
    char* dst = malloc(sizeof(char)*(strlen(src) + strlen(suffix) + 1));
    memset(dst, 0, sizeof(char)*(strlen(src) + strlen(suffix) + 1));

    size_t pos = strrchr(src, '.') - src; // find the dot position of filename extension
    strncpy(dst, src, pos); // copy prefix file name to dst
    strcat(dst, suffix);    // concatenate with the "-defrag" suffix
    strcat(dst, src + pos); // concatenate with the filename extension

    return dst;
}

void validation(char* fileName) {
    FILE* inFile = fopen(fileName, "r");
    printf("Validating file %s\n", fileName);
    printFiles(inFile);
    validator(inFile);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        perror("Usage: defrag data-file");
        exit(1);
    }

    validation(argv[1]);

    FILE* inFile;
    FILE* outFile;
    char *outName = generateFileName(argv[1]);

    inFile = fopen(argv[1], "r");
    outFile = fopen(outName, "w+");

    if (inFile == NULL) {
        perror("Input file not exists.");
        exit(1);
    }
    if (outFile == NULL) {
        perror("Cannot create output file.");
        exit(1);
    }

    if (defragmenter(inFile, outFile) != 0) {
        perror("Cannot defrag input file, this file may be corrupted.");
        exit(1);
    }

    free(outName);
    fclose(inFile);
    fclose(outFile);
    return 0;
}