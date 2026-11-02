# kvstore — an in-memory key-value store in C

A small key-value database written in pure C99: a hash table with separate
chaining, save/load to disk, an interactive shell, and a test suite that
runs clean under Valgrind.

## Build and run

```sh
make            # builds ./kvstore and ./test_kvstore
make test       # runs the tests
make memcheck   # runs the tests under Valgrind
make clean
```

```text
$ ./kvstore
kvstore> SET name Alice Smith
OK
kvstore> GET name
Alice Smith
kvstore> SAVE data.kv
Saving to data.kv ...
kvstore> DEL name
OK
kvstore> LOAD data.kv
Loaded data.kv
kvstore> GET name
Alice Smith
```

## Project layout

| File             | Purpose                                         |
|------------------|-------------------------------------------------|
| `kvstore.h`      | Public API, data structures, ownership rules    |
| `hashtable.c`    | create / set / get / delete / free              |
| `persistence.c`  | save_to_disk / load_from_disk                   |
| `main.c`         | Interactive shell                               |
| `test_kvstore.c` | Unit tests (36 checks)                          |

## Design

- **Hashing:** djb2 (`hash = hash * 33 + c`) on `unsigned long`, then
  `hash % size` picks the bucket.
- **Collisions:** separate chaining — each bucket is a linked list; new keys
  are pushed onto the front in O(1).
- **Memory ownership:** the table stores its own copies of keys and values,
  so callers can reuse their buffers. `get()` returns a pointer owned by the
  table. `free_table()` releases everything.
- **Failure safety:** every `malloc` is checked; on failure the table is left
  unchanged. When updating a key, the new value is copied before the old one
  is freed.
- **File format:** length-prefixed records, so keys and values may contain
  spaces, `=` or newlines. Damaged or truncated files are rejected, and a
  failed load never returns a half-built table.

```text
KVSTORE1 64
4 11
nameAlice Smith
```

| Operation | Average | Worst case (all keys in one bucket) |
|-----------|---------|-------------------------------------|
| set / get / delete | O(1) | O(n) |

## Possible next steps

- Resize (rehash) the table when it gets too full, to keep chains short.
- Return an error code from `set()` and `save_to_disk()` instead of `void`.
- Save atomically (write to a temp file, then `rename()` over the original).
- Add a TCP server so several clients can use the store.
