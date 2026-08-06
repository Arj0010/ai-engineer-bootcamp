# Module 10 — Algorithms

Module 09 was about *storing* data. This one is about *doing things to it*.

Every algorithm here is implemented, instrumented (comparisons and swaps are counted), and
benchmarked against the alternatives. The measurements matter: the gap between theory and a
stopwatch is where most of the learning is.

---

## 1. Complexity, honestly

Big-O describes how cost *grows*, ignoring constants. That makes it the right tool for
choosing an algorithm and the wrong tool for predicting a runtime.

| Growth | n=10 | n=1,000 | n=1,000,000 | Example |
|---|---|---|---|---|
| O(1) | 1 | 1 | 1 | array index, hash lookup |
| O(log n) | 3 | 10 | 20 | binary search, balanced tree |
| O(n) | 10 | 1,000 | 10⁶ | linear scan |
| O(n log n) | 33 | 10⁴ | 2×10⁷ | good sorting |
| O(n²) | 100 | 10⁶ | 10¹² | nested loops, bubble sort |
| O(2ⁿ) | 1,024 | 10³⁰¹ | — | naive subsets |
| O(n!) | 3.6×10⁶ | — | — | brute-force permutations |

**The constants are not always negligible.** Insertion sort is O(n²) and *beats* merge sort
below ~30 elements, which is why every real library sort switches to it for small
subarrays. `practice/01_sorting.c` measures exactly where the crossover is on your machine.

Also worth separating:
- **worst / average / best case** — quicksort is O(n log n) average and O(n²) worst.
- **amortised** — a cost averaged over a sequence (dynamic-array append).
- **space** — merge sort's O(n) scratch buffer can matter more than its speed.

---

## 2. Sorting

| Algorithm | Best | Average | Worst | Space | Stable | Notes |
|---|---|---|---|---|---|---|
| Bubble | O(n) | O(n²) | O(n²) | O(1) | yes | teaching only |
| Selection | O(n²) | O(n²) | O(n²) | O(1) | no | minimum swaps (n−1) |
| Insertion | O(n) | O(n²) | O(n²) | O(1) | yes | **excellent** on small or nearly-sorted input |
| Merge | O(n log n) | O(n log n) | O(n log n) | O(n) | yes | predictable; the choice for linked lists and external sorting |
| Quick | O(n log n) | O(n log n) | O(n²) | O(log n) | no | fastest in practice; pivot choice is everything |
| Heap | O(n log n) | O(n log n) | O(n log n) | O(1) | no | guaranteed, in place, cache-unfriendly |
| Counting | O(n+k) | O(n+k) | O(n+k) | O(k) | yes | integers in a small known range |
| Radix | O(d(n+k)) | — | — | O(n+k) | yes | fixed-width keys; beats comparison sorts |

**Ω(n log n) is a proven lower bound for comparison sorts.** Counting and radix sort beat
it by not comparing — they use the key's *value* as an index.

**Stability** — equal elements keep their relative order — matters when you sort by one
field and then another.

---

## 3. The algorithm design patterns

**Divide and conquer.** Split, solve the parts, combine. Merge sort, quicksort, binary
search, FFT. Cost obeys the master theorem: `T(n) = aT(n/b) + f(n)`.

**Dynamic programming.** Applies when a problem has *optimal substructure* (the best
solution contains best solutions to subproblems) **and** *overlapping subproblems* (the same
subproblem recurs). Two forms:
- **top-down / memoised** — write the recursion, cache the results. Easier to derive.
- **bottom-up / tabulated** — fill a table in dependency order. Faster, no recursion, and
  often lets you drop the table to a single row.

**Greedy.** Take the locally best option and never reconsider. Far simpler than DP — and
*wrong* unless the problem has the greedy-choice property. Coin change is the standard
demonstration: greedy is optimal for {1,5,10,25} and wrong for {1,3,4}.

**Backtracking.** Try, recurse, undo. A DFS over the space of partial solutions, with
pruning. N-queens, sudoku, permutations, subset-sum.

---

## 4. Searching

| Algorithm | Requires | Time |
|---|---|---|
| Linear | nothing | O(n) |
| Binary | sorted | O(log n) |
| Interpolation | sorted, uniformly distributed | O(log log n) avg, O(n) worst |
| Exponential | sorted, unbounded | O(log i) where i is the target's index |

Binary search is famously easy to get subtly wrong. The three things that bite:
- `mid = (lo + hi) / 2` **overflows**. Write `lo + (hi - lo) / 2`.
- The loop condition (`<` vs `<=`) must match how you update `lo` and `hi`, or it hangs.
- "Find any match" and "find the *first* match" are different algorithms.

---

## 5. Graph algorithms

| Problem | Algorithm | Time | Constraint |
|---|---|---|---|
| Shortest path, unweighted | BFS | O(V+E) | — |
| Shortest path, non-negative weights | Dijkstra | O((V+E) log V) | no negative edges |
| Shortest path, negative weights | Bellman-Ford | O(VE) | detects negative cycles |
| All-pairs shortest paths | Floyd-Warshall | O(V³) | simple; fine for small V |
| Minimum spanning tree | Kruskal / Prim | O(E log E) | undirected |
| Topological order | Kahn / DFS | O(V+E) | DAG only |

---

## 6. String algorithms

| Algorithm | Preprocessing | Search | Idea |
|---|---|---|---|
| Naive | — | O(nm) | try every offset |
| KMP | O(m) | O(n) | a failure table says how far to skip after a mismatch |
| Rabin-Karp | O(m) | O(n) avg | a rolling hash makes each window O(1) to compare |
| Boyer-Moore | O(m+σ) | O(n/m) best | scan the pattern backwards, skip whole pattern lengths |
| Z-algorithm | O(n+m) | O(n+m) | longest-common-prefix array |

---

## Practice

| File | Contents |
|---|---|
| `practice/01_sorting.c` | Eight sorts, instrumented and benchmarked; the insertion-sort crossover; stability demonstrated |
| `practice/02_searching.c` | Binary search and its variants (first/last/insertion point), the overflow bug, interpolation and exponential search |
| `practice/03_dynamic_programming.c` | Fibonacci three ways, knapsack, LCS, edit distance, coin change, LIS, with the memoised/tabulated contrast |
| `practice/04_greedy.c` | Activity selection, fractional knapsack, Huffman coding, and a worked case where greedy **fails** |
| `practice/05_backtracking.c` | N-queens, sudoku solver, permutations, subsets, subset-sum — with pruning measured |
| `practice/06_graph_algorithms.c` | Dijkstra, Bellman-Ford (with negative-cycle detection), Floyd-Warshall, Prim |
| `practice/07_string_algorithms.c` | Naive, KMP, Rabin-Karp, Boyer-Moore, Z-algorithm, benchmarked on adversarial input |

---

## How to study this

For each algorithm: understand the *idea* in one sentence, implement it from that sentence
alone, then compare. If you cannot state the idea in a sentence, you have not understood it
yet — you have memorised it.

- Merge sort: *split in half, sort each, merge the two sorted halves.*
- Quicksort: *pick a pivot, partition around it, recurse on both sides.*
- KMP: *on a mismatch, the pattern's own prefix table says how far you can skip.*
- Dijkstra: *always expand the closest unvisited node.*
