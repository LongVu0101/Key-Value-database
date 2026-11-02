# Makefile for the simple key-value store.
#
#   make           build the interactive program and the tests
#   make test      build and run the tests
#   make memcheck  run the tests under Valgrind (checks for leaks)
#   make clean     delete everything that was built

CC     = gcc
# -Wall -Wextra : turn on most useful warnings
# -g            : include debug info for gdb and Valgrind
# -std=c99      : compile as standard C99 (no compiler-specific extensions)
CFLAGS = -Wall -Wextra -g -std=c99

# Object files shared by the program and the tests.
LIB_OBJS = hashtable.o persistence.o

all: kvstore test_kvstore

# $@ = the target name, $^ = all the prerequisites.
kvstore: main.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_kvstore: test_kvstore.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Build any .o from its .c file. Every source includes kvstore.h, so a
# change to the header rebuilds them all. $< = the first prerequisite.
%.o: %.c kvstore.h
	$(CC) $(CFLAGS) -c $<

test: test_kvstore
	./test_kvstore

memcheck: test_kvstore
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./test_kvstore

clean:
	rm -f *.o kvstore test_kvstore

# These targets are commands, not files.
.PHONY: all test memcheck clean
