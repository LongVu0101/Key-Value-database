#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "kvstore.h"

#define DEFAULT_TABLE_SIZE 64
#define LINE_SIZE 1024

static void print_help(void)
{
    printf("Commands:\n"
           "  SET <key> <value>   store a value (value may contain spaces)\n"
           "  GET <key>           print a value\n"
           "  DEL <key>           delete a key\n"
           "  SAVE <file>         save all data to a file\n"
           "  LOAD <file>         load data from a file\n"
           "  HELP                show this help\n"
           "  EXIT                quit\n");
}

static void to_upper(char *text)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        text[i] = (char)toupper((unsigned char)text[i]);
    }
}

int main(void)
{
    HashTable *table = create_table(DEFAULT_TABLE_SIZE);
    if (table == NULL) {
        fprintf(stderr, "Could not create the table.\n");
        return 1;
    }

    char line[LINE_SIZE];
    printf("Simple KV store. Type HELP for commands.\n");

    while (1) {
        printf("kvstore> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break; 
        }

        char *command = strtok(line, " \t\r\n");
        if (command == NULL) {
            continue;
        }
        char *key = strtok(NULL, " \t\r\n");
        char *value = strtok(NULL, "\r\n");
        to_upper(command);

        if (strcmp(command, "EXIT") == 0 || strcmp(command, "QUIT") == 0) {
            break;
        } else if (strcmp(command, "HELP") == 0) {
            print_help();
        } else if (key == NULL) {
            printf("Missing argument. Type HELP for usage.\n");
        } else if (strcmp(command, "SET") == 0) {
            if (value == NULL) {
                printf("Usage: SET <key> <value>\n");
            } else {
                set(table, key, value);
                printf("OK\n");
            }
        } else if (strcmp(command, "GET") == 0) {
            char *found = get(table, key);
            printf("%s\n", found != NULL ? found : "(not found)");
        } else if (strcmp(command, "DEL") == 0) {
            if (get(table, key) == NULL) {
                printf("(not found)\n");
            } else {
                delete(table, key);
                printf("OK\n");
            }
        } else if (strcmp(command, "SAVE") == 0) {
            printf("Saving to %s ...\n", key);
            save_to_disk(table, key);
        } else if (strcmp(command, "LOAD") == 0) {
            HashTable *loaded = load_from_disk(key);
            if (loaded != NULL) {
                free_table(table);
                table = loaded;
                printf("Loaded %s\n", key);
            }
        } else {
            printf("Unknown command '%s'. Type HELP for commands.\n", command);
        }
    }

    free_table(table);
    return 0;
}
