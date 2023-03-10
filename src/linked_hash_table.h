#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// TODO: eventually use macros to set the hash sizes so that user can replace hash function with a different output size
// TODO: incorporate a randomized hash seed for strings, "salting": see http://ocert.org/advisories/ocert-2011-003.html, referenced from https://docs.python.org/3.8/reference/datamodel.html#object.__hash__

#ifndef LINKED_HASH_TABLE_LOAD_FACTOR 
#define LINKED_HASH_TABLE_LOAD_FACTOR .75
#endif

#ifndef LINKED_HASH_TABLE_DEFAULT_CAPACITY
#define LINKED_HASH_TABLE_DEFAULT_CAPACITY 32
#endif

#ifndef LINKED_HASH_TABLE_SCALE_FACTOR
#define LINKED_HASH_TABLE_SCALE_FACTOR 2
#endif

#ifndef HASH_OUT_BIT_SIZE
#define HASH_OUT_BIT_SIZE 64
#endif // HASH_OUT_BIT_SIZE

/* INTERNAL SETTINGS */

// leave undefined if HASH_OUT_BIT_SIZE > 64
#define HASH_TYPE

#if HASH_OUT_BIT_SIZE <= 64
    #undef HASH_TYPE
    #define HASH_TYPE unsigned long long
#endif

#if HASH_OUT_BIT_SIZE <= 32
    #undef HASH_TYPE
    #define HASH_TYPE unsigned long
#endif

#if HASH_OUT_BIT_SIZE <= 16
    #undef HASH_TYPE
    #define HASH_TYPE unsigned short
#endif

#if HASH_OUT_BIT_SIZE <= 8
    #undef HASH_TYPE
    #define HASH_TYPE unsigned char
#endif

typedef struct DoubleLinkedHashNode DoubleLinkedHashNode;
typedef struct LinkedHashTable LinkedHashTable;

struct DoubleLinkedHashNode {
    void * key;
    void * value;
    HASH_TYPE hash; // store the raw hash so it does not have to be recalculated if expanding size
    DoubleLinkedHashNode * next_inorder; // next in linked list of HashTable Keys as inserted
    DoubleLinkedHashNode * next_inhash; // next in linked list of HashTable Keys as inserted that have the same hash
    DoubleLinkedHashNode * prev_inorder; // previous in linked list of HashTable Keys as inserted
};

struct LinkedHashTable {
    DoubleLinkedHashNode ** hash_table; // array of hash nodes as bins
    DoubleLinkedHashNode * head_inorder;
    DoubleLinkedHashNode * tail_inorder;
    size_t capacity; // allocation of hash_table
    size_t size;
    size_t max_load_factor;
    int (*key_comp) (void *, void *);
};

HASH_TYPE hash_djb(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

size_t hash_to_bin(HASH_TYPE hash, size_t ht_size) {

}