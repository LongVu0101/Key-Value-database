#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvstore.h"

static const char FILE_MAGIC[] = "KVSTORE1";

#define MAX_FIELD_LENGTH (64u * 1024u * 1024u)

void save_to_disk(HashTable *table, const char *filename)
{
    if (table == NULL || filename == NULL) {
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror(filename);
        return;
    }

    fprintf(file, "%s %d\n", FILE_MAGIC, table->size);

    for (int i = 0; i < table->size; i++) {
        for (Node *node = table->buckets[i]; node != NULL; node = node->next) {
            size_t key_length = strlen(node->key);
            size_t value_length = strlen(node->value);

            fprintf(file, "%zu %zu\n", key_length, value_length);
            fwrite(node->key, 1, key_length, file);
            fwrite(node->value, 1, value_length, file);
            fputc('\n', file);
        }
    }

    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        fprintf(stderr, "save_to_disk: error while writing '%s'\n", filename);
    }
}

static char *read_field(FILE *file, size_t length)
{
    if (length > MAX_FIELD_LENGTH) {
        return NULL;
    }

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        return NULL;
    }

    if (fread(buffer, 1, length, file) != length) {
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';

    if (strlen(buffer) != length) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

HashTable *load_from_disk(const char *filename)
{
    if (filename == NULL) {
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror(filename);
        return NULL;
    }

    char magic[16];
    int size = 0;
    if (fscanf(file, "%15s %d", magic, &size) != 2
            || strcmp(magic, FILE_MAGIC) != 0
            || size <= 0) {
        fprintf(stderr, "load_from_disk: '%s' is not a valid kvstore file\n",
                filename);
        fclose(file);
        return NULL;
    }

    HashTable *table = create_table(size);
    if (table == NULL) {
        fclose(file);
        return NULL;
    }

    size_t key_length = 0;
    size_t value_length = 0;
    int ok = 1;
    int matched;

    while ((matched = fscanf(file, "%zu %zu", &key_length, &value_length)) == 2) {
        if (fgetc(file) != '\n') {
            ok = 0;
            break;
        }

        char *key = read_field(file, key_length);
        char *value = read_field(file, value_length);
        if (key == NULL || value == NULL || fgetc(file) != '\n') {
            free(key);
            free(value);
            ok = 0;
            break;
        }

        set(table, key, value);

        char *stored = get(table, key);
        if (stored == NULL || strcmp(stored, value) != 0) {
            ok = 0;
        }

        free(key);
        free(value);

        if (!ok) {
            break;
        }
    }

    if (ok && (matched != EOF || ferror(file))) {
        ok = 0;
    }
    fclose(file);

    if (!ok) {
        fprintf(stderr, "load_from_disk: '%s' is damaged or incomplete\n",
                filename);
        free_table(table);
        return NULL;
    }
    return table;
}
