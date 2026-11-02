#include <stdio.h>
#include <string.h>

#include "kvstore.h"

static int tests_run = 0;
static int tests_failed = 0;

static void check(int condition, const char *description)
{
    tests_run++;
    if (!condition) {
        tests_failed++;
        printf("  FAIL: %s\n", description);
    }
}

static int value_is(HashTable *table, const char *key, const char *expected)
{
    char *actual = get(table, key);
    return actual != NULL && strcmp(actual, expected) == 0;
}

static void write_file(const char *filename, const char *content)
{
    FILE *file = fopen(filename, "wb");
    if (file != NULL) {
        fputs(content, file);
        fclose(file);
    }
}

static void test_create_table(void)
{
    printf("create_table\n");
    check(create_table(0) == NULL, "size 0 is rejected");
    check(create_table(-5) == NULL, "negative size is rejected");

    HashTable *table = create_table(8);
    check(table != NULL, "valid size creates a table");
    check(table != NULL && table->size == 8, "size is stored");
    free_table(table);
    free_table(NULL);
}

static void test_set_and_get(void)
{
    printf("set / get\n");
    HashTable *table = create_table(16);

    set(table, "name", "Alice");
    set(table, "city", "Hanoi");
    check(value_is(table, "name", "Alice"), "get returns stored value");
    check(value_is(table, "city", "Hanoi"), "second key is independent");
    check(get(table, "missing") == NULL, "missing key returns NULL");

    set(table, "name", "Bob");
    check(value_is(table, "name", "Bob"), "set overwrites existing key");

    char buffer[32] = "temporary";
    set(table, "copy", buffer);
    strcpy(buffer, "changed!!");
    check(value_is(table, "copy", "temporary"), "set copies the value");

    set(table, "city", get(table, "city"));
    check(value_is(table, "city", "Hanoi"), "set(k, get(k)) is safe");

    set(table, "", "empty key");
    check(value_is(table, "", "empty key"), "empty key works");

    set(NULL, "a", "b");
    set(table, NULL, "b");
    set(table, "a", NULL);
    check(get(table, "a") == NULL, "NULL arguments are ignored");
    check(get(NULL, "a") == NULL, "get on NULL table returns NULL");

    free_table(table);
}

static void test_collisions(void)
{
    printf("collisions\n");
    HashTable *table = create_table(1);
    char key[32];
    char value[32];

    for (int i = 0; i < 100; i++) {
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        set(table, key, value);
    }

    int all_found = 1;
    for (int i = 0; i < 100; i++) {
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        if (!value_is(table, key, value)) {
            all_found = 0;
        }
    }
    check(all_found, "all 100 colliding keys are retrievable");
    free_table(table);
}

static void test_delete(void)
{
    printf("delete\n");
    HashTable *table = create_table(1);
    set(table, "a", "1");
    set(table, "b", "2");
    set(table, "c", "3");

    delete(table, "b");
    check(get(table, "b") == NULL, "middle node deleted");
    check(value_is(table, "a", "1") && value_is(table, "c", "3"),
          "neighbours of middle node survive");

    delete(table, "c");
    check(get(table, "c") == NULL, "head node deleted");
    check(value_is(table, "a", "1"), "rest of chain survives head delete");

    delete(table, "a");
    check(get(table, "a") == NULL, "last node deleted");
    check(table->buckets[0] == NULL, "bucket is empty again");

    delete(table, "never-existed");
    delete(NULL, "x");
    check(1, "deleting missing key / NULL table is safe");

    set(table, "a", "again");
    check(value_is(table, "a", "again"), "key can be re-added after delete");
    free_table(table);
}

static void test_save_and_load(void)
{
    printf("save / load\n");
    const char *filename = "test_roundtrip.kv";
    HashTable *table = create_table(7);

    set(table, "simple", "value");
    set(table, "with spaces", "hello world");
    set(table, "multi\nline", "first\nsecond\n");
    set(table, " leading", "=equals=");
    set(table, "empty", "");
    save_to_disk(table, filename);

    HashTable *loaded = load_from_disk(filename);
    check(loaded != NULL, "file loads");
    if (loaded != NULL) {
        check(loaded->size == 7, "bucket count is restored");
        check(value_is(loaded, "simple", "value"), "simple pair");
        check(value_is(loaded, "with spaces", "hello world"), "spaces kept");
        check(value_is(loaded, "multi\nline", "first\nsecond\n"),
              "newlines kept");
        check(value_is(loaded, " leading", "=equals="),
              "leading space and '=' kept");
        check(value_is(loaded, "empty", ""), "empty value kept");
    }
    free_table(loaded);
    free_table(table);

    table = create_table(3);
    save_to_disk(table, filename);
    loaded = load_from_disk(filename);
    check(loaded != NULL, "empty table round-trips");
    free_table(loaded);
    free_table(table);
    remove(filename);
}

static void test_bad_files(void)
{
    printf("bad files (error messages below are expected)\n");
    const char *filename = "test_bad.kv";

    check(load_from_disk("does_not_exist.kv") == NULL, "missing file -> NULL");

    write_file(filename, "hello world\n");
    check(load_from_disk(filename) == NULL, "wrong header -> NULL");

    write_file(filename, "KVSTORE1 0\n");
    check(load_from_disk(filename) == NULL, "zero buckets -> NULL");

    write_file(filename, "KVSTORE1 4\n3 5\nkeyval");
    check(load_from_disk(filename) == NULL, "truncated record -> NULL");

    write_file(filename, "KVSTORE1 4\n3 3\nkeyval\nxyz");
    check(load_from_disk(filename) == NULL, "trailing garbage -> NULL");

    write_file(filename, "KVSTORE1 4\n-1 3\nkeyval\n");
    check(load_from_disk(filename) == NULL, "absurd length -> NULL");

    remove(filename);
}

int main(void)
{
    test_create_table();
    test_set_and_get();
    test_collisions();
    test_delete();
    test_save_and_load();
    test_bad_files();

    printf("\n%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
