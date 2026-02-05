#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <linux/time.h>
#include "cuckoo_filter.h"

#define BUFFER_SIZE 65536  // 64KB buffer for efficient reading
#define MAX_LINE_LENGTH 16 // Max length for "xxx.xxx.xxx.xxx\n"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip_address_file>\n", argv[0]);
        return 1;
    }

    CuckooFilter* filter = LibCuckooFilter.create(10000000, DEFAULT_BUCKET_DEPTH, 7);
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
    puts("Inserting Addresses");
    unsigned char value = 1UL;
    
    struct timespec start, end;
    unsigned long insertedCount = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) {
            LibCuckooFilter.set_str(filter, line);
            insertedCount++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%ld addresses insertion elapsed time: %.6f seconds\n", insertedCount, elapsed);

    fclose(fp);

    unsigned long foundCount = 0;
    unsigned long falseNegativeCount = 0;
    unsigned long totalReads = 0;
    puts("Running Lookup Test 1");
    fp = fopen("present.txt", "r");

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) {
            totalReads++;
            if (LibCuckooFilter.get_str(filter, line)) {
                foundCount++;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Lookup Test 1 Elapsed Time: %.6f seconds\n", elapsed);
    printf("Found: %ld\n", foundCount);
    printf("False negatives: %ld\n", totalReads - foundCount);
    fclose(fp);

    unsigned long test2FalsePositiveCount = 0;
    unsigned long test2NotFoundCount = 0;

    puts("Running Lookup Test 2");
    unsigned long falsePositiveCount = 0;
    unsigned long notFoundCount = 0;
    fp = fopen("not_present.txt", "r");

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strcmp(line, "") != 0) {
            if (LibCuckooFilter.get_str(filter, line)) {
                falsePositiveCount++;
            } else {
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
    free(buffer);
    
    LibCuckooFilter.free(filter);
    
    return 0;
}
