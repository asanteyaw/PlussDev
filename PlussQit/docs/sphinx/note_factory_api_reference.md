<!-- pluss-hand-authored -->

# Note Factory API Reference

[Back to PlussQit API Reference](../api_reference.md)

This reference covers the free functions in `pluss::note`. They answer a
different question from the [`Note` method reference](note_api_reference.md):
how to **build** a text payload before there is a receiver to call a method
on.

## What `pluss::note` Is

`Note` is the text object in the PlussQit data-analysis stack. It carries
UTF-8 text — or, in bytes mode, raw byte-oriented content — and supports
shape-preserving, vectorized text manipulation and natural-language
processing across scalars, columns, and small text tensors, the same way
`Series` carries numeric values and `Timeline` carries instants.

`pluss::note::*` is the construction namespace for that object. Once a `Note`
exists, every transformation is a **method** on it; these free functions only
*create* one. The design mirrors `pluss::series::*` and `pluss::timeline::*`:
the type and its factory surface are kept apart because "I need a `Note`" and
"I have a `Note`, now what" are different jobs.

## Choose By Intent

- **From text you already hold** — `make` for UTF-8 text (scalar, a 1-D
  batch, or a small nested tensor); `bytes` when the content is raw bytes
  with no UTF-8 guarantee. `pluss::make_note` is the one flat shortcut kept
  for the common `make`-from-data case.
- **Shape first, one value** — `full` (every cell a chosen string), `empty`
  (every cell the empty string), `rep` (the same, with the value written
  first at the call site).
- **Shape from an existing `Note`** — `full_like`, `empty_like`, and the
  bytes-mode `full_bytes_like`, `empty_bytes_like`.
- **Generate content** — `rand` (one random letter per cell),
  `rand_between` (random single characters in a range), `arange` (a stepped
  run of single characters), `perm` (character combinations or permutations
  of a word).
- **Split existing text** — `from_lines` and `from_words` turn one string
  into a 1-D `Note`.
- **Assemble and choose** — `format` fills a `{placeholder}` template;
  `select` builds a `Note` by ordered `Series` conditions.
- **Restore packed storage** — `unpack` validates and reconstructs the stable
  binary envelope emitted by `Note::pack()`.
- **Render** — `print` produces a diagnostic representation (`std::cout << n`
  works too).

## Reading The Signatures

Return type comes first, as everywhere in this reference. Where a parameter
accepts several call-site spellings — a bare string literal, a braced list,
a nested braced list, or a range — the pages document the canonical behavior
first and then the overload quirks that change the result. `make` and
`bytes` each collapse a wide set of historical spellings into one converted
text parameter plus one explicit `(flat, sizes)` overload per source
container.

## Running The Examples

Every example is a runnable fragment (assume `#include <pluss/pluss.h>` and
`#include <iostream>`), and every `### Output` block is copied verbatim from
the compiled program. To print a `Note` to the terminal, any of these three
spellings works — they render byte-identical text, and the pages rotate
through them so each is visible:

```cpp
std::cout << pluss::note::print(n) << "\n";  // print(n) -> std::string
std::cout << n << "\n";                       // operator<< (default options)
the stream pluss::note::print(std::cout, n) << "\n";     // print(os, n)
```

`pluss::note::print(n, opt)` takes a `NotePrintOptions` for control over the
layout — see [`print`](api/note_factory/print.md).

Examples that show a rejection call the factory inside a `try` block and
print the first line of `pluss::Error::what()` (the message); `what()` also
appends a `Triggered at:` location line, which the output blocks omit.

## Reproducibility

`rand` and `rand_between` take a `seed`. A fixed nonzero seed makes the draw
deterministic; the pages use seeded calls so their output is stable. `perm`,
`arange`, and every other factory here are already deterministic.

Deterministic construction output is produced by
`examples/CodeN/note_factory_reference_demo.cpp`; the binary round-trip and
rejection cases for `unpack` are produced by
`examples/CodeN/tokenbatch_codec_reference_demo.cpp`.

Every `pluss::note::*` factory has a page: the core constructors (`make`,
`bytes`), the shape-first family (`full`, `empty`, `rep`, and the four
`*_like` factories), the generative builders (`arange`, `perm`, `rand`,
`rand_between`), the text-splitting factories (`from_lines`, `from_words`),
assembly and rendering (`format`, `select`, `print`), and validated binary
reconstruction (`unpack`).

```{toctree}
:hidden:
:maxdepth: 1

api/note_factory/arange
api/note_factory/bytes
api/note_factory/empty
api/note_factory/empty_bytes_like
api/note_factory/empty_like
api/note_factory/format
api/note_factory/from_lines
api/note_factory/from_words
api/note_factory/full
api/note_factory/full_bytes_like
api/note_factory/full_like
api/note_factory/make
api/note_factory/perm
api/note_factory/print
api/note_factory/rand
api/note_factory/rand_between
api/note_factory/rep
api/note_factory/select
api/note_factory/unpack
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/note_factory/arange.html">
    <span class="label">Next</span>
    <span class="title">arange</span>
  </a>
</div>
