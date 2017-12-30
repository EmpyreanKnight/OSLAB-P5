#include <unistd.h>
#include "defrag.h"

/**
 * This function concatenate "-defrag" suffix to the file name given by argument src
 * Note that if the source file name have a filename extension, the suffix will be put just before it
 * In addition, the space for result is allocated by malloc, remember to ***free*** it.
 * E.g: <filename>(.<extension>) -> <filename>-defrag(.<extension>)
 * @param src The source file name
 * @return A pointer to the result filename, need to be freed after use
 */
char* generateFileName(char* src) {
    char* suffix = "-defrag";
    char* dst = malloc(sizeof(char)*(strlen(src) + strlen(suffix) + 1));
    memset(dst, 0, sizeof(char)*(strlen(src) + strlen(suffix) + 1));

    char* pos = strrchr(src, '.'); // find the dot position of filename extension
    if (pos == NULL) { // no extension
        strcpy(dst, src);
        strcat(dst, suffix);
    } else {
        strncpy(dst, src, pos - src); // copy prefix file name to dst
        strcat(dst, suffix);          // concatenate with the "-defrag" suffix
        strcat(dst, pos);             // concatenate with the filename extension
    }
    return dst;
}

void validation(FILE* inFile) {
    printf("Validating file...\n");
    printFiles(inFile);
    validator(inFile);
    fclose(inFile);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        perror("Usage: defrag data-file");
        exit(1);
    }

    FILE* inFile;
    inFile = fopen(argv[1], "r");
    if (inFile == NULL) {
        perror("Input file not exists.");
        exit(1);
    }

    //validation(inFile);

    FILE* outFile;
    char *outName = generateFileName(argv[1]);
    outFile = fopen(outName, "w+");
    if (outFile == NULL) {
        perror("Cannot create output file.");
        exit(1);
    }

    if (defragmenter(inFile, outFile) != 0) {
        perror("Cannot defrag input file, this file may be corrupted.");
        exit(1);
    }

	printf("Defragmentation Succeed!\n");
	printf("Output file name: %s\n", outName);
    free(outName);
    fclose(inFile);
    fclose(outFile);
    return 0;
}
