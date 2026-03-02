#include "hash_table.h"

// implementation of a basic hash table with separate chaining for learning purposes

static int _ht_free_bucket(LinkedList* list, void free_item(void*));
static KeyValue* _ht_create_pair(const void* key, const void* value, size_t keySize, size_t valueSize);
static int _ht_delete_pair(KeyValue* pair, void free_item(void*));
static int _ht_delete_from_bucket(LinkedList* bucket, int position, void free_item(void*));

// insert data into specific bucket (index) in hash table
// performs separate chaining to resolve collisions
static int _ht_insert(HashTable* table, int index, KeyValue* pair);
static int _ht_delete(HashTable* table, const void* key, int index, void free_item(void*),
			int compare_key(const void* a, const void* b));
static int _resize(HashTable* table);
size_t _set_type(HashType type);
static unsigned int _ht_hash_string(const char* s);
static unsigned int _ht_hash_int(int x);
static int _compare_pair_int(const void* _pair, const void* _key);
static int _compare_pair_str(const void* _pair, const void* _key);
static int _compare_pair_chr(const void* _pair, const void* _key);
static KeyValue* _ht_get(LinkedList* bucket, const void* key, int (*compare)(const void *, const void *));

// creates a hash table
HashTable* ht_create(HashType keyType, const int bucketCount, const size_t dataSize, void free_function(void*))
{
	HashTable* table = (HashTable* )malloc(sizeof(HashTable));
	if (table == NULL)
	{
		return NULL;
	}

	// set type
	table->type = keyType;
	table->bucketCount = bucketCount;
	table->storedKeyCount = 0;

	table->free = free_function != NULL ? free_function : free;

	table->keySize = _set_type(keyType);
	table->dataSize = dataSize;

	// create buckets in hash table
	// generate static array
	table->buckets = (LinkedList**)malloc(sizeof(LinkedList*) * bucketCount);

	// populate array with linked lists
	for (int i = 0; i < bucketCount; i++)
	{
		// create linked list of key value structs
		table->buckets[i] = LibLinkedList.create(NULL, 0, sizeof(KeyValue));
		if (table->buckets[i] == NULL)
		{
			return NULL;
		}
	}

	return table;
}

int ht_insert_str(HashTable* table, const char* key, const void* value)
{
	const int index = _ht_hash_string(key) % table->bucketCount;

	KeyValue* _exist = ht_get_str(table, key);
	if (_exist != NULL)
	{
		table->buckets[index]->itemCount++;
		return EXIT_SUCCESS;
	}

	KeyValue* pair = _ht_create_pair(key, value, strlen(key) + 1, table->dataSize);
	_ht_insert(table, index, pair);
	free(pair);
	pair = NULL;
	return EXIT_SUCCESS;
}

int ht_insert_int(HashTable* table, int key, const void* value)
{
	int bucketIndex = _ht_hash_int(key) % table->bucketCount;

	KeyValue* _exist = ht_get_int(table, key);
	if (_exist != NULL)
	{
		table->buckets[bucketIndex]->itemCount++;
		return EXIT_SUCCESS;
	}

	KeyValue* pair = _ht_create_pair(&key, value, sizeof(int), table->dataSize);
	_ht_insert(table, bucketIndex, pair);
	free(pair);
	pair = NULL;
	return EXIT_SUCCESS;
}

int ht_insert_chr(HashTable* table, char key, const void* value)
{
	int bucketIndex = _ht_hash_int((int)key) % table->bucketCount;

	KeyValue* _exist = ht_get_int(table, (int)key);
	if (_exist != NULL)
	{
		table->buckets[bucketIndex]->itemCount++;
		return EXIT_SUCCESS;
	}

	KeyValue* pair = _ht_create_pair(&key, value, sizeof(char), table->dataSize);
	
	_ht_insert(table, bucketIndex, pair);
	free(pair);
	pair = NULL;
	return EXIT_SUCCESS;
}

static int _ht_insert(HashTable* table, const int index, KeyValue* pair)
{
	table->storedKeyCount++;
	int insertRes = LibLinkedList.insert(table->buckets[index], (void*)pair, -1);
	if (insertRes == EXIT_FAILURE) 
	{
		return EXIT_FAILURE;
	}
	double loadFactor = table->storedKeyCount / table->bucketCount;
	if (loadFactor >= 0.75) 
	{
		_resize(table);
	}

	return EXIT_SUCCESS;
}

KeyValue* ht_get_str(HashTable* table, const char* key)
{
	const int index = _ht_hash_string(key) % table->bucketCount;

	if (table->buckets[index] == NULL)
	{
		return NULL;
	}

	LinkedList* bucket = table->buckets[index];
	return _ht_get(bucket, key, _compare_pair_str);
}

KeyValue* ht_get_int(HashTable* table, int key)
{
	int index = _ht_hash_int(key) % table->bucketCount;

	if (table->buckets[index] == NULL)
	{
		puts("Could not find");
		return NULL;
	}

	LinkedList* bucket = table->buckets[index];
	return _ht_get(bucket, &key, _compare_pair_int);
}


KeyValue* ht_get_chr(HashTable* table, char key)
{
	int index = _ht_hash_int(key) % table->bucketCount;

	if (table->buckets[index] == NULL)
	{
		puts("Could not find");
		return NULL;
	}

	LinkedList* bucket = table->buckets[index];
	return _ht_get(bucket, &key, _compare_pair_chr);
}

static KeyValue* _ht_get(LinkedList* bucket, const void* key, int (*compare)(const void *, const void *))
{
	KeyValue* ret = (KeyValue*)LibLinkedList.search(bucket, key, compare);
	return ret;
}

// possible implement new method in linked list for deleting by key
static int _ht_delete(HashTable* table, const void* key, int index, void free_item(void*),
					int compare_key(const void* a, const void* b))
{
	if (free_item == NULL)
	{
		free_item = free;
	}

	if (compare_key == NULL)
	{
		return EXIT_FAILURE;
	}

	LinkedList* bucket = table->buckets[index];
	ListNode* current = bucket->head;

	int position = 0;
	while (current != NULL)
	{
		KeyValue* pair = (KeyValue*)current->value;
		if (compare_key(pair->key, key) == EXIT_SUCCESS)
		{
			// return LibLinkedList.delete(bucket, position, free_item);
			return _ht_delete_from_bucket(bucket, position, free_item);
		}
		position++;
		current = current->next;
	}

	return EXIT_FAILURE;
}

static int _ht_delete_from_bucket(LinkedList* bucket, int position, void free_item(void*))
{
    if (free_item == NULL)
    {
        free_item = free;
    }

    if (bucket->itemCount == 1)
    {
		_ht_delete_pair((KeyValue*)bucket->tail->value, free_item);
        free(bucket->tail);
        bucket->tail = NULL;
        bucket->head = NULL;
        bucket->itemCount = 0;
        return EXIT_SUCCESS;
    }

    if (position == 0 || bucket->itemCount == 1)
    {
        ListNode* temp = bucket->head;
        bucket->head = temp->next;
        if (bucket->head == NULL) 
        {
            bucket->tail = NULL;
        }
        
		_ht_delete_pair((KeyValue*)temp->value, free_item);
        free(temp);
        temp = NULL;
    }
    else if (position == -1)
    {   
        ListNode* current = bucket->head;
        // get node before tail
        while (current->next != bucket->tail) 
        {
            current = current->next;
        }
		_ht_delete_pair((KeyValue*)bucket->tail->value, free_item);
        free(bucket->tail);
        bucket->tail = current;
        bucket->tail->next = NULL;
    }
    else 
    {
        ListNode* current = bucket->head;
        
        // travel to node that comes before index
        for (int i = 0; i < position - 1; i++)
        {
            if (current->next == NULL)
            {
                break;
            }
            current = current->next;
        }

        // store node after index
        ListNode* temp = current->next;
        // isolate node to be freed by connect node at index with node after node to be freed
        current->next = current->next->next;

		_ht_delete_pair((KeyValue*)temp->value, free_item);
        free(temp);
        temp = NULL;
    }

    bucket->itemCount--;
    return EXIT_SUCCESS;
}

static int _ht_delete_pair(KeyValue* pair, void free_item(void*))
{	
	free_item(pair->data);
	pair->data = NULL;
	free(pair->key);
	pair->key = NULL;
	free(pair);
	pair = NULL;
	return EXIT_SUCCESS;
}

int ht_delete_str(HashTable* table, const char* key, void free_item(void*))
{
	int index = _ht_hash_string(key) % table->bucketCount;
	return _ht_delete(table, key, index, free_item, compare_str);
}

int ht_delete_int(HashTable* table, int key, void free_item(void*))
{
	int bucketIndex = _ht_hash_int(key) % table->bucketCount;
	return _ht_delete(table, &key, bucketIndex, free_item, compare_int);
}

int ht_delete_chr(HashTable* table, char key, void free_item(void*))
{
	return ht_delete_int(table, (int)key, free_item);
}

int ht_free(HashTable* table, void free_item(void*))
{
	if (free_item == NULL)
	{
		free_item = free;
	}

	for (int i = 0; i < table->bucketCount; i++)
	{
		_ht_free_bucket(table->buckets[i], free_item);
	}

	free(table->buckets);
	table->buckets = NULL;
	free(table);
	table = NULL;
	return EXIT_SUCCESS;
}

// modified version of the linked list free
// function but adapted to expect KeyValue structs
// whilst allowing the use of a user defined free function
static int _ht_free_bucket(LinkedList* list, void free_item(void*))
{
	if ((list)->head == NULL)
	{
		free(list);
		list = NULL;
		return EXIT_SUCCESS;
	}

	ListNode* previous = list->head;
	KeyValue* pair = (KeyValue*)previous->value;

	list->head = list->head->next;
	while (list->head != NULL)
	{
		_ht_delete_pair(pair, free_item);
		// free key value pair
		free(previous);
		previous = list->head;

		pair = (KeyValue* )list->head->value;
		list->head = list->head->next;
	}

	// free final just keep this the same idk why it break when changed
	free(pair->key);
	pair->key = NULL;
	free_item(pair->data);
	pair->data = NULL;

	free_item(previous->value);
	previous->value = NULL;
	free(previous);
	previous = NULL;
	free(list);
	list = NULL;
	return EXIT_SUCCESS;
}

int ht_print_keys(HashTable* table, void print(const void*))
{
	puts("---------");
	for (int i = 0; i < table->bucketCount; i++)
    {
		if (table->buckets[i] == NULL)
		{
			continue;
		}
        if (table->buckets[i]->head == NULL)
        {
            puts("NULL");
            continue;
        }
        LibLinkedList.print(table->buckets[i], print);
    }
	puts("---------");
	return EXIT_SUCCESS;
}


static KeyValue* _ht_create_pair(const void* key, const void* value, size_t keySize, 
								size_t valueSize)
{
	KeyValue* pair = (KeyValue*)malloc(sizeof(KeyValue));
	if (pair == NULL)
	{
		return NULL;
	}

	pair->data = (void*)malloc(valueSize);
	pair->key = (void*)malloc(keySize);

	if (pair->data == NULL || pair->key == NULL)
	{
		free(pair);
		pair = NULL;
		return NULL;
	}

	memmove(pair->data, value, valueSize);
	memmove(pair->key, key, keySize);
	return pair;
}

static int _compare_pair_str(const void* _pair, const void* _key)
{
	KeyValue* pair = (KeyValue*)_pair;
	return compare_str(pair->key, _key);
}

static int _compare_pair_int(const void* _pair, const void* _key)
{
	KeyValue* pair = (KeyValue*)_pair;
	int* key =  (int*)_key; 
	return compare_int(pair->key, key);
}

static int _compare_pair_chr(const void* _pair, const void* _key)
{
	KeyValue* pair = (KeyValue*)_pair;
	char* key = (char*)_key;
	return compare_chr(pair->key, key);
}

static int _resize(HashTable* table)
{
	size_t newSize = table->bucketCount * 2;
	LinkedList** resized = malloc(sizeof(LinkedList*) * newSize);

	if (resized == NULL) 
	{
		return EXIT_FAILURE;
	}

	for (int i = 0; i < newSize; i++) 
	{
		resized[i] = LibLinkedList.create(NULL, 0, sizeof(KeyValue));
	}

	// rehash existing elements to move to new table
	for (int i = 0; i < table->bucketCount; i++)
	{
		LinkedList* currentBucket = table->buckets[i];
		ListNode* current = currentBucket->head;

		while (current != NULL)
		{
			KeyValue* pair = (KeyValue*)(current->value);
			KeyValue* insertPair = _ht_create_pair(pair->key, pair->data, 
											table->keySize, table->dataSize);
			unsigned int insertBucket = 0;
			switch (table->type)
			{
				case STRING:
					insertBucket = _ht_hash_string((char*)pair->key) % newSize;
					break;
				case INT:
					insertBucket = _ht_hash_int(*(int*)pair->key) % newSize;
					break;
				case CHAR:
					insertBucket = _ht_hash_int((int)(*(char*)pair->key)) % newSize;
					break;
				default:
					return EXIT_FAILURE;
			}

		
			LibLinkedList.insert(resized[insertBucket], (void*)insertPair, -1);
			free(insertPair);
			
			current = current->next;
		}
	}

	for (int i = 0; i < table->bucketCount; i++)
	{
		_ht_free_bucket(table->buckets[i], table->free);
	}

	free(table->buckets);
	table->bucketCount = newSize;
	table->buckets = resized;
	return EXIT_SUCCESS;
}

// returns the correct memory size based on hash type
size_t _set_type(HashType type)
{
	switch (type)
	{
	case 1:
		return sizeof(char*);
	case 2:
		return sizeof(int);
	case 3:
		return sizeof(char);
	default:
		return -1;
	}
}

// djb2 hashing algorithm by Dan Bernstein
static unsigned int _ht_hash_string(const char* s)
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

static unsigned int _ht_hash_int(int x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

const struct LibHashTable_l LibHashTable = {
	.create = ht_create,
	.free = ht_free,
	.insert_str = ht_insert_str,
	.insert_int = ht_insert_int,
	.insert_chr = ht_insert_chr,
	.get_str = ht_get_str,
	.get_int = ht_get_int,
	.get_chr = ht_get_chr,
	.delete_str = ht_delete_str,
	.delete_int = ht_delete_int,
	.delete_chr = ht_delete_chr,
	.print_keys = ht_print_keys
};
