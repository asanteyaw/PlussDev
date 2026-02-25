# Note Guide

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/coden/coden.h>
#include <pluss/coden/note_factory.h>
```

## Mental model

`Note` is the text/bytes tensor-like type in PlussQit.

- UTF-8 mode is default (`content`, `make_note`).
- bytes mode is explicit (`bytes_content`, `make_bytes_note`).
- Most operations are method families: cleanup, tokenization, NLP, TeX transforms.

For exhaustive call forms, use [Note: All Methods Usage](note_all_methods_usage).

## Construction quick recipes

```cpp
auto s = pluss::content("hello");
auto v = pluss::content({"a", "b", "c"});
auto m = pluss::content({{"a", "b"}, {"c", "d"}});

std::vector<std::string> flat{"p","q","r","s"};
auto n = pluss::make_note(flat, std::array<std::int64_t,2>{2,2});

auto from_lines = pluss::note_from_lines("line1\nline2\nline3");
auto from_words = pluss::note_from_words("alpha beta gamma");

auto bytes = pluss::bytes_content({{"\x01", "\x02"}, {"\x03", "\x04"}});
```

## High-value method families

```cpp
// cleanup + normalization
auto clean = n.lower().collapse_whitespace().strip();

// tokenization
auto tok = clean.tokenize_words();

// TeX/NLP
auto text = pluss::content("\\textbf{Bold} and $x+y$").tex_to_text();
auto pii  = pluss::content("Email bob@example.com at 9:30am").nlp_mask_pii();
```

## Demos

- `PlussQit/examples/CodeN/note_factory_demo.cpp`
- `PlussQit/examples/CodeN/note_base_methods_demo.cpp`
- `PlussQit/examples/CodeN/note_cleanup_demo.cpp`
- `PlussQit/examples/CodeN/note_method_families_demo.cpp`
- `PlussQit/examples/CodeN/note_printing_demo.cpp`
