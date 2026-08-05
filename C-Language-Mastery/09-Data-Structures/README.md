# Module 09 — Data Structures

Every structure here is implemented from scratch, with a `main()` that exercises it and
checks its invariants. **Type them out yourself.** Reading a hash table teaches you almost
nothing; writing one teaches you collision handling, load factors, and why `realloc`
invalidates pointers.

C has no generics and no standard container library, so you get to see exactly what every
other language is doing underneath.

---

## Complexity summary

| Structure | Access | Search | Insert | Delete | Space | Notes |
|---|---|---|---|---|---|---|
| Dynamic array | O(1) | O(n) | O(1)* | O(n) | O(n) | *amortised; O(n) worst on growth |
| Singly linked list | O(n) | O(n) | O(1)† | O(1)† | O(n) | †at a known position |
| Doubly linked list | O(n) | O(n) | O(1)† | O(1)† | O(n) | O(1) delete given the node |
| Stack | — | — | O(1) | O(1) | O(n) | LIFO |
| Queue | — | — | O(1) | O(1) | O(n) | FIFO |
| Ring buffer | O(1) | O(n) | O(1) | O(1) | O(n) | fixed size, no allocation |
| Hash table | — | O(1)avg | O(1)avg | O(1)avg | O(n) | O(n) worst; depends on the hash |
| BST (unbalanced) | — | O(log n)avg | O(log n)avg | O(log n)avg | O(n) | **O(n) on sorted input** |
| AVL tree | — | O(log n) | O(log n) | O(log n) | O(n) | guaranteed, via rotations |
| Binary heap | O(1) min | O(n) | O(log n) | O(log n) | O(n) | priority queue |
| Trie | — | O(k) | O(k) | O(k) | O(n·k) | k = key length, not n |
| Union-Find | — | O(α(n)) | O(α(n)) | — | O(n) | α is effectively ≤ 4 |
| Graph (adj. list) | — | O(V+E) | O(1) | O(E) | O(V+E) | sparse graphs |
| Graph (adj. matrix) | O(1) | O(V²) | O(1) | O(1) | O(V²) | dense graphs |
| LRU cache | O(1) | O(1) | O(1) | O(1) | O(n) | hash map + doubly linked list |

---

## The recurring implementation themes

**1. Growth by doubling.** Anything that grows doubles its capacity, making `n` appends
O(n) total. Growing by one is O(n²). (Module 05 measures this.)

**2. `realloc` invalidates pointers.** Any structure built on a dynamic array must not
hand out long-lived pointers to its elements — they dangle after the next growth. Return
indices, or document the invalidation.

**3. Sentinels and dummy heads** remove special cases. A linked list with a dummy head node
never needs `if (head == NULL)` in its insert and delete paths.

**4. The intrusive pattern.** Embed the link *inside* the user's struct and recover the
object with `container_of` (module 07). One list implementation works for every type, with
zero allocations per node. This is how the Linux kernel does it.

**5. Two pointers, slow and fast.** Cycle detection, finding the middle, and
partitioning all fall out of this.

**6. Composition.** An LRU cache is a hash map plus a doubly linked list. A priority queue
is a heap. Most "advanced" structures are two simple ones wired together.

---

## Practice

| File | What it implements |
|---|---|
| `practice/01_dynamic_array.c` | Growable array, amortised analysis, insert/remove, the invalidation trap |
| `practice/02_linked_list.c` | Singly and doubly linked, reverse, cycle detection, merge sort on a list |
| `practice/03_stack_queue.c` | Stack, queue, ring buffer, deque; queue from two stacks |
| `practice/04_hash_table.c` | Chaining **and** open addressing, FNV-1a and djb2, load factor, resizing, tombstones |
| `practice/05_binary_search_tree.c` | Insert/search/delete (all three delete cases), traversals, degeneration on sorted input |
| `practice/06_avl_tree.c` | Self-balancing: all four rotations, height tracking, measured against the BST |
| `practice/07_heap.c` | Binary heap, sift up/down, heapify in O(n), heapsort, a priority queue |
| `practice/08_trie.c` | Prefix tree: insert, search, prefix match, autocomplete, memory analysis |
| `practice/09_graph.c` | Adjacency list and matrix, BFS, DFS, topological sort, cycle detection |
| `practice/10_union_find.c` | Disjoint sets with path compression and union by rank; Kruskal's MST |
| `practice/11_lru_cache.c` | O(1) get/put by composing a hash map with a doubly linked list |
| `practice/12_generic_containers.c` | Type-agnostic containers three ways: `void *`, macros, intrusive links |

```bash
cd 09-Data-Structures
for f in practice/*.c; do gcc -std=c17 -Wall -Wextra -g "$f" -o /tmp/ds && /tmp/ds; done

# every one is valgrind-clean by construction
gcc -g practice/04_hash_table.c -o /tmp/ht && valgrind --leak-check=full /tmp/ht
```

---

## How to study this module

For each structure, in order:

1. Read the theory above and the complexity row.
2. **Implement it from scratch, from memory, without looking.** Get it wrong.
3. Compare against the file here.
4. Break it deliberately: remove the NULL check, use `<=` instead of `<`, forget to update
   `prev`. Run it under ASan and read the report.
5. Explain out loud why each operation has the complexity it has.

Step 2 is the whole module. Steps 1, 3, 4, 5 are support.
