#ifndef KVSTORE_H
#define KVSTORE_H

typedef struct Node {
    char *key;
    char *value;
    struct Node *next;
} Node;

typedef struct HashTable {
    Node **buckets;
    int size;
} HashTable;

HashTable *create_table(int size);

void set(HashTable *table, const char *key, const char *value);

char *get(HashTable *table, const char *key);

void delete(HashTable *table, const char *key);

void free_table(HashTable *table);

void save_to_disk(HashTable *table, const char *filename);

HashTable *load_from_disk(const char *filename);

#endif
