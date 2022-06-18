/*
The MIT License (MIT)

Copyright (c) 2022, Viktor Borodin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Based upon modified code of
 *  https://github.com/Jhuster/TLV
 */


#ifndef __KVP_HASH_TABLE_H__
#define __KVP_HASH_TABLE_H__

#ifdef __cplusplus
extern "C" {
#else
#endif /* __cplusplus */

#include "utilits.h"

#include <stddef.h>

// Hash table structure: create with ht_create, free with ht_destroy.
typedef struct kvphash_table kvphash_table;

// Create hash table and return pointer to it, or NULL if out of memory.
kvphash_table* ht_create();

// Free memory allocated for hash table, including allocated keys.
void ht_destroy(kvphash_table* table);

// Get item with given key (NUL-terminated) from hash table. Return
// value (which was set with ht_set), or NULL if key not found.
void* ht_get(kvphash_table* table, const char* key);

// Set item with given key (NUL-terminated) to value (which must not
// be NULL). If not already present in table, key is copied to newly
// allocated memory (keys are freed automatically when ht_destroy is
// called). Return address of copied key, or NULL if out of memory.
const char* ht_set(kvphash_table* table, const char* key, void* value);

// Return number of items in hash table.
size_t ht_length(kvphash_table* table);



////////////////////////////////

// Hash table entry (slot may be filled or empty).
typedef struct {
    const char* key;  // key is NULL if this slot is empty
    void* value;
} ht_item;

// Hash table structure: create with ht_create, free with ht_destroy.
struct kvphash_table {
    ht_item* entries;  // hash slots
    size_t capacity;    // size of _entries array
    size_t length;      // number of items in hash table
};


// Hash table iterator: create with ht_iterator, iterate with ht_next.
typedef struct {
    const char* key;  // current key
    void* value;      // current value

    // Don't use these fields directly.
    kvphash_table* _table;       // reference to hash table being iterated
    size_t _index;    // current index into ht._entries
} hti;




// Return new hash table iterator (for use with ht_next).
hti ht_iterator(kvphash_table* table);

// Move iterator to next item in hash table, update iterator's key
// and value to current item, and return true. If there are no more
// items, return false. Don't call ht_set during iteration.
bool ht_next(hti* it);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif // __KVP_HASH_TABLE_H__
