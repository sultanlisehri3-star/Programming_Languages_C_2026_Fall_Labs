// week5_task1_file_io.c
// Name: Sehri  Surname: Sultanli  Student ID: 251ADB093
//
// Task 1: Read and write data from text files
// Week 5 – Files & Modular Programming
//
// Build: make lab5   Run: ./bin/week5_task1_file_io

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_FILENAME "data.txt"
#define LINE_LEN 256

int main(int argc, char *argv[]) {
    FILE *fp = NULL;
    const char *filename = DEFAULT_FILENAME;
    char line[LINE_LEN];
    int line_count = 0;

    // BONUS: use the file name from the command line if given
    if (argc > 1) {
        filename = argv[1];
    }

    // 1. Print message and open the file for writing
    printf("Writing lines to %s...\n", filename);
    fp = fopen(filename, "w");

    // 2. Check that the file was opened successfully
    if (fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }

    // 3. Write 2 lines of text to the file
    fprintf(fp, "Hello, file I/O in C!\n");
    fprintf(fp, "This is another line.\n");

    // 4. Close the file
    fclose(fp);

    // 5. Open the file again for reading and check for NULL
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }

    // 6. Read line by line and print each line (BONUS: count lines)
    printf("Reading contents:\n");
    while (fgets(line, LINE_LEN, fp) != NULL) {
        printf("%s", line);
        line_count++;
    }

    // 7. Close the file
    fclose(fp);

    // BONUS: print the number of lines read
    printf("Lines read: %d\n", line_count);

    return EXIT_SUCCESS;
}