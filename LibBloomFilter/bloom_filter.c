#include "bloom_filter.h"

// library implementing a standard bloom filter for learning purposes

static void _set_bit(unsigned char* s, size_t sBitCount, size_t index, bool value);
static unsigned char _get_bit(unsigned char* s, size_t sBitCount, size_t index);
static bool _check_present(BloomFilter* filter, int indexes[filter->hashFunctionCount]);
static int _hash_all_str(const char* key, size_t hashFunctionCount, size_t bitCount, int indexes[hashFunctionCount]);
static int _hash_all_int(int key, size_t hashFunctionCount, size_t bitCount, int indexes[hashFunctionCount]);
static unsigned int _hash_string(const char* s);
static unsigned int _hash_int(int x);

BloomFilter* bf_create_fixed(unsigned int expectedElementCount) 
{
    if (expectedElementCount <= 0) {
        return NULL;
    }

    BloomFilter* filter = malloc(sizeof(BloomFilter));
    const int n = expectedElementCount;
    if (filter == NULL) {
        return NULL;
    }

    // assume a p value of 0.001 or 0.1%
    const double p = 0.001;

    // bit size of filter
    const size_t m = (int)round(-(n * log(p) / (pow(log(2), 2))));
    const size_t k = (int)round(m/n * log(2));

    filter->buckets = calloc((int)round(m / BYTE_SIZE) + BYTE_SIZE, sizeof(unsigned char));
    filter->hashFunctionCount = k;
    filter->totalBitCount = m;

    return filter;
}

int bf_set_str(BloomFilter* filter, const char* key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_str(key, filter->hashFunctionCount, filter->totalBitCount, indexes);

    for (size_t i = 0; i < filter->hashFunctionCount; i++) 
    {
        _set_bit(filter->buckets, filter->totalBitCount, indexes[i], 1);
    }

    return EXIT_SUCCESS;   
}

static int _hash_all_str(const char* key, size_t hashFunctionCount, size_t bitCount, int indexes[hashFunctionCount])
{
    // store 64 bit number as a string
    char salt[21 + 1];
    snprintf(salt, sizeof(salt), "%lld", SALT);

    char saltedKey[8 + strlen(key) + 1];
    snprintf(saltedKey, sizeof(saltedKey), "%s%s", key, salt);

    for (size_t i = 0; i < hashFunctionCount; i++) 
    {
        indexes[i] = (_hash_string(key)  + i * _hash_string(saltedKey)) % bitCount;
    }
    return EXIT_SUCCESS;
}

bool bf_get_str(BloomFilter* filter, const char* key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_str(key, filter->hashFunctionCount, filter->totalBitCount, indexes);
    return _check_present(filter, indexes);
}

int bf_set_chr(BloomFilter* filter, char key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_int(key, filter->hashFunctionCount, filter->totalBitCount, indexes);

    for (size_t i = 0; i < filter->hashFunctionCount; i++) 
    {
        _set_bit(filter->buckets, filter->totalBitCount, indexes[i], 1);
    }
    
    return EXIT_SUCCESS;
}

bool bf_get_chr(BloomFilter* filter, char key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_int(key, filter->hashFunctionCount, filter->totalBitCount, indexes);    
    return _check_present(filter, indexes);
}

int bf_set_int(BloomFilter* filter, int key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_int(key, filter->hashFunctionCount, filter->totalBitCount, indexes);

    for (size_t i = 0; i < filter->hashFunctionCount; i++) 
    {
        _set_bit(filter->buckets, filter->totalBitCount, indexes[i], 1);
    }
    
    return EXIT_SUCCESS;   
}

bool bf_get_int(BloomFilter* filter, int key) 
{
    int indexes[filter->hashFunctionCount];
    _hash_all_int(key, filter->hashFunctionCount, filter->totalBitCount, indexes);    
    return _check_present(filter, indexes);    
}

int bf_free(BloomFilter* filter) 
{
    free(filter->buckets);
    filter->buckets = NULL;
    free(filter);
    filter = NULL;
    return EXIT_SUCCESS;
}

static bool _check_present(BloomFilter* filter, int indexes[filter->hashFunctionCount]) 
{
    for (size_t i = 0; i < filter->hashFunctionCount; i++) 
    {
        unsigned int bit = _get_bit(filter->buckets, filter->totalBitCount, indexes[i]);
        if (bit == (unsigned int)-1) 
        {
            // error 
            printf("Error\n");
            return false;
        }

        if (bit == 0) 
        {
            return false;
        }
    }
    return true;
}

static int _hash_all_int(int key, size_t hashFunctionCount, size_t bitCount, int indexes[hashFunctionCount]) 
{
    for (size_t i = 0; i  < hashFunctionCount; i++) 
    {
        indexes[i] = (_hash_int(key) + i * _hash_int(key + SALT)) % bitCount;
    }

    return EXIT_SUCCESS;
}

static void _set_bit(unsigned char* s, size_t sBitCount, size_t index, bool value) 
{
    if ((int)index < 0  || index >= sBitCount) 
    { 
        return;
    }

    int _index = index / BYTE_SIZE;
    int offset = index % BYTE_SIZE;

    switch(value)
    {
        case 0:
            s[_index] &= ~(1 << offset);
            break;
        case 1:
            s[_index] |= (1 << offset);
            break; 
    }

    return;
}

static unsigned char _get_bit(unsigned char* s, size_t sBitCount, size_t index) 
{
    if ((int)index < 0  || index >= sBitCount) 
    {
        return -1;
    }

    int _index = index / BYTE_SIZE;
    int offset = index % BYTE_SIZE;

    return (s[_index] & (1 << offset)) != 0;
}

// djb2 hashing algorithm by Dan Bernstein
static unsigned int _hash_string(const char* s)
{
	// hash(i - 1) * 33 ^ str[i];
	unsigned long hash = 5381;
	while (*s)
	{
		hash = hash * 33 ^ *s;
		s++;
	}
	return hash;
}


static unsigned int _hash_int(int x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}


const struct LibBloomFilter_l LibBloomFilter = {
	.create = bf_create_fixed,
	.set_str = bf_set_str,
    .get_str = bf_get_str,
    .set_chr = bf_set_chr,
    .get_chr = bf_get_chr,
    .set_int = bf_set_int,
    .get_int = bf_get_int,
    .free = bf_free
};
