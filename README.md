# Minimal Subsequential Transducer

This repository contains a C++20 implementation of a finite-state construction pipeline:

1. parse a finite regular expression over the monoid `Sigma* x Sigma*`;
2. build an acyclic finite-state transducer;
3. convert it to a real-time transducer;
4. determinize it into an acyclic subsequential transducer;
5. canonicalize the subsequential transducer;
6. construct an equivalent minimal subsequential transducer using an acyclic coloured-automaton minimization algorithm whose intended complexity is worst-case linear in the number of transitions.

The project was written as the final project for the course **"Приложения на крайните автомати"** by Prof. Stoyan Mihov at the Faculty of Mathematics and Informatics, Sofia University.

The algorithms and terminology follow Stoyan Mihov and Klaus U. Schulz, *Finite-State Techniques: Automata, Transducers and Bimachines* (2018). The most relevant parts are Chapter 4 for finite-state transducers, Chapter 5 for deterministic and subsequential transducers, especially Sections 5.1, 5.2, 5.4 and 5.5, and Chapter 10 for acyclic/minimal dictionary-style constructions.

The original assignment, translated from Bulgarian, is:

> Given a regular expression over the monoid `Sigma* x Sigma*` with finite language, first construct an acyclic subsequential transducer. Then construct and output an equivalent minimal subsequential transducer, using a worst-case linear algorithm, with respect to the number of transitions, for minimization.

The Bulgarian original is:

> По зададен регулярен израз над моноида `Sigma* x Sigma*` с краен език първо да се построи ацикличен подпоследователен преобразувател. След това да се построи и изведе еквивалентен минимален подпоследователен преобразувател, като се използва линеен (по отношение на броя на преходите), в най-лошия случай, алгоритъм за минимизация.

## Repository Layout

```text
.
├── transducer.hpp / transducer.cpp
├── real_time_transducer.hpp / real_time_transducer.cpp
├── subsequential_transducer.hpp / subsequential_transducer.cpp
├── new_automaton.hpp / new_automaton.cpp
├── main.cpp
├── tests/smoke_tests.cpp
├── CMakeLists.txt
├── Makefile
├── README.md
├── README.bg.md
└── CITATION.bib
```

The older experimental `automaton.*` implementation is intentionally not included. The active minimization path is implemented in `new_automaton.*`.

## Building

With `make`:

```bash
make
./minimal_subsequential_transducer
```

Run the smoke tests:

```bash
make test
```

Debug build with standard-library bounds assertions:

```bash
make debug
./minimal_subsequential_transducer_debug
```

With CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

If CMake picks a broken `ccache` wrapper on your machine, pass the compiler explicitly:

```bash
cmake -S . -B build-cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build build-cmake
ctest --test-dir build-cmake
```

## Input Syntax

The implemented regular-expression syntax is intentionally small because the input language is assumed to be finite.

Atoms have the form:

```text
"input"^"output"
```

Examples:

```text
"a"^"x"
"ab"^"xy"
""^"hundred "
```

The supported operations are:

```text
R1|R2      union
R1.R2      concatenation
(R)        grouping
```

Kleene star and plus are not supported. This matches the assignment assumption that the language is finite.

Example:

```text
("a"^"x"|"b"^"y")."c"^"z"
```

This represents the finite relation:

```text
ac -> xz
bc -> yz
```

The code assumes valid input. It performs some checks, but it is not meant to be a full user-facing regex parser.

## Mathematical Objects

The input relation is a finite subset of `Sigma* x Sigma*`. A pair `(u, v)` means that input word `u` is translated to output word `v`.

A finite-state transducer has states and transitions of the form:

```text
q -- input/output --> p
```

where both `input` and `output` are words. In the initial transducer, a transition may consume several input symbols at once.

A real-time transducer is a transducer where every non-epsilon transition consumes exactly one input symbol. Epsilon-input transitions are removed by closure construction.

A subsequential transducer is deterministic on the input side and may additionally have:

- an initial output;
- output labels on transitions;
- final outputs on final states.

This is the deterministic transducer model discussed in *Finite-State Techniques*, Section 5.1.

## Pipeline Overview

For an expression such as:

```text
"ab"^"xy"|"ac"^"xz"
```

the program performs these conceptual transformations:

```text
regular expression
  -> acyclic transducer over word pairs
  -> real-time transducer
  -> subsequential transducer
  -> canonical subsequential transducer
  -> coloured deterministic automaton
  -> minimal coloured deterministic automaton
  -> minimal subsequential transducer
```

Each stage preserves the represented partial string function, assuming the input relation is functional.

## Step 1: Parsing The Regular Expression

Implemented in `Transducer::reverse_polish_notation` and `Transducer::from_regex`.

The parser uses a shunting-yard style algorithm:

1. Atoms are read as complete quoted pairs `"u"^"v"`.
2. Parentheses control grouping.
3. Concatenation `.` has higher precedence than union `|`.
4. The resulting token stream is reverse Polish notation.
5. A stack of temporary transducers is evaluated from the reverse Polish expression.

For example:

```text
("a"^"x"|"b"^"y")."c"^"z"
```

becomes approximately:

```text
"a"^"x"  "b"^"y"  |  "c"^"z"  .
```

The stack evaluator then builds the transducer bottom-up.

## Step 2: Building An Acyclic Transducer

Implemented in `Transducer`.

For one atom:

```text
"ab"^"xy"
```

the construction creates two states and one transition:

```text
q0 -- ab/xy --> q1
```

where `q0` is initial and `q1` is final.

For union `T1 | T2`, the construction creates a new initial state with epsilon-input/epsilon-output transitions to the initial states of `T1` and `T2`.

For concatenation `T1 . T2`, every final state of `T1` is made non-final and connected to the initial state of `T2` by an epsilon-input/epsilon-output transition.

Because the input regular expression is finite and no Kleene star is supported, these constructions produce an acyclic transducer.

## Step 3: Converting To A Real-Time Transducer

Implemented in `RealTimeTransducer`.

The first part splits transitions with multi-symbol input. A transition:

```text
q -- abc/xyz --> p
```

is turned into a chain:

```text
q -- a/xyz --> r1 -- b/epsilon --> r2 -- c/epsilon --> p
```

This makes every consuming transition read exactly one input symbol.

The second part removes epsilon-input transitions. Conceptually, the algorithm computes epsilon closures. If:

```text
q  -- epsilon/u --> q0
q0 -- a/v       --> r
r  -- epsilon/w --> p
```

then the real-time transducer receives the direct transition:

```text
q -- a/(u v w) --> p
```

In other words, all output produced before and after the real input-consuming transition is accumulated on the new transition.

The implementation stores each epsilon closure as a map from a state to the states reachable through epsilon-input paths, together with the output accumulated along that epsilon path. Since the automaton is acyclic and the input is assumed functional, this is finite.

Finally, unreachable and non-co-reachable states are trimmed.

## Step 4: Determinization Into A Subsequential Transducer

Implemented in `SubsequentialTransducer::from_realtime`.

This follows the determinization procedure for functional transducers with the bounded variation property from *Finite-State Techniques*, Section 5.2.

The key idea is that a deterministic state is not just a subset of old states. It is a set of delayed-output pairs:

```text
{ (q1, w1), (q2, w2), ... }
```

Here `qi` is a state of the real-time transducer and `wi` is an output suffix that has been produced by the nondeterministic path but not emitted yet by the deterministic transducer.

For each deterministic state `S` and input symbol `a`, the implementation:

1. follows every real-time transition `q -- a/v --> p` from every `(q, u)` in `S`;
2. forms candidate outputs `u v`;
3. computes the longest common prefix `lcp` of all candidate outputs for this input symbol;
4. emits that `lcp` on the deterministic transition;
5. stores the residual suffixes `remove_prefix(lcp, u v)` in the target state label.

Example:

```text
"ab"^"xy" | "ac"^"xz"
```

After reading `a`, both possible paths have produced an output beginning with `x`. The subsequential transition on `a` can safely output `x`; the residual outputs `y` and `z` remain delayed until the next input symbol distinguishes the two paths.

The implementation uses a worklist:

```text
queue of newly discovered deterministic states
seen map from normalized label to deterministic state id
```

A label is sorted and deduplicated before it is used as a key. This prevents equivalent deterministic states from being created multiple times.

For finite functional transducers, the bounded variation property is satisfied in the intended use case, so the construction terminates.

## Step 5: Trimming

Implemented in `SubsequentialTransducer::trim` and `RealTimeTransducer::trim`.

A useful transducer state must be both:

- reachable from the initial state;
- co-reachable, meaning that some final state can still be reached from it.

The implementation computes:

```text
reachable states      by BFS/queue from the initial state
co-reachable states   by building reverse edges and BFS/queue from final states
```

All other states and transitions to them are removed.

## Step 6: Canonicalization

Implemented in `DenseSubsequentialTransducer::cannonize`.

The canonicalization follows the idea of maximal state outputs from *Finite-State Techniques*, Section 5.5.

For each state `q`, define `mso(q)` as the longest output prefix that is forced no matter how a successful continuation proceeds from `q`.

For an acyclic transducer, this can be computed in reverse topological order:

```text
mso(q) = lcp({
    final_output(q), if q is final
    transition_output(q, a) . mso(delta(q, a)), for every outgoing transition
})
```

After computing `mso`, the transducer is rewritten:

```text
initial_output' = initial_output . mso(initial)
transition_output'(q, a) = remove_prefix(mso(q), transition_output(q, a) . mso(target))
final_output'(q) = remove_prefix(mso(q), final_output(q))
```

This moves common output prefixes as early as possible. Two equivalent subsequential transducers may look different before canonicalization; after canonicalization, their states can be compared through a coloured deterministic automaton.

## Step 7: Building A Coloured Deterministic Automaton

Implemented in `DeterminedColoredAutomaton::from_transducer`.

After canonicalization, the subsequential transducer is transformed into a deterministic coloured automaton:

- every subsequential state becomes a deterministic automaton state;
- every transition label becomes a single interned symbol representing `(input character, output string)`;
- final states receive colours based on their final output;
- non-final states receive a separate colour.

The implementation interns output strings through a trie:

```text
output string -> OutputId
```

Then it interns transition symbols:

```text
(input character, OutputId) -> SymbolId
```

This is important: two equal transition labels must receive the same `SymbolId`, otherwise equivalent states would fail to merge.

## Step 8: Worst-Case Linear Acyclic Minimization

Implemented in `DeterminedColoredAutomaton::get_minimal`, `construct_L`, `initialize_level_blocks`, `build_incoming`, `refine_by_preimages_of_block`, and `assign_class_ids_from_blocks`.

This is the part intended to satisfy the assignment requirement: minimization linear in the number of transitions in the worst case.

The algorithm uses the fact that the automaton is acyclic.

### Levels

For each state `q`, compute:

```text
d(q) = maximum length of an input path from q to a final state
```

States with the same `d(q)` form a level:

```text
L_i = { q | d(q) = i }
```

This is computed in reverse topological order. Targets are processed before sources.

### Initial Blocks

For each level, states are initially partitioned by colour:

```text
non-final states
final states with final_output = x
final states with final_output = y
...
```

The code represents colours by integer ids:

```text
0                    non-final
1 + final_output_id  final with this final output
```

### Reverse Graph

For every transition:

```text
p -- sigma --> q
```

the algorithm stores an incoming arc:

```text
q <- (p, sigma)
```

This makes it possible to refine predecessor blocks by looking at inverse images of already stabilized target blocks.

### Refinement By Inverse Images

For a block `B` and a symbol `sigma`, define:

```text
sigma^-1(B) = { p | delta(p, sigma) is in B }
```

If a current block `C` contains some states from `sigma^-1(B)` but not all of them, then `C` must be split:

```text
C ∩ sigma^-1(B)
C \ sigma^-1(B)
```

The implementation keeps arrays such as `block_of`, `pos_in_block`, `marked_by_block`, and `touched_blocks` so that each split can be performed by moving marked states in constant time per moved state. Removing a state from a block is done by swapping it with the last element and popping the vector.

Because the automaton is acyclic and refinement proceeds level by level, the target-side classes are already stable when their predecessors are refined.

### Class Assignment

After all refinements, every non-empty block is one equivalence class. The code stores:

```text
class_id_of[state]          -> equivalence class
representative_of_class[id] -> one state from this class
```

The minimal automaton is constructed from representatives: one state per equivalence class, and transitions remapped through class ids.

## Why The Number Of Transitions May Not Decrease

Minimization guarantees a minimal number of states. It does not guarantee that the number of transitions must decrease.

In this representation, a transition label is not just an input character. It is effectively:

```text
(input character, output string)
```

So even if several states merge, many distinct labelled transitions may remain. In dictionary-like examples such as number-to-English conversion, many branches share structure but still carry distinct output labels. It is therefore normal for the transition count to remain the same or decrease only slightly.

A useful sanity check is idempotence:

```text
minimize(minimize(T)) has the same number of states and transitions as minimize(T)
```

The included smoke tests check this property on several examples.

## Complexity Notes

The intended final minimization algorithm in `new_automaton.*` avoids hash-based signature lookup for equivalence classes. Instead, it uses dense vectors, reverse arcs, block ids, and level-wise partition refinement.

Under the assumptions that:

- state ids are dense after conversion to `DenseSubsequentialTransducer`;
- symbols are interned to dense integer ids;
- transition lists are represented in vectors;
- every transition participates in a bounded amount of refinement work;

the acyclic coloured-automaton minimization stage is linear in the number of states and transitions, and therefore linear in the number of transitions for nontrivial connected automata.

Some earlier construction stages may have larger intermediate output, especially determinization, whose output size can be much larger than the original nondeterministic transducer. The linear bound applies to the minimization stage once the acyclic deterministic/subsequential structure has been built.

## Limitations

- The parser assumes valid input.
- The input relation is expected to be functional.
- The language is expected to be finite.
- Kleene star and plus are not implemented.
- The code is educational and explicit; it prioritizes readability over all possible low-level optimizations.

## References

- Stoyan Mihov and Klaus U. Schulz, *Finite-State Techniques: Automata, Transducers and Bimachines*, pre-publication version, 2018.
- Chapter 4: monoidal and classical finite-state transducers.
- Section 5.1: deterministic and subsequential transducers.
- Section 5.2: determinization of functional transducers with the bounded variation property.
- Sections 5.4-5.5: Myhill-Nerode relation, canonical form, and minimization of subsequential transducers.
- Chapter 10: acyclic minimal automata and dictionary-style finite-language constructions.

Figures from the book are not embedded in this repository because the available PDF explicitly restricts redistribution and derivative use. The README therefore cites the relevant sections instead of copying book images.
