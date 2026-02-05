#ifndef CUCKOO_FILTER_H
#define CUCKOO_FILTER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define DEFAULT_BUCKET_DEPTH 4 // common b value used in Cuckoo filters to achieve a value of 95% load factor before insertions fail
#define DEFAULT_FINGERPRINT_BIT_COUNT 7 // ideal f value used in the original paper
#define OPTIMAL_LOAD_FACTOR 0.95
#define MAX_KICK_COUNT 500

typedef struct {
    uint64_t* buckets;
    size_t storedFingerprintCount; // used for calculating load factor
    size_t bucketCount; // also known as m 
    size_t bucketDepth; // the number of fingerprints per bucket known as b
    size_t fingerprintSize; // bit count for fingerprints also known as f 
} CuckooFilter;

CuckooFilter* cf_create(size_t expectedElementCount, size_t bucketDepth, size_t fingerprintBitCount);
bool cf_get_str(CuckooFilter* filter, const char* key);
bool cf_get_chr(CuckooFilter* filter, char key);
bool cf_get_int(CuckooFilter* filter, int key);
int cf_set_str(CuckooFilter* filter, const char* key);
int cf_set_chr(CuckooFilter* filter, char key);
int cf_set_int(CuckooFilter* filter, int key);
int cf_remove_str(CuckooFilter* filter, const char* key);
int cf_remove_chr(CuckooFilter* filter, char key);
int cf_remove_int(CuckooFilter* filter, int key);
int cf_free(CuckooFilter* filter);

struct LibCuckooFilter_l {
    /**
     * Instantiate a new cuckoo filter 
     * @param expectedElementCount number of unique elements the filter expects to hold
     */
    CuckooFilter* (*create)(size_t expectedElementCount, size_t bucketDepth, size_t fingerprintBitCount);
    int (*set_str)(CuckooFilter* filter, const char* key);
    int (*set_chr)(CuckooFilter* filter, char key);
    int (*set_int)(CuckooFilter* filter, int key); 

    bool (*get_str)(CuckooFilter* filter, const char* key);
    bool (*get_chr)(CuckooFilter* filter, char key);
    bool (*get_int)(CuckooFilter* filter, int key);
    int (*remove_str)(CuckooFilter* filter, const char* key);
    int (*remove_chr)(CuckooFilter* filter, char key);
    int (*remove_int)(CuckooFilter* filter, int key);
    CuckooFilter* (*rebuild)(CuckooFilter* filter, unsigned int newExpectedElementCount);
    int (*free)(CuckooFilter* filter);
    
};

extern const struct LibCuckooFilter_l LibCuckooFilter;

#endif
