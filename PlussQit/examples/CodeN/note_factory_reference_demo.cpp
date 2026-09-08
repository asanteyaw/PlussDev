// Permanent verifier for the Note Factory API Reference
// (docs/sphinx/api/note_factory/*.md). Every deterministic example output on
// those pages is produced here. Add a section per documented example; keep the
// labels stable so a page and its output stay easy to line up.

#include <pluss/pluss.h>

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::note::print(value) << '\n';
}

void show(std::string_view label, const pluss::Note& value,
          const pluss::NotePrintOptions& opt) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::note::print(value, opt) << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  std::cout << "\n=== " << label << " ===\n";
  try {
    fn();
    std::cout << "(no error)\n";
  } catch (const std::exception& e) {
    std::string m = e.what();
    std::cout << m.substr(0, m.find('\n')) << '\n';
  }
}

// ---------------------------------------------------------------- make --------
void section_make() {
  show("make/scalar", pluss::note::make("Final Exam Review"));

  show("make/1d", pluss::note::make({"Ada Lovelace", "Grace Hopper", "Linus"}));

  show("make/2d", pluss::note::make({{"r0c0", "r0c1"}, {"r1c0", "r1c1"}}));

  show("make/3d",
       pluss::note::make({{{"a", "b"}, {"c", "d"}}, {{"e", "f"}, {"g", "h"}}}));

  show("make_note/shortcut", pluss::make_note({"draft", "review", "final"}));

  show("make_note/char", pluss::make_note('Q'));

  {
    std::vector<std::string> flat{"p", "q", "r", "s", "t", "u"};
    std::array<std::int64_t, 2> sizes{2, 3};
    show("make/flat+sizes",
         pluss::note::make(pluss::msharp::RangeView::Of<std::string>(flat),
                           pluss::msharp::RangeView::Of<std::int64_t>(sizes)));
  }

  show("make/utf8", pluss::note::make({"café", "naïve", "日本語", "🎯"}));

  show("make/empty-string", pluss::note::make(""));

  show_rejection("make/reject-ragged",
                 [] { (void)pluss::note::make({{"a", "b"}, {"c"}}); });

  show_rejection("make/reject-flat-mismatch", [] {
    std::vector<std::string> flat{"a", "b", "c"};
    std::array<std::int64_t, 1> sizes{5};
    (void)pluss::note::make(pluss::msharp::RangeView::Of<std::string>(flat),
                            pluss::msharp::RangeView::Of<std::int64_t>(sizes));
  });

  show_rejection("make/reject-4d",
                 [] { (void)pluss::note::make({{{{"a"}}}}); });
}

// --------------------------------------------------------------- bytes --------
void section_bytes() {
  pluss::NotePrintOptions hex;
  hex.bytes_mode = pluss::NotePrintBytesMode::Hex;

  show("bytes/scalar", pluss::note::bytes("plain ascii bytes"));

  show("bytes/1d", pluss::note::bytes({"AB", "\xDE\xAD\xBE\xEF"}));

  show("bytes/1d-hex", pluss::note::bytes({"AB", "\xDE\xAD\xBE\xEF"}), hex);

  {
    std::vector<std::string> flat{"one", "two", "three", "four"};
    std::array<std::int64_t, 2> sizes{2, 2};
    show("bytes/flat+sizes",
         pluss::note::bytes(pluss::msharp::RangeView::Of<std::string>(flat),
                            pluss::msharp::RangeView::Of<std::int64_t>(sizes)));
  }

  {
    std::array<std::byte, 4> raw{std::byte{0x48}, std::byte{0x69},
                                 std::byte{0x21}, std::byte{0x00}};
    show("bytes/raw-span",
         pluss::note::bytes(pluss::msharp::RangeView::Of<std::byte>(raw)));
    show("bytes/raw-span-hex",
         pluss::note::bytes(pluss::msharp::RangeView::Of<std::byte>(raw)), hex);
  }

  show("bytes/nul-truncation", pluss::note::bytes({"\x00\x01\x02", "kept"}), hex);

  show("bytes/escaped-vs-hex/escaped",
       pluss::note::bytes({"\xC3\xA9", "\xFF"}));
  show("bytes/escaped-vs-hex/hex",
       pluss::note::bytes({"\xC3\xA9", "\xFF"}), hex);
}

// --------------------------------------------------------------- shape --------
void section_shape() {
  pluss::NotePrintOptions hex;
  hex.bytes_mode = pluss::NotePrintBytesMode::Hex;

  {
    std::array<std::int64_t, 2> sizes{2, 3};
    show("full/2d",
         pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes),
                           "TODO"));
  }
  {
    std::array<std::int64_t, 0> sizes{};
    show("full/scalar",
         pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes),
                           "x"));
  }
  {
    std::array<std::int64_t, 1> sizes{0};
    show("full/empty-axis",
         pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes),
                           "x"));
  }
  {
    std::array<std::int64_t, 2> sizes{2, 2};
    show("empty/2d",
         pluss::note::empty(pluss::msharp::RangeView::Of<std::int64_t>(sizes)));
  }

  show("rep/1d", pluss::note::rep("-", {5}));
  show("rep/2d", pluss::note::rep("ab", {2, 2}));

  const auto grid = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
  show("full_like", pluss::note::full_like(grid, "?"));
  show("empty_like", pluss::note::empty_like(grid));
  show("full_bytes_like", pluss::note::full_bytes_like(grid, "\xDE\xAD"), hex);
  show("empty_bytes_like", pluss::note::empty_bytes_like(grid));

  show_rejection("full/reject-4d", [] {
    std::array<std::int64_t, 4> sizes{2, 2, 2, 2};
    (void)pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes),
                            "x");
  });
  show_rejection("full/reject-negative", [] {
    std::array<std::int64_t, 1> sizes{-1};
    (void)pluss::note::full(pluss::msharp::RangeView::Of<std::int64_t>(sizes),
                            "x");
  });
}

// ---------------------------------------------------------- generative -------
void section_generative() {
  // rand: one random letter per cell, seeded for determinism.
  show("rand/1d-caps", pluss::note::rand({6}, true, 42));
  show("rand/1d-lower", pluss::note::rand({6}, false, 42));
  show("rand/2d", pluss::note::rand({2, 4}, true, 7));
  show("rand/scalar", pluss::note::rand({}, true, 99));
  show("rand/same-seed-again", pluss::note::rand({6}, true, 42));

  // rand_between: random single chars within an inclusive ASCII range.
  show("rand_between/digits", pluss::note::rand_between("0", "9", {10}, 5));
  show("rand_between/az", pluss::note::rand_between("a", "z", {2, 3}, 5));
  show_rejection("rand_between/reject-lo-gt-hi",
                 [] { (void)pluss::note::rand_between("z", "a", {3}, 1); });
  show_rejection("rand_between/reject-multichar",
                 [] { (void)pluss::note::rand_between("ab", "z", {3}, 1); });

  // arange: a stepped run of single ASCII characters.
  show("arange/az", pluss::note::arange("a", "e"));
  show("arange/step2", pluss::note::arange("a", "i", 2));
  show("arange/exclusive", pluss::note::arange("a", "e", 1, false));
  show("arange/descending", pluss::note::arange("e", "a", -1));
  show("arange/digits", pluss::note::arange("0", "9", 3));
  show("arange/end-not-landed", pluss::note::arange("a", "f", 2));
  show("arange/empty-wrong-direction", pluss::note::arange("e", "a"));
  show_rejection("arange/reject-step0",
                 [] { (void)pluss::note::arange("a", "e", 0); });

  // perm: character combinations / permutations of a word.
  show("perm/combination", pluss::note::perm("abc", 2));
  show("perm/permutation",
       pluss::note::perm("abc", 2, pluss::note::WordSelectMode::Permutation));
  show("perm/with-replacement",
       pluss::note::perm("ab", 2, pluss::note::WordSelectMode::Combination,
                        true));
  show("perm/dedup-default", pluss::note::perm("aab", 2));
  show("perm/pick-zero", pluss::note::perm("abc", 0));
  show("perm/pick-too-large", pluss::note::perm("abc", 4));
  show("perm/non-unique",
       pluss::note::perm("aab", 2, pluss::note::WordSelectMode::Combination,
                        false, false));
}

// --------------------------------------------------------------- split -------
void section_split() {
  show("from_lines/basic", pluss::note::from_lines("alpha\nbeta\ngamma"));
  show("from_lines/trailing-newline", pluss::note::from_lines("alpha\nbeta\n"));
  show("from_lines/blank-kept", pluss::note::from_lines("alpha\n\ngamma"));
  show("from_lines/drop-empty",
       pluss::note::from_lines("alpha\n\ngamma", false, true));
  show("from_lines/keep-ends", pluss::note::from_lines("alpha\nbeta", true));
  show("from_lines/empty", pluss::note::from_lines(""));

  show("from_words/basic", pluss::note::from_words("  the   quick brown  "));
  show("from_words/mixed-space",
       pluss::note::from_words("tab\tand\nnewline"));
  show("from_words/single", pluss::note::from_words("single"));
  show("from_words/empty", pluss::note::from_words("   "));
}

// -------------------------------------------------------------- format -------
void section_format() {
  show("format/plain",
       pluss::note::format("Hello, {name}!", {{"name", "Ada"}}));
  show("format/escape-braces",
       pluss::note::format("{{literal}} and {x}", {{"x", 7}}));
  show("format/arithmetic",
       pluss::note::format("{a} + {b} = {a + b}", {{"a", 2}, {"b", 3}}));
  show("format/division",
       pluss::note::format("{p / q}", {{"p", 10}, {"q", 4}}));
  show("format/string-concat",
       pluss::note::format("{greet + ' ' + who}",
                           {{"greet", "hi"}, {"who", "there"}}));
  show("format/ternary",
       pluss::note::format("{n > 1 ? 'many' : 'one'}", {{"n", 5}}));
  show("format/bool-and-compare",
       pluss::note::format("{ok && n == 3}", {{"ok", true}, {"n", 3}}));
  show("format/vector-index-size",
       pluss::note::format(
           "first={xs[0]} count={xs.size()}",
           {{"xs", std::vector<std::string>{"a", "b", "c"}}}));
  show("format/builtin-sqrt",
       pluss::note::format("{std::sqrt(x)}", {{"x", 16}}));
  show("format/builtin-min-max",
       pluss::note::format("{std::min(a, b, c)}/{std::max(a, b, c)}",
                           {{"a", 4}, {"b", 1}, {"c", 9}}));
  show("format/join",
       pluss::note::format(
           "{pluss::join(xs, '-')}",
           {{"xs", std::vector<std::string>{"x", "y", "z"}}}));
  show("format/accumulate",
       pluss::note::format("{std::accumulate(xs, 0)}",
                           {{"xs", std::vector<int>{1, 2, 3, 4}}}));
  show("format/callable",
       pluss::note::format("{shout('hi')}",
                           {{"shout", [](std::string s) {
                               for (auto& ch : s) ch = std::toupper(ch);
                               return s + "!";
                             }}}));
  show_rejection("format/reject-missing-binding",
                 [] { (void)pluss::note::format("{absent}", {{"x", 1}}); });
  show_rejection("format/reject-unmatched-brace",
                 [] { (void)pluss::note::format("{x", {{"x", 1}}); });
  show_rejection("format/reject-duplicate-binding", [] {
    (void)pluss::note::format("{x}", {{"x", 1}, {"x", 2}});
  });
}

// -------------------------------------------------------------- select -------
void section_select() {
  const auto c_true_false_true = pluss::make_series({true, false, true});

  show("select/single-string-sugar",
       pluss::note::select(c_true_false_true, "YES", "no"));

  show("select/single-note-choice",
       pluss::note::select(c_true_false_true, pluss::note::make("HIT"),
                           pluss::note::make("miss")));

  {
    const auto early = pluss::make_series({true, true, false, false});
    const auto late = pluss::make_series({false, true, true, false});
    show("select/ordered-first-true-wins",
         pluss::note::select({early, late},
                             {pluss::note::make("early"),
                              pluss::note::make("late")},
                             "none"));
  }

  {
    const auto mask = pluss::make_series({{true, false}, {false, true}});
    show("select/broadcast-scalar-choice",
         pluss::note::select(mask, pluss::note::make("X"),
                             pluss::note::make(".")));
  }

  show_rejection("select/reject-non-bool", [] {
    (void)pluss::note::select(pluss::make_series({1, 0, 1}), "y", "n");
  });
  show_rejection("select/reject-count-mismatch", [] {
    const auto a = pluss::make_series({true});
    (void)pluss::note::select({a, a}, {pluss::note::make("x")}, "z");
  });
}

// --------------------------------------------------------------- print -------
void section_print() {
  const auto col = pluss::note::make({"one", "two", "three"});

  show("print/default", col);

  {
    pluss::NotePrintOptions header_only;
    header_only.content_mode = pluss::NotePrintContentMode::HeaderOnly;
    show("print/header-only", col, header_only);
  }
  {
    pluss::NotePrintOptions values_only;
    values_only.content_mode = pluss::NotePrintContentMode::ValuesOnly;
    show("print/values-only", col, values_only);
  }
  {
    pluss::NotePrintOptions no_meta;
    no_meta.metadata_mode = pluss::NotePrintMetadataMode::None;
    show("print/metadata-none", col, no_meta);
  }
  {
    pluss::NotePrintOptions compact;
    compact.metadata_mode = pluss::NotePrintMetadataMode::Compact;
    show("print/metadata-compact", col, compact);
  }
  {
    pluss::NotePrintOptions edge;
    edge.edge_items = 1;
    edge.max_elements = 3;
    show("print/edge-items",
         pluss::note::make({"a", "b", "c", "d", "e", "f"}), edge);
  }
  {
    pluss::NotePrintOptions raw;
    raw.quote_strings = false;
    show("print/no-quotes", col, raw);
  }
  {
    pluss::NotePrintOptions clip;
    clip.max_cell_chars = 12;
    show("print/cell-clip",
         pluss::note::make({"short", "a-much-longer-cell-value"}), clip);
  }

  // The ostream overload writes directly and returns the stream.
  std::cout << "\n=== print/ostream ===\n";
  pluss::note::print(std::cout, col) << '\n';
}

// --------------------------------------------------------- print forms -------
// The reference pages use three interchangeable spellings to send a Note to a
// stream. This proves they render identical text.
void section_print_forms() {
  const auto n = pluss::note::make({"one", "two", "three"});

  const std::string via_print = pluss::note::print(n);      // returns a string

  std::ostringstream via_insert;
  via_insert << n;                                          // operator<<

  std::ostringstream via_stream_overload;
  pluss::note::print(via_stream_overload, n);               // print(os, n)

  std::cout << "\n=== print-forms/identical ===\n"
            << std::boolalpha
            << (via_print == via_insert.str() &&
                via_print == via_stream_overload.str())
            << '\n';
}

}  // namespace

int main() {
  section_make();
  section_bytes();
  section_shape();
  section_generative();
  section_split();
  section_format();
  section_select();
  section_print();
  section_print_forms();
  return 0;
}
