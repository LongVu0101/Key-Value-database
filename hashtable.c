#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvstore.h"

static unsigned long hash_string(const char *str)
{
    unsigned long hash = 5381;

    for (size_t i = 0; str[i] != '\0'; i++) {
        hash = hash * 33 + (unsigned char)str[i];
    }
    return hash;
}

static int bucket_index(const HashTable *table, const char *key)
{
    return (int)(hash_string(key) % (unsigned long)table->size);
}

static char *copy_string(const char *src)
{
    size_t length = strlen(src) + 1;
    char *copy = malloc(length);

    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, src, length);
    return copy;
}

static void free_node(Node *node)
{
    free(node->key);
    free(node->value);
    free(node);
}

static Node *find_node(HashTable *table, const char *key)
{
    Node *current = table->buckets[bucket_index(table, key)];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

HashTable *create_table(int size)
{
    if (size <= 0) {
        fprintf(stderr, "create_table: size must be positive (got %d)\n", size);
        return NULL;
    }

    HashTable *table = malloc(sizeof(HashTable));
    if (table == NULL) {
        return NULL;
    }

    table->buckets = malloc((size_t)size * sizeof(Node *));
    if (table->buckets == NULL) {
        free(table);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        table->buckets[i] = NULL;
    }
    table->size = size;
    return table;
}

void set(HashTable *table, const char *key, const char *value)
{
    if (table == NULL || key == NULL || value == NULL) {
        return;
    }

    Node *existing = find_node(table, key);
    if (existing != NULL) {
        char *new_value = copy_string(value);
        if (new_value == NULL) {
            fprintf(stderr, "set: out of memory, key '%s' not updated\n", key);
            return;
        }
        free(existing->value);
        existing->value = new_value;
        return;
    }

    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "set: out of memory, key '%s' not stored\n", key);
        return;
    }
    node->key = copy_string(key);
    node->value = copy_string(value);
    node->next = NULL;

    if (node->key == NULL || node->value == NULL) {
        fprintf(stderr, "set: out of memory, key '%s' not stored\n", key);
        return;
    }

    int index = bucket_index(table, key);
    node->next = table->buckets[index];
    table->buckets[index] = node;
}

char *get(HashTable *table, const char *key)
{
    if (table == NULL || key == NULL) {
        return NULL;
    }

    Node *node = find_node(table, key);
    return (node != NULL) ? node->value : NULL;
}

void delete(HashTable *table, const char *key)
{
    if (table == NULL || key == NULL) {
        return;
    }

    int index = bucket_index(table, key);
    Node *current = table->buckets[index];
    Node *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (previous == NULL) {
                table->buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }
            free_node(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void free_table(HashTable *table)
{
    if (table == NULL) {
        return;
    }

    for (int i = 0; i < table->size; i++) {
        Node *current = table->buckets[i];
        while (current != NULL) {
            Node *next = current->next;
            free_node(current);
            current = next;
        }
    }
    free(table->buckets);
    free(table);
}
