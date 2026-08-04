# Note: All Methods Usage

This page documents all practical construction styles and call patterns for `Note` methods.

## Construction: all practical forms

```cpp
#include <pluss/pluss.h>
```

```cpp
// UTF-8 scalar / 1D / ND
auto n0 = pluss::make_note("hello");
auto n1 = pluss::make_note("A"); // char-like scalar text

auto n2 = pluss::make_note({"alpha", "beta", "gamma"});
auto n3 = pluss::make_note({{"r1c1", "r1c2"}, {"r2c1", "r2c2"}});

std::vector<std::string> flat{"a","b","c","d"};
auto n4 = pluss::make_note(flat, std::array<std::int64_t,2>{2,2});
```

```cpp
// Text-derived helpers
auto lines = pluss::note::from_lines("line1\nline2\nline3");
auto words = pluss::note::from_words("alpha beta gamma");
auto full = pluss::note::full({2,3}, "x");
auto empty_like = pluss::note::empty_like(full);
```

```cpp
// Bytes-mode construction
auto b0 = pluss::note::bytes("\x01\x02");
auto b1 = pluss::note::bytes({"\x01", "\x02", "\x03"});
auto b2 = pluss::note::bytes({{"\x0A", "\x0B"}, {"\x0C", "\x0D"}});
```

## Method call rules (all overload styles)

```cpp
// No-arg transform families
auto clean = note.lower().strip().collapse_whitespace();
auto tex = note.tex_to_text();
auto pii = note.nlp_mask_pii();

// String-argument transforms
auto rep = note.replace("from", "to");
auto rgx = note.regex_replace("\\d+", "<NUM>");

// TokenBatch-returning methods
auto tb = note.tokenize_words();
auto lines = note.split_lines();
auto math = note.tex_commands();

// Vector-returning predicates/counts
auto has = note.contains("foo");
auto lens = note.lengths_chars();
```

## Canonical runnable coverage

- `PlussQit/examples/CodeN/note_factory_demo.cpp`
- `PlussQit/examples/CodeN/note_base_methods_demo.cpp`
- `PlussQit/examples/CodeN/note_cleanup_demo.cpp`
- `PlussQit/examples/CodeN/note_method_families_demo.cpp`
- `PlussQit/examples/CodeN/note_printing_demo.cpp`

## Full call pattern index from signatures

```{include} ../generated/note_method_call_patterns.md
```
