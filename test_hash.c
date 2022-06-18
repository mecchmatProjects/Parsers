// Simple hash table demonstration program

/*

*/

#include "kvphash_table.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char* key;
    int value;
} item;

int main() {
    item items[] = {
        {"foo", 10}, {"bar", 42}, {"bazz", 36}, {"buzz", 7},
        {"bob", 11}, {"jane", 100}, {"x", 200}};
    size_t num_items = sizeof(items) / sizeof(item);

    kvphash_table* table = ht_create();
    if (table == NULL) {
        printf("hash table was not created");
        return -1;
    }

    for (int i = 0; i < num_items; i++) {
        if (ht_set(table, items[i].key, &items[i].value) == NULL) {
            printf("hash set did not work");
            return -2;
        }
    }

    for (int i = 0; i < table->capacity; i++) {
        if (table->entries[i].key != NULL) {
            printf("index %d: key %s, value %d\n",
                i, table->entries[i].key, *(int*)table->entries[i].value);
        } else {
            printf("index %d: empty\n", i);
        }
    }

    return 0;
}