#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <linux/time.h>
#include "hash_table.h"


#define BUFFER_SIZE 65536  // 64KB buffer for efficient reading
#define MAX_LINE_LENGTH 16 // Max length for "xxx.xxx.xxx.xxx\n"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip_address_file>\n", argv[0]);
        return 1;
    }

    HashTable* table = LibHashTable.create(STRING, 10000000, sizeof(unsigned char), free);
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }
    
    // Set up buffered reading
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    if (setvbuf(fp, buffer, _IOFBF, BUFFER_SIZE) != 0) {
        fprintf(stderr, "Warning: Failed to set buffer\n");
    }
    
    char line[MAX_LINE_LENGTH];
    unsigned char val = 1;
    
    puts("Inserting addresses into hash table");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) 
        {
            LibHashTable.insert_str(table, line, &val);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Insertion elapsed time: %.06f seconds\n", elapsed );

    fclose(fp);

    unsigned long foundCount = 0;
    unsigned long falseNegativeCount = 0;
    puts("Running Lookup Test 1");
    fp = fopen("present.txt", "r");

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) 
        {
            if (LibHashTable.get_str(table, line) != NULL)
            {
                foundCount++;
            }
            else
            {
                falseNegativeCount++;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Lookup Test 1 Elapsed Time: %.6f seconds\n", elapsed);

    printf("Found: %ld\n", foundCount);
    printf("Missed: %ld\n", falseNegativeCount);

    fclose(fp);

    puts("Running Lookup Test 2");

    fp = fopen("not_present.txt", "r");
    unsigned long falsePositiveCount = 0;
    unsigned long notFoundCount = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) 
        {
            if (LibHashTable.get_str(table, line) != NULL)
            {
                falsePositiveCount++;
            }
            else
            {
                notFoundCount++;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + 
            (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Lookup Test 2 Elapsed Time: %.6f seconds\n", elapsed);
    printf("True Negatives: %ld\n", notFoundCount);
    printf("False Positives: %ld\n", falsePositiveCount);

    fclose(fp);
    free(buffer);

    return 0;
}
