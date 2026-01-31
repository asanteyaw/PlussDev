# pluss::Note — Usage Guide

This document demonstrates the public `Note` API and common use cases: construction, inspection, transformations, and text/NLP helpers.

Files:
- Example code snippets assume you include the umbrella header: `#include <pluss/pluss.h>` or the narrower `#include <pluss/Note.h>` and `#include <pluss/Chord.h>`.

## 1. Creating Notes

// Scalar
Note n1 = make_note("hello");
Note n2 = make_note('x');

// From initializer lists (vector)
Note v = make_note({"a", "b", "c"});

// 2D matrix
Note m = make_note({ {"r1c1", "r1c2"}, {"r2c1", "r2c2"} });

// Using lexicon TextStructure helper
auto t = content(lexicon::TextStructure{"one", "two", "three"});

// From C++ containers (copying)
std::vector<Chord> chords = {Chord("a"), Chord("b")};
Note v2 = make_note(chords);

## 2. Inspecting Notes

// Shape and dimensionality
int ndim = v.ndim();
auto shape = v.shape();

// Flatten to strings
auto flat = v.flatten(); // vector<string>

// Scalar/Vector/Matrix access
Chord s = n1.scalar();
auto vec = v.vector();
auto mat = m.matrix();

// Debug / pretty print
std::cout << v.debug() << "\n";
std::cout << v << std::endl; // operator<< provides metadata + content

## 3. Elementwise transformations

// Case transforms
Note lower = v.to_lower();
Note upper = v.to_upper();

// Map-style (apply function to each Chord)
Note mapped = v.map([](const Chord& c){ return c.to_upper(); });

// Replace pattern (elementwise string replace)
Note replaced = v.replace(make_note("a"), make_note("A"));

// Arithmetic-style dispatch uses helper APIs (dispatch_unary/binary) behind the scenes

## 4. Reshaping and stacking

// Transpose
Note t1 = m.T();

// Stack vectors into higher-dim
Note stacked = dispatch_stack({v, v2}, 0);

// Squeeze/Unsqueeze
Note squeezed = v.squeeze();
Note unsq = v.unsqueeze(0);

## 5. Indexing/slicing

// slice, repeat, stack, where
// Example: slice first two elements of vector
Note first_two = v.slice({0,1});

// where(cond, x, y)
Note cond = make_note({"1", "0", "1"});
Note selected = where(cond, v, make_note("NA"));

## 6. NLP / Text helpers

// Bag-of-words (stopwords removed, lowercased)
auto bow = v.to_bow(); // unordered_map<string,int64>

// Cosine similarity between notes
double cos_sim = v.similarity(v2);

// Jaccard similarity on token sets
double jac = v.similarity_jaccard(v2);

// Regex search across all strings
auto matches = v.regex_search("\\b[a-z]{2}\\b");

// Join flattened strings
std::string joined = v.join(",");

## 7. Factory functions and TextStructure

// `content()` accepts a `lexicon::TextStructure` which can be constructed from initializer lists
Note from_text = content(lexicon::TextStructure{"alpha", "beta"});

// Use `make_note()` family for convenience in tests/examples and code

## 8. Examples and idioms

// Example: token-based search
auto b = make_note("The quick brown fox");
auto tokens = b.flatten(); // {"The quick brown fox"}
// Use lexicon tokenizer directly if needed (internal auxiliary API)

// Example: compute similarity matrix for a list of notes
std::vector<Note> items = {v, v2, from_text};
for (size_t i = 0; i < items.size(); ++i) {
  for (size_t j = 0; j < items.size(); ++j) {
    double s = items[i].similarity(items[j]);
    std::cout << s << (j+1<items.size()? ", " : "\n");
  }
}

## 9. Notes on performance and behavior

- `Note` is a lightweight handle around an intrusive shared `NoteBook` object; copying `Note` is cheap (shared pointer semantics).
- Many operations are implemented as recursive dispatches that allocate new `Text` trees — for very large datasets consider specialized APIs.
- NLP helpers use a lightweight tokenizer and stopword filter (naive stemming); for production-grade text processing, replace or extend tokenization.
- `TextStructure::convert_to_note()` constructs a `Note` from initializer lists and small literals — use `make_note()` for common cases.

## 10. Where to look in the codebase

- Implementation and tests: `src/Note/` and `src/Note/test/`
- Header: `include/pluss/Note.h` and factory helpers in `include/pluss/note_factory.h`
- Lexicon helpers: `include/internals/aux/lexicon/text.h` and `include/internals/aux/lexicon/nlp.h`


---
This document is a quick start reference; if you want, I can also generate a runnable example program under `examples/` that demonstrates these use cases and a small test harness. Would you like that?