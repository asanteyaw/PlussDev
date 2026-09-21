#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

static_assert(std::is_same_v<decltype(
    std::declval<const pluss::Note&>().contains(std::string_view{})),
    pluss::Note>);
static_assert(std::is_same_v<decltype(
    std::declval<const pluss::Note&>() == std::declval<const pluss::Note&>()),
    pluss::Note>);
static_assert(std::is_same_v<decltype(
    std::declval<const pluss::Note&>().any()), pluss::Note>);
// Predicate/comparison methods now return a shape-preserving Bool Note
// (pass 4); flatten to a plain vector<uint8_t> for the REQUIRE comparisons
// below.
std::vector<std::uint8_t> flags(const pluss::Note& b) {
  const auto lanes = b.bools();
  return std::vector<std::uint8_t>(lanes.begin(), lanes.end());
}
}  // namespace

TEST_CASE("CodeN replace/remove family has concrete behavior", "[coden][patch13]") {
  pluss::Note s = pluss::make_note("alpha beta beta");
  REQUIRE(s.replace_first("beta", "X").at(0) == "alpha X beta");
  REQUIRE(s.replace_last("beta", "X").at(0) == "alpha beta X");
  REQUIRE(s.replace_n("beta", "X", 1).at(0) == "alpha X beta");
  REQUIRE(s.remove("beta").at(0) == "alpha  beta");
  REQUIRE(s.remove_all("beta").at(0) == "alpha  ");

  pluss::Note t = pluss::make_note("payload");
  REQUIRE(t.ensure_prefix("pre_").at(0) == "pre_payload");
  REQUIRE(t.ensure_suffix("_suf").at(0) == "payload_suf");
  REQUIRE(t.surround("[", "]").at(0) == "[payload]");
  REQUIRE(t.quote().at(0) == "\"payload\"");
  REQUIRE(t.quote().unquote().at(0) == "payload");
}

TEST_CASE("CodeN tokenization families return structured TokenBatch", "[coden][patch13]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"One.\nTwo!", "A,b;c"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto lines = n.split_lines();
  {
    const auto lanes = lines.offsets().integers();
    REQUIRE(std::vector<std::int64_t>(lanes.begin(), lanes.end()) ==
            std::vector<std::int64_t>({0, 2, 3}));
  }
  REQUIRE(lines.tokens().numel() == 3);
  REQUIRE(lines.tokens().at(0) == "One.");
  REQUIRE(lines.tokens().at(1) == "Two!");
  REQUIRE(lines.tokens().at(2) == "A,b;c");

  auto words = n.nlp().tokens();
  REQUIRE(words.tokens().numel() >= 4);
  REQUIRE(words.tokens().at(0) == "One");
  REQUIRE(words.tokens().at(1) == "Two");

  auto punct = n.nlp().punct_tokens();
  REQUIRE(punct.tokens().numel() >= 4);

  auto findall = n.regex().findall("[A-Za-z]+");
  REQUIRE(findall.tokens().numel() >= 4);
  REQUIRE(findall.tokens().at(0) == "One");

  auto split_re = n.regex().split("[,;]");
  REQUIRE(split_re.tokens().numel() >= 4);
}

TEST_CASE("CodeN TeX cleanup family has concrete behavior", "[coden][patch13]") {
  pluss::Note tex = pluss::make_note(
      "\\documentclass{article}\n"
      "% header comment\n"
      "\\begin{document}\n"
      "Text \\textbf{bold} and $x+y$.\n"
      "\\end{document}\n");

  auto stripped = tex.tex().strip_comments();
  REQUIRE(stripped.at(0).find("% header comment") == std::string::npos);

  auto body = stripped.tex().strip_preamble();
  REQUIRE(body.at(0).find("\\documentclass") == std::string::npos);

  auto plain = body.tex().to_text();
  REQUIRE(plain.at(0).find('\\') == std::string::npos);
  REQUIRE(plain.at(0).find("Text") != std::string::npos);
  REQUIRE(plain.at(0).find("bold") != std::string::npos);
}

TEST_CASE("CodeN tokenize/regex/tex parameterized overloads take effect", "[coden][patch13]") {
  // regex_findall(pattern, flags): bit 0 = case-insensitive.
  pluss::Note s = pluss::make_note("AbCdE");
  REQUIRE(s.regex().findall("[a-z]+").tokens().numel() == 2);     // "b", "d"
  REQUIRE(s.regex().findall("[a-z]+", 0).tokens().numel() == 2);
  auto ci = s.regex().findall("[a-z]+", 1);
  REQUIRE(ci.tokens().numel() == 1);
  REQUIRE(ci.tokens().at(0) == "AbCdE");

  // tokenize_words(flags): bit 0 = case-fold each token.
  pluss::Note w = pluss::make_note("One TWO three");
  REQUIRE(w.nlp().tokens().tokens().at(1) == "TWO");
  auto wl = w.nlp().tokens(1);
  REQUIRE(wl.tokens().at(0) == "one");
  REQUIRE(wl.tokens().at(1) == "two");
  REQUIRE(wl.tokens().at(2) == "three");

  // tex().to_text(mode): mode >= 1 also case-folds the recovered text.
  pluss::Note t = pluss::make_note("\\textbf{Hello} World");
  REQUIRE(t.tex().to_text().at(0) == "Hello World");
  REQUIRE(t.tex().to_text(1).at(0) == "hello world");
}

TEST_CASE("CodeN capitalize matches Python str.capitalize semantics", "[coden][patch13]") {
  // First character upper-cased in place; the rest lower-cased. A leading
  // digit or space is left as-is (no "first letter" skipping).
  REQUIRE(pluss::make_note("heLLo wORLd").capitalize().at(0) == "Hello world");
  REQUIRE(pluss::make_note("3rd place").capitalize().at(0) == "3rd place");
  REQUIRE(pluss::make_note("ALL_CAPS").capitalize().at(0) == "All_caps");
}

TEST_CASE("CodeN lengths_graphemes clusters combining marks", "[coden][patch13]") {
  // "e" + U+0301 (combining acute) is one grapheme but two code points.
  pluss::Note n = pluss::make_note(std::string("e\xCC\x81"));
  REQUIRE(n.lengths_chars().int_item() == 2);
  REQUIRE(n.lengths_graphemes().int_item() == 1);
  // Pure ASCII: graphemes == chars == bytes.
  REQUIRE(pluss::make_note("abc").lengths_graphemes().int_item() == 3);
}

TEST_CASE("CodeN regex_replace(pattern, repl, flags) honors the flags bitmask", "[coden][patch13]") {
  pluss::Note n = pluss::make_note("Hello HELLO hello");
  REQUIRE(n.regex().replace("hello", "hi").at(0) == "Hello HELLO hi");
  REQUIRE(n.regex().replace("hello", "hi", 0).at(0) == "Hello HELLO hi");
  REQUIRE(n.regex().replace("hello", "hi", 1).at(0) == "hi hi hi");  // bit 0 = icase
  // Capture-group back-references still work.
  REQUIRE(pluss::make_note("2026-08-02")
              .regex().replace(R"((\d{4})-(\d{2})-(\d{2}))", "$3/$2/$1")
              .at(0) == "02/08/2026");
}

TEST_CASE("CodeN character-property predicates match Python str.isX semantics", "[coden][patch13]") {
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"abc", "Abc", "123", "", "  ", "ABC"},
      std::array<std::int64_t, 1>{6});

  REQUIRE(flags(n.is_alpha())  == std::vector<std::uint8_t>({1, 1, 0, 0, 0, 1}));
  REQUIRE(flags(n.is_alnum())  == std::vector<std::uint8_t>({1, 1, 1, 0, 0, 1}));
  REQUIRE(flags(n.is_digit())  == std::vector<std::uint8_t>({0, 0, 1, 0, 0, 0}));
  REQUIRE(flags(n.is_lower())  == std::vector<std::uint8_t>({1, 0, 0, 0, 0, 0}));
  REQUIRE(flags(n.is_upper())  == std::vector<std::uint8_t>({0, 0, 0, 0, 0, 1}));
  // is_title: needs at least one cased word start; "123" is NOT title.
  REQUIRE(flags(n.is_title())  == std::vector<std::uint8_t>({0, 1, 0, 0, 0, 0}));
  // is_whitespace: the empty string is NOT whitespace; is_blank includes it.
  REQUIRE(flags(n.is_whitespace()) == std::vector<std::uint8_t>({0, 0, 0, 0, 1, 0}));
  REQUIRE(flags(n.is_blank())      == std::vector<std::uint8_t>({0, 0, 0, 1, 1, 0}));

  // Each predicate/comparison Note above also carries the shape of the
  // receiver, and dtype Bool -- not just the flattened values.
  REQUIRE(n.is_alpha().is_bool());
  REQUIRE(n.is_alpha().sizes() == std::vector<std::int64_t>{6});

  // regex_match is anchored (whole cell), regex_search is unanchored.
  pluss::Note r = pluss::make_note(
      std::vector<std::string>{"2026", "year 2026"},
      std::array<std::int64_t, 1>{2});
  REQUIRE(flags(r.regex().match(R"(\d+)"))  == std::vector<std::uint8_t>({1, 0}));
  REQUIRE(flags(r.regex().search(R"(\d+)")) == std::vector<std::uint8_t>({1, 1}));
}

TEST_CASE("CodeN comparisons produce shape-preserving broadcasting Bool Notes",
          "[coden][patch13][pass4]") {
  pluss::Note a = pluss::make_note(
      std::vector<std::string>{"a", "b", "c"}, std::array<std::int64_t, 1>{3});
  pluss::Note b = pluss::make_note(
      std::vector<std::string>{"a", "x", "c"}, std::array<std::int64_t, 1>{3});

  const auto eq = a == b;
  REQUIRE(eq.is_bool());
  REQUIRE(eq.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(flags(eq) == std::vector<std::uint8_t>({1, 0, 1}));

  // Scalar broadcast: either side may be a single-element Note, and the
  // result takes the shape of the non-scalar operand.
  const auto scalar_eq = a == "a";
  REQUIRE(scalar_eq.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(flags(scalar_eq) == std::vector<std::uint8_t>({1, 0, 0}));
  REQUIRE(flags("a" == a) == flags(scalar_eq));

  // != is derived from == via operator~ (bitwise Bool negation) and must
  // agree elementwise.
  REQUIRE(flags(a != b) == std::vector<std::uint8_t>({0, 1, 0}));
  REQUIRE(flags("a" != a) == std::vector<std::uint8_t>({0, 1, 1}));

  // Native bitwise combinators over Bool Notes, with the same scalar
  // broadcast rule as the comparisons above.
  pluss::Note p = pluss::note::make(std::vector<bool>{true, false, true});
  pluss::Note q = pluss::note::make(std::vector<bool>{true, true, false});
  REQUIRE(flags(p & q) == std::vector<std::uint8_t>({1, 0, 0}));
  REQUIRE(flags(p | q) == std::vector<std::uint8_t>({1, 1, 1}));
  REQUIRE(flags(p ^ q) == std::vector<std::uint8_t>({0, 1, 1}));
  REQUIRE(flags(~p)    == std::vector<std::uint8_t>({0, 1, 0}));

  pluss::Note scalar_true = pluss::note::make(true);
  REQUIRE(flags(p & scalar_true) == flags(p));
  REQUIRE((p & scalar_true).sizes() == p.sizes());

  REQUIRE(p.any().is_bool());
  REQUIRE(p.any().dim() == 0);
  REQUIRE(p.any().bool_item());
  REQUIRE_FALSE(p.all().bool_item());
  REQUIRE((p | ~p).all().bool_item());

  const auto empty = pluss::note::make(
      std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
  REQUIRE_FALSE(empty.any().bool_item());
  REQUIRE(empty.all().bool_item());

  REQUIRE_THROWS(p & a);  // mixed Bool/Text operands are rejected

  // Equal cardinality is not broadcasting: differently shaped arrays must
  // fail before an output allocation is observable.
  const auto matrix = pluss::note::make(
      std::vector<bool>{true, false, true, false},
      std::array<std::int64_t, 2>{2, 2});
  const auto vector = pluss::note::make(
      std::vector<bool>{true, false, true, false});
  REQUIRE_THROWS(matrix & vector);
}

TEST_CASE("Note text operators require compatible shapes and text storage",
          "[coden][operators]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});
  const auto vector = pluss::note::make({"1", "2", "3", "4"});

  REQUIRE_THROWS(matrix + vector);
  REQUIRE_THROWS(pluss::note::make(std::vector<bool>{true, false}) * 2);
  REQUIRE_THROWS(pluss::note::make(std::vector<std::int64_t>{1, 2}) +
                 pluss::note::make(std::vector<std::int64_t>{3, 4}));

  const auto suffix = pluss::note::make("!");
  REQUIRE((matrix + suffix).sizes() == matrix.sizes());
}

TEST_CASE("every content predicate preserves rank and validates its domain",
          "[coden][patch13][pass4]") {
  const auto text = pluss::note::make(
      std::vector<std::string>{"abc", "123", "", " A "},
      std::array<std::int64_t, 2>{2, 2});
  const std::vector<pluss::Note> results{
      text.contains("a"), text.starts_with("a"), text.ends_with("3"),
      text.regex().match("[a-z]+"), text.regex().search("[A-Z]"),
      text.is_ascii(), text.is_utf8_valid(), text.is_blank(),
      text.is_alpha(), text.is_alnum(), text.is_digit(), text.is_lower(),
      text.is_upper(), text.is_title(), text.is_whitespace(),
      text.contains_any_digit(), text.contains_any_alpha(),
  };
  REQUIRE(results.size() == 17);
  for (const auto& result : results) {
    REQUIRE(result.is_bool());
    REQUIRE(result.sizes() == std::vector<std::int64_t>{2, 2});
  }

  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{0, 4});
  REQUIRE(empty.contains("x").sizes() ==
          std::vector<std::int64_t>{0, 4});
  REQUIRE(empty.is_ascii().bools().empty());

  REQUIRE_THROWS(pluss::Note::undefined().is_alpha());
  REQUIRE_THROWS(pluss::note::make({true, false}).contains("true"));
  REQUIRE_THROWS(text.regex().match("["));

  const std::string raw{"A\0B", 3};
  const auto bytes = pluss::note::bytes(raw);
  REQUIRE(bytes.contains(std::string_view{"\0", 1}).bool_item());
  REQUIRE(bytes.is_ascii().bool_item());
  REQUIRE_THROWS(bytes.regex().search("A"));
}

TEST_CASE("all comparison call forms preserve operand order",
          "[coden][patch13][pass4]") {
  const auto values = pluss::note::make({"a", "b", "c"});
  const auto peers = pluss::note::make({"a", "c", "b"});

  REQUIRE((flags(values == peers) == std::vector<std::uint8_t>{1, 0, 0}));
  REQUIRE((flags(values != peers) == std::vector<std::uint8_t>{0, 1, 1}));
  REQUIRE((flags(values < peers) == std::vector<std::uint8_t>{0, 1, 0}));
  REQUIRE((flags(values <= peers) == std::vector<std::uint8_t>{1, 1, 0}));
  REQUIRE((flags(values > peers) == std::vector<std::uint8_t>{0, 0, 1}));
  REQUIRE((flags(values >= peers) == std::vector<std::uint8_t>{1, 0, 1}));

  REQUIRE((flags(values == "b") == std::vector<std::uint8_t>{0, 1, 0}));
  REQUIRE((flags(values != "b") == std::vector<std::uint8_t>{1, 0, 1}));
  REQUIRE((flags(values < "b") == std::vector<std::uint8_t>{1, 0, 0}));
  REQUIRE((flags(values <= "b") == std::vector<std::uint8_t>{1, 1, 0}));
  REQUIRE((flags(values > "b") == std::vector<std::uint8_t>{0, 0, 1}));
  REQUIRE((flags(values >= "b") == std::vector<std::uint8_t>{0, 1, 1}));

  REQUIRE((flags("b" == values) == std::vector<std::uint8_t>{0, 1, 0}));
  REQUIRE((flags("b" != values) == std::vector<std::uint8_t>{1, 0, 1}));
  REQUIRE((flags("b" < values) == std::vector<std::uint8_t>{0, 0, 1}));
  REQUIRE((flags("b" <= values) == std::vector<std::uint8_t>{0, 1, 1}));
  REQUIRE((flags("b" > values) == std::vector<std::uint8_t>{1, 0, 0}));
  REQUIRE((flags("b" >= values) == std::vector<std::uint8_t>{1, 1, 0}));
}

TEST_CASE("CodeN Note-vs-Note comparisons are dtype-generic, not text-only",
          "[coden][patch13][pass4]") {
  // Int64: value comparison, not lexical ("9" < "10" would be true lexically
  // but 9 < 10 numerically is also true here -- pick values where the two
  // orderings disagree to prove it's genuinely numeric).
  pluss::Note x = pluss::note::make(std::vector<int>{2, 10, 3}, std::array<std::int64_t, 1>{3});
  pluss::Note y = pluss::note::make(std::vector<int>{2, 9, 30}, std::array<std::int64_t, 1>{3});
  REQUIRE(flags(x == y) == std::vector<std::uint8_t>({1, 0, 0}));
  REQUIRE(flags(x < y)  == std::vector<std::uint8_t>({0, 0, 1}));
  REQUIRE(flags(x > y)  == std::vector<std::uint8_t>({0, 1, 0}));
  REQUIRE(flags(x >= y) == std::vector<std::uint8_t>({1, 1, 0}));

  // Scalar broadcast works for numeric dtypes too.
  pluss::Note scalar_ten = pluss::note::make(10);
  REQUIRE(flags(x < scalar_ten) == std::vector<std::uint8_t>({1, 0, 1}));

  // Bool: false < true.
  pluss::Note p = pluss::note::make(std::vector<bool>{true, false, true});
  pluss::Note q = pluss::note::make(std::vector<bool>{false, false, true});
  REQUIRE(flags(p == q) == std::vector<std::uint8_t>({0, 1, 1}));
  REQUIRE(flags(p > q)  == std::vector<std::uint8_t>({1, 0, 0}));

  // Float64: normal IEEE-754 ordering, including NaN's usual non-reflexive
  // equality (NaN != NaN, every ordered comparison against NaN is false).
  pluss::Note f = pluss::note::make(
      std::vector<double>{1.5, 2.5, std::numeric_limits<double>::quiet_NaN()},
      std::array<std::int64_t, 1>{3});
  pluss::Note g = pluss::note::make(std::vector<double>{1.5, 3.5, 0.0}, std::array<std::int64_t, 1>{3});
  REQUIRE(flags(f == g) == std::vector<std::uint8_t>({1, 0, 0}));
  REQUIRE(flags(f < g)  == std::vector<std::uint8_t>({0, 1, 0}));
  REQUIRE(flags(f == f) == std::vector<std::uint8_t>({1, 1, 0}));  // NaN != NaN

  // Every comparison Note above carries the correct shape and Bool dtype,
  // exactly like the Text case -- this was the gap: it used to throw
  // ("operation requires Text or Bytes storage") for any non-text dtype.
  REQUIRE((x == y).is_bool());
  REQUIRE((x == y).sizes() == std::vector<std::int64_t>{3});

  // Mixed numeric dtypes are rejected, same as mixed Text/Bytes.
  REQUIRE_THROWS(x == p);
  REQUIRE_THROWS(x == f);

  // Comparing a numeric Note against a string literal is rejected with a
  // purpose-built message rather than an incidental accessor error.
  REQUIRE_THROWS(x == "2");

  const auto matrix = pluss::note::make(
      std::vector<std::int64_t>{1, 2, 3, 4},
      std::array<std::int64_t, 2>{2, 2});
  const auto vector = pluss::note::make(
      std::vector<std::int64_t>{1, 2, 3, 4});
  REQUIRE_THROWS(matrix == vector);
}

TEST_CASE("CodeN measurement/count/ordering methods produce shape-preserving Int64 Notes",
          "[coden][patch13][pass5]") {
  pluss::Note words = pluss::make_note(
      std::vector<std::string>{"a", "bb", "ccc"}, std::array<std::int64_t, 1>{3});

  const auto byte_lengths = words.lengths_bytes();
  REQUIRE(byte_lengths.is_int64());
  REQUIRE(byte_lengths.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(std::vector<std::int64_t>(byte_lengths.integers().begin(), byte_lengths.integers().end()) ==
          std::vector<std::int64_t>({1, 2, 3}));

  pluss::Note text = pluss::make_note(
      std::vector<std::string>{"banana", "apple", "banana split"},
      std::array<std::int64_t, 1>{3});
  const auto counts = text.count_substring("an");
  REQUIRE(counts.is_int64());
  REQUIRE(counts.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(std::vector<std::int64_t>(counts.integers().begin(), counts.integers().end()) ==
          std::vector<std::int64_t>({2, 0, 2}));

  const auto re_counts = text.regex().count(R"(an)");
  REQUIRE(re_counts.is_int64());
  REQUIRE(std::vector<std::int64_t>(re_counts.integers().begin(), re_counts.integers().end()) ==
          std::vector<std::int64_t>({2, 0, 2}));

  // Scalar receiver -> scalar measurement Note, checked via the scalar host
  // accessor rather than a shape-comparison.
  REQUIRE(pluss::make_note("hello").lengths_chars().int_item() == 5);

  // argsort: ordering positions as a closed Int64 Note, no Series interop.
  pluss::Note letters = pluss::make_note(
      std::vector<std::string>{"c", "a", "b"}, std::array<std::int64_t, 1>{3});
  const auto order = letters.argsort();
  REQUIRE(order.is_int64());
  REQUIRE(order.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(std::vector<std::int64_t>(order.integers().begin(), order.integers().end()) ==
          std::vector<std::int64_t>({1, 2, 0}));
  const auto sorted = letters.take(order);
  REQUIRE(sorted.vector() == std::vector<std::string_view>{"a", "b", "c"});

  // take(Note) is the closed consumption path for an Int64 index Note and
  // preserves the receiver dtype for every native lane family.
  const auto pick = pluss::note::make({2, 0});
  const auto flags = pluss::note::make({true, false, true}).take(pick);
  REQUIRE(flags.is_bool());
  REQUIRE(std::vector<std::uint8_t>(flags.bools().begin(), flags.bools().end()) ==
          std::vector<std::uint8_t>{1, 1});
  const auto ints = pluss::note::make({10, 20, 30}).take(pick);
  REQUIRE(ints.is_int64());
  REQUIRE(std::vector<std::int64_t>(ints.integers().begin(), ints.integers().end()) ==
          std::vector<std::int64_t>{30, 10});
  const auto reals = pluss::note::make({1.5, 2.5, 3.5}).take(pick);
  REQUIRE(reals.is_float64());
  REQUIRE(std::vector<double>(reals.reals().begin(), reals.reals().end()) ==
          std::vector<double>{3.5, 1.5});
  REQUIRE_THROWS(letters.take(pluss::note::make({true, false})));
}

TEST_CASE("CodeN character-class filters keep the right classes", "[coden][patch13]") {
  pluss::Note n = pluss::make_note("Hello, World! 42 café");
  REQUIRE(n.remove_punctuation().at(0) == "Hello World 42 café");  // keeps letters/digits/space/non-ASCII
  REQUIRE(n.remove_digits().at(0)      == "Hello, World!  café");
  REQUIRE(n.remove_letters().at(0)     == ", ! 42 é");             // ASCII letters only; é byte kept
  REQUIRE(n.remove_symbols().at(0)     == "Hello World 42 café");  // strips ASCII punct, keeps non-ASCII
  REQUIRE(n.keep_alnum().at(0)         == "HelloWorld42caf");      // ASCII alnum only, no spaces
  REQUIRE(n.keep_digits().at(0)        == "42");
  REQUIRE(pluss::make_note("café résumé").keep_ascii().at(0) == "caf rsum");
}

TEST_CASE("CodeN whitespace/layout normalization family", "[coden][patch13]") {
  // collapse_whitespace trims and folds any whitespace run to one space.
  REQUIRE(pluss::make_note("  a  b\tc \n").collapse_whitespace().at(0) == "a b c");
  // normalize_spaces is the same operation.
  REQUIRE(pluss::make_note("  a  b\tc \n").collapse_whitespace().at(0) == "a b c");
  // collapse_spaces folds space/tab runs only; newlines and edges untouched.
  REQUIRE(pluss::make_note("a   b\n c").collapse_spaces().at(0) == "a b\n c");
  REQUIRE(pluss::make_note("a\n\n\n\nb").collapse_newlines().at(0) == "a\nb");
  REQUIRE(pluss::make_note("aaabbb!!!").collapse_repeats().at(0) == "ab!");
  REQUIRE(pluss::make_note("wait...  really?!").collapse_punct().at(0) == "wait.  really?");
  REQUIRE(pluss::make_note("    x\n    y\n      z").dedent().at(0) == "x\ny\n  z");
  REQUIRE(pluss::make_note("l1\nl2").indent().at(0) == "  l1\n  l2");
  // remove_control_chars drops C0 controls but keeps \t \n \r and non-ASCII.
  REQUIRE(pluss::make_note(std::string("a\x01" "b\tc café")).remove_control_chars().at(0) == "ab\tc café");
  // to_ascii: one '?' per non-ASCII character.
  REQUIRE(pluss::make_note("café").to_ascii().at(0) == "caf?");
}

TEST_CASE("CodeN Unicode normalization family", "[coden][patch13]") {
  const std::string decomposed = std::string("caf") + "e\xCC\x81";  // "e" + U+0301
  // to_nfc composes: 6 bytes -> 5; to_nfd is the inverse.
  REQUIRE(pluss::make_note(decomposed).to_nfc().lengths_bytes().int_item() == 5);
  REQUIRE(pluss::make_note("café").to_nfd().lengths_bytes().int_item() == 6);
  REQUIRE(pluss::make_note("café").to_nfc().lengths_bytes().int_item() == 5);
  // to_nfkc / to_nfkd apply compatibility folds.
  REQUIRE(pluss::make_note(std::string("\xEF\xAC\x81") + "le").to_nfkc().at(0) == "file");
  REQUIRE(pluss::make_note("\xC2\xB2").to_nfkd().at(0) == "2");
  // strip_accents: Latin-1 folded; Latin Extended-A only after to_nfd.
  REQUIRE(pluss::make_note("café señor").strip_accents().at(0) == "cafe senor");
  REQUIRE(pluss::make_note("ĥ").strip_accents().at(0) == "ĥ");
  REQUIRE(pluss::make_note("ĥ").to_nfd().strip_accents().at(0) == "h");
  // strip_bom: leading only.
  REQUIRE(pluss::make_note(std::string("\xEF\xBB\xBF") + "x").strip_bom().at(0) == "x");
  REQUIRE(pluss::make_note(std::string("y") + "\xEF\xBB\xBF").strip_bom().at(0) ==
          std::string("y") + "\xEF\xBB\xBF");
  // punctuation normalisers.
  REQUIRE(pluss::make_note(std::string("\xE2\x80\x9C") + "q" + "\xE2\x80\x9D").normalize_quotes().at(0) == "\"q\"");
  REQUIRE(pluss::make_note(std::string("a") + "\xE2\x80\x94" + "b").normalize_dashes().at(0) == "a-b");
  REQUIRE(pluss::make_note(std::string("hm") + "\xE2\x80\xA6").normalize_ellipsis().at(0) == "hm...");
}

namespace {
// Collect a TokenBatch's flat tokens into a vector for order-sensitive checks.
std::vector<std::string> batch_tokens(const pluss::coden::TokenBatch& b) {
  std::vector<std::string> out;
  for (std::int64_t i = 0; i < b.tokens().numel(); ++i) out.push_back(std::string(b.tokens().at(i)));
  return out;
}
// offsets() is a closed Int64 Note (pass 6); flatten for REQUIRE comparisons.
std::vector<std::int64_t> batch_offsets(const pluss::coden::TokenBatch& b) {
  const auto lanes = b.offsets().integers();
  return std::vector<std::int64_t>(lanes.begin(), lanes.end());
}
}  // namespace

TEST_CASE("CodeN diff/align family tags and shape", "[coden][patch13]") {
  // diff_words: only '-' (in self) and '+' (in other) tokens; punctuation dropped.
  auto dw = pluss::make_note("the quick brown fox")
                .nlp().diff_words(pluss::make_note("the slow brown fox"));
  REQUIRE(batch_tokens(dw) == std::vector<std::string>({"-quick", "+slow"}));
  REQUIRE(batch_offsets(dw) == std::vector<std::int64_t>({0, 2}));

  // align_tokens: same alignment, but unchanged words are kept as '=' tokens.
  auto at = pluss::make_note("the quick brown fox")
                .nlp().align_tokens(pluss::make_note("the slow brown fox"));
  REQUIRE(batch_tokens(at) ==
          std::vector<std::string>({"=the", "-quick", "+slow", "=brown", "=fox"}));

  // Identical cells: diff_* yields an empty range, align_* yields all '='.
  REQUIRE(pluss::make_note("no change").nlp().diff_words(pluss::make_note("no change"))
              .tokens().numel() == 0);
  REQUIRE(batch_tokens(pluss::make_note("no change")
                           .nlp().align_tokens(pluss::make_note("no change"))) ==
          std::vector<std::string>({"=no", "=change"}));

  // Punctuation-only edits are invisible to diff_words.
  REQUIRE(pluss::make_note("stop here").nlp().diff_words(pluss::make_note("stop, here!"))
              .tokens().numel() == 0);

  // Scalar broadcast on the right-hand side.
  auto bc = pluss::make_note(std::vector<std::string>{"a b c", "a x c"},
                             std::array<std::int64_t, 1>{2})
                .nlp().diff_words(pluss::make_note("a b c"));
  REQUIRE(batch_offsets(bc) == std::vector<std::int64_t>({0, 0, 2}));
  REQUIRE(batch_tokens(bc) == std::vector<std::string>({"-x", "+b"}));

  // diff_lines splits on '\n'; diff_chars works per character.
  auto dl = pluss::make_note("alpha\nbeta\ngamma")
                .diff_lines(pluss::make_note("alpha\nBETA\ngamma\ndelta"));
  REQUIRE(batch_tokens(dl) == std::vector<std::string>({"-beta", "+BETA", "+delta"}));

  auto dc = pluss::make_note("kitten").diff_chars(pluss::make_note("sitting"));
  REQUIRE(batch_tokens(dc) ==
          std::vector<std::string>({"-k", "+s", "-e", "+i", "+g"}));

  // align_sentences keeps '=' sentences.
  auto as = pluss::make_note("One. Two. Three.")
                .nlp().align_sentences(pluss::make_note("One. Two changed. Three."));
  REQUIRE(batch_tokens(as) ==
          std::vector<std::string>({"=One.", "-Two.", "+Two changed.", "=Three."}));

  // Rejections: bytes-mode lhs, and numel mismatch with neither side scalar.
  REQUIRE_THROWS_AS(pluss::note::bytes({"x"}).nlp().diff_words(pluss::make_note("y")),
                    pluss::Error);
  REQUIRE_THROWS_AS(
      pluss::make_note(std::vector<std::string>{"a", "b"}, std::array<std::int64_t, 1>{2})
          .nlp().diff_words(pluss::make_note(std::vector<std::string>{"a", "b", "c"},
                                      std::array<std::int64_t, 1>{3})),
      pluss::Error);
}

TEST_CASE("CodeN TokenBatch offsets is a closed Int64 Note", "[coden][patch13][pass6]") {
  pluss::Note docs = pluss::make_note(
      std::vector<std::string>{"a bb", "ccc", ""}, std::array<std::int64_t, 1>{3});
  const auto batch = docs.nlp().tokens();

  // offsets() is a real Note -- typed, shaped, and dtype-tagged -- not a
  // borrowed std::vector<int64_t>&.
  const auto& offsets = batch.offsets();
  REQUIRE(offsets.defined());
  REQUIRE(offsets.is_int64());
  REQUIRE(offsets.dim() == 1);
  REQUIRE(offsets.sizes() == std::vector<std::int64_t>{docs.numel() + 1});

  // CSR-style row boundaries: offsets[i]..offsets[i+1] indexes tokens() for
  // row i, and the two Notes agree on total token count.
  const auto lanes = offsets.integers();
  REQUIRE(lanes.front() == 0);
  REQUIRE(lanes.back() == batch.tokens().numel());
  REQUIRE(std::is_sorted(lanes.begin(), lanes.end()));

  // Calling offsets() twice returns the same underlying storage (a cheap
  // handle, not a fresh copy/rebuild per call).
  REQUIRE(offsets.integers().data() == batch.offsets().integers().data());
}

TEST_CASE("CodeN set-similarity family: intersect / jaccard / overlap", "[coden][patch13]") {
  // intersect_tokens: shared words, self order, deduped, case-folded.
  auto it = pluss::make_note("fox fox dog cat fox").nlp().intersect_tokens(pluss::make_note("cat FOX"));
  REQUIRE(batch_tokens(it) == std::vector<std::string>({"fox", "cat"}));

  // intersect_terms drops stopwords and words shorter than 4.
  auto trm = pluss::make_note("The cat sat on the warm mat")
                 .nlp().intersect_terms(pluss::make_note("a warm mat and a cat"));
  REQUIRE(batch_tokens(trm) == std::vector<std::string>({"warm"}));

  // intersect_acronyms: 2+ uppercase runs, lowercased; empty without caps.
  REQUIRE(batch_tokens(pluss::make_note("NASA ESA NOAA").nlp().intersect_acronyms(pluss::make_note("ESA NASA"))) ==
          std::vector<std::string>({"nasa", "esa"}));
  REQUIRE(pluss::make_note("nasa esa").nlp().intersect_acronyms(pluss::make_note("nasa")).tokens().numel() == 0);

  // intersect_citations keeps brackets.
  REQUIRE(batch_tokens(pluss::make_note("[1] [2] [10]").nlp().intersect_citations(pluss::make_note("[2] [3] [10]"))) ==
          std::vector<std::string>({"[2]", "[10]"}));

  // jaccard_tokens = |A n B| / |A u B|; symmetric; edge cases. Now a native
  // Float64 Note (pass 8 cleanup) rather than a formatted Text Note.
  REQUIRE(pluss::make_note("the quick brown fox").nlp().jaccard_tokens(pluss::make_note("the quick red fox")).float_item() == 0.6);
  REQUIRE(pluss::make_note("a b c").nlp().jaccard_tokens(pluss::make_note("d e f")).float_item() == 0.0);
  REQUIRE(pluss::make_note("x y").nlp().jaccard_tokens(pluss::make_note("y x")).float_item() == 1.0);
  REQUIRE(pluss::make_note("").nlp().jaccard_tokens(pluss::make_note("")).float_item() == 1.0);        // both empty
  REQUIRE(pluss::make_note("a b").nlp().jaccard_tokens(pluss::make_note("")).float_item() == 0.0);     // one empty

  // overlap_tokens = |A n B| / min(|A|,|B|): 1.0 when the smaller set is contained.
  REQUIRE(pluss::make_note("quick brown fox").nlp().overlap_tokens(pluss::make_note("the quick brown fox jumps")).float_item() == 1.0);
  const auto overlap_two_thirds = pluss::make_note("a b c d").nlp().overlap_tokens(pluss::make_note("c d e")).float_item();
  REQUIRE((overlap_two_thirds > 0.6666 && overlap_two_thirds < 0.6667));
  REQUIRE(pluss::make_note("a b").nlp().overlap_tokens(pluss::make_note("")).float_item() == 0.0);

  // jaccard / overlap preserve input shape; intersect flattens to a TokenBatch.
  auto j2d = pluss::make_note(std::vector<std::string>{"cat dog", "sun moon", "red blue", "up down"},
                              std::array<std::int64_t, 2>{2, 2})
                 .nlp().jaccard_tokens(pluss::make_note(std::vector<std::string>{"cat fish", "sun moon", "red green", "left right"},
                                                  std::array<std::int64_t, 2>{2, 2}));
  REQUIRE(j2d.dim() == 2);
  REQUIRE(j2d.numel() == 4);
  REQUIRE(j2d.is_float64());
  REQUIRE(j2d.reals()[1] == 1.0);   // "sun moon" vs "sun moon"

  // Rejections mirror the diff/align family.
  REQUIRE_THROWS_AS(pluss::note::bytes({"x"}).nlp().jaccard_tokens(pluss::make_note("y")), pluss::Error);
  REQUIRE_THROWS_AS(
      pluss::make_note(std::vector<std::string>{"a", "b"}, std::array<std::int64_t, 1>{2})
          .nlp().intersect_tokens(pluss::make_note(std::vector<std::string>{"a", "b", "c"}, std::array<std::int64_t, 1>{3})),
      pluss::Error);
}

TEST_CASE("CodeN encoding / transcode family", "[coden][patch13]") {
  // html_escape: exactly five characters; non-ASCII untouched; not idempotent.
  REQUIRE(pluss::make_note("a & b < c > \" '").html_escape().at(0) ==
          "a &amp; b &lt; c &gt; &quot; &#39;");
  REQUIRE(pluss::make_note("café — €").html_escape().at(0) == "café — €");
  REQUIRE(pluss::make_note("<a>").html_escape().html_escape().at(0) == "&amp;lt;a&amp;gt;");

  // html_unescape: named + numeric (dec/hex); unknown left verbatim; round-trips.
  REQUIRE(pluss::make_note("&amp;&lt;&gt;&quot;&#39;&apos;").html_unescape().at(0) == "&<>\"''");
  REQUIRE(pluss::make_note("&#65;&#x41;&#8364;").html_unescape().at(0) == "AA€");
  REQUIRE(pluss::make_note("&foo; &amp x").html_unescape().at(0) == "&foo; &amp x");
  REQUIRE(pluss::make_note("x \" & < y").html_escape().html_unescape().at(0) == "x \" & < y");

  // json_escape: quotes/backslash, short control forms, \u00XX for the rest.
  REQUIRE(pluss::make_note(std::string("a\"b\\c")).json_escape().at(0) == "a\\\"b\\\\c");
  REQUIRE(pluss::make_note(std::string("t\ta\nb")).json_escape().at(0) == "t\\ta\\nb");
  REQUIRE(pluss::make_note(std::string("x") + '\x07' + "y").json_escape().at(0) == "x\\u0007y");
  REQUIRE(pluss::make_note("café").json_escape().at(0) == "café");  // non-ASCII passthrough

  // to_utf8_strict: bytes -> text when valid, raises otherwise.
  auto strict = pluss::note::bytes({"café", "abc"}).to_utf8_strict();
  REQUIRE(strict.at(0) == "café");
  REQUIRE_FALSE(strict.is_bytes());
  REQUIRE_THROWS_AS(pluss::note::bytes({std::string("bad\xFF\xFE") + "x"}).to_utf8_strict(), pluss::Error);

  // to_utf8_lossy: never raises; one U+FFFD per bad byte; valid unchanged.
  REQUIRE(pluss::note::bytes({std::string("a") + '\xFF' + "b"}).to_utf8_lossy().at(0) ==
          std::string("a\xEF\xBF\xBD") + "b");
  REQUIRE(pluss::note::bytes({"café"}).to_utf8_lossy().at(0) == "café");

  // from_latin1: every byte -> its code point; result is text.
  auto l1 = pluss::note::bytes({std::string("caf") + '\xE9'}).from_latin1();
  REQUIRE(l1.at(0) == "café");
  REQUIRE_FALSE(l1.is_bytes());

  // to_latin1_lossy: text -> Latin-1 bytes; out-of-range -> '?'; result is bytes; round-trips.
  auto tl1 = pluss::make_note("café €").to_latin1_lossy();
  REQUIRE(tl1.is_bytes());
  REQUIRE(tl1.at(0) == std::string("caf\xE9") + " ?");
  REQUIRE(pluss::make_note("café").to_latin1_lossy().from_latin1().at(0) == "café");

  // from_windows1252: 0x80-0x9F smart characters; unassigned slots -> '?'.
  REQUIRE(pluss::note::bytes({std::string("\x93") + "x" + "\x94"}).from_windows1252().at(0) == "“x”");
  REQUIRE(pluss::note::bytes({std::string("a\x81") + "b"}).from_windows1252().at(0) == "a?b");

  // to_windows1252_lossy: smart chars encode to 0x80-0x9F; unrepresentable -> '?'.
  auto tw = pluss::make_note(std::string("“q”")).to_windows1252_lossy();
  REQUIRE(tw.is_bytes());
  REQUIRE(tw.at(0) == std::string("\x93") + "q" + "\x94");
  REQUIRE(pluss::make_note("a中b").to_windows1252_lossy().at(0) == "a?b");
}

TEST_CASE("CodeN transliteration / phonetics family", "[coden][patch13]") {
  // transliterate_ascii: fold Latin-1 accents, '?' for anything else.
  REQUIRE(pluss::make_note("café señor Ærø").nlp().to_ascii().at(0) == "cafe senor AEro");
  REQUIRE(pluss::make_note("ĥ").nlp().to_ascii().at(0) == "??");        // Latin Extended-A
  REQUIRE(pluss::make_note("ā").to_nfd().nlp().to_ascii().at(0) == "a"); // NFD widens the fold
  REQUIRE(pluss::make_note("plain 1").nlp().to_ascii().at(0) == "plain 1");

  // transliterate_greek: accented vowels handled, combining marks dropped,
  // both cases -> lower Latin, non-Greek passes through.
  REQUIRE(pluss::make_note("Ελλάς").nlp().to_greek().at(0) == "ellas");
  REQUIRE(pluss::make_note("θΧΩ").nlp().to_greek().at(0) == "thcho");
  REQUIRE(pluss::make_note("x=Α").nlp().to_greek().at(0) == "x=a");
  REQUIRE(pluss::make_note("ἄνθρωπος").to_nfd().nlp().to_greek().at(0) == "anthropos");

  // transliterate_cyrillic: multi-letter sounds; hard/soft signs dropped.
  REQUIRE(pluss::make_note("Щи").nlp().to_cyrillic().at(0) == "shchi");
  REQUIRE(pluss::make_note("подъезд").nlp().to_cyrillic().at(0) == "podezd");
  REQUIRE(pluss::make_note("день").nlp().to_cyrillic().at(0) == "den");

  // phonetic_soundex: NARA rules incl. H/W transparency and first-letter dedup.
  REQUIRE(pluss::make_note({"Robert", "Rupert", "Rubin"}).nlp().soundex().at(0) == "R163");
  REQUIRE(pluss::make_note({"Robert", "Rupert", "Rubin"}).nlp().soundex().at(1) == "R163");
  REQUIRE(pluss::make_note("Ashcraft").nlp().soundex().at(0) == "A261");   // H bridges S and C
  REQUIRE(pluss::make_note("Pfister").nlp().soundex().at(0) == "P236");    // leading f repeats P's code
  REQUIRE(pluss::make_note("John Smith").nlp().soundex().at(0) == "J500 S530");
  REQUIRE(pluss::make_note("A").nlp().soundex().at(0) == "A000");
  REQUIRE(pluss::make_note("").nlp().soundex().at(0) == "");

  // Metaphone is the canonical consonant-skeleton operation.
  REQUIRE(pluss::make_note("programming").nlp().metaphone().at(0) == "PRGRMNG");
  REQUIRE(pluss::make_note("quick brown fox").nlp().metaphone().at(0) == "QCK BRN FX");

  // bytes-mode input is rejected by the whole family.
  REQUIRE_THROWS_AS(pluss::note::bytes({"x"}).nlp().to_greek(), pluss::Error);
  REQUIRE_THROWS_AS(pluss::note::bytes({"x"}).nlp().soundex(), pluss::Error);
}

TEST_CASE("CodeN shape / layout family", "[coden][patch13]") {
  const auto v6 = pluss::make_note({"a", "b", "c", "d", "e", "f"});
  const auto m23 = pluss::make_note(std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
                                    std::array<std::int64_t, 2>{2, 3});
  const auto s = pluss::make_note("solo");

  // ---- reshape ----
  {
    auto g = v6.reshape(std::vector<std::int64_t>{2, 3});
    REQUIRE(g.dim() == 2);
    REQUIRE(g.sizes() == std::vector<std::int64_t>{2, 3});
    REQUIRE(g.at(4) == "e");                       // row-major: cells unchanged
    REQUIRE(m23.reshape(std::vector<std::int64_t>{6}).dim() == 1);
    // one inferred (-1) dimension
    REQUIRE(v6.reshape(std::vector<std::int64_t>{-1, 3}).sizes() == std::vector<std::int64_t>{2, 3});
    REQUIRE(v6.reshape(std::vector<std::int64_t>{2, -1}).sizes() == std::vector<std::int64_t>{2, 3});
    REQUIRE(v6.reshape(std::vector<std::int64_t>{-1}).dim() == 1);
    // scalar round trip
    REQUIRE(s.reshape(std::vector<std::int64_t>{1}).reshape(std::vector<std::int64_t>{}).dim() == 0);
    REQUIRE_THROWS_AS(pluss::make_note({"a", "b", "c"}).reshape(std::vector<std::int64_t>{2, 2}), pluss::Error);
    REQUIRE_THROWS_AS(pluss::make_note({"a", "b", "c", "d"}).reshape(std::vector<std::int64_t>{-1, -1}), pluss::Error);
    REQUIRE_THROWS_AS(pluss::make_note({"a", "b", "c", "d", "e"}).reshape(std::vector<std::int64_t>{-1, 2}), pluss::Error);
    REQUIRE_THROWS_AS(pluss::make_note({"a", "b"}).reshape(std::vector<std::int64_t>{1, 1, 1, 2}), pluss::Error);
  }

  // ---- flatten ----
  REQUIRE(m23.flatten().dim() == 1);
  REQUIRE(m23.flatten().numel() == 6);
  REQUIRE(v6.flatten().dim() == 1);
  REQUIRE(s.flatten().dim() == 1);
  REQUIRE(s.flatten().numel() == 1);

  // ---- squeeze ----
  REQUIRE(pluss::make_note({{"x", "y", "z"}}).squeeze(0).dim() == 1);
  REQUIRE(v6.reshape(std::vector<std::int64_t>{6, 1}).squeeze(1).dim() == 1);
  REQUIRE(v6.reshape(std::vector<std::int64_t>{1, 6}).squeeze(-2).dim() == 1);
  REQUIRE(m23.squeeze(0).dim() == 2);          // not size-1: unchanged, no throw
  REQUIRE(s.squeeze(0).dim() == 0);            // scalar: unchanged

  // ---- unsqueeze / expand_dims ----
  REQUIRE(v6.unsqueeze(0).sizes() == std::vector<std::int64_t>{1, 6});
  REQUIRE(v6.unsqueeze(1).sizes() == std::vector<std::int64_t>{6, 1});
  REQUIRE(v6.unsqueeze(-1).sizes() == std::vector<std::int64_t>{6, 1});
  REQUIRE(s.unsqueeze(0).sizes() == std::vector<std::int64_t>{1});
  REQUIRE(v6.unsqueeze(0).sizes() == v6.unsqueeze(0).sizes());
  REQUIRE_THROWS_AS(v6.reshape(std::vector<std::int64_t>{1, 2, 3}).unsqueeze(0), pluss::Error);

  // ---- transpose ----
  {
    auto t = m23.transpose(0, 1);
    REQUIRE(t.sizes() == std::vector<std::int64_t>{3, 2});
    REQUIRE(t.at(1) == "d");                   // cell moved: (0,1) <- old (1,0)
    REQUIRE(m23.transpose(1, 1).sizes() == std::vector<std::int64_t>{2, 3});   // same-dim no-op
    REQUIRE(v6.transpose(0, 0).dim() == 1);                                    // 1-D no-op
    REQUIRE_THROWS_AS(pluss::make_note({"a", "b"}).transpose(0, 1), pluss::Error);
  }

  // ---- permute ----
  REQUIRE(m23.permute(std::vector<std::int64_t>{1, 0}).sizes() == std::vector<std::int64_t>{3, 2});
  REQUIRE(m23.permute(std::vector<std::int64_t>{0, 1}).sizes() == std::vector<std::int64_t>{2, 3});
  REQUIRE(v6.reshape(std::vector<std::int64_t>{1, 2, 3}).permute(std::vector<std::int64_t>{2, 0, 1}).sizes()
          == std::vector<std::int64_t>{3, 1, 2});
  REQUIRE_THROWS_AS(m23.permute(std::vector<std::int64_t>{0, 0}), pluss::Error);
  REQUIRE_THROWS_AS(m23.permute(std::vector<std::int64_t>{0}), pluss::Error);

  // ---- contiguous ----
  {
    auto tp = m23.transpose(0, 1);
    auto c = tp.contiguous();
    REQUIRE(c.sizes() == tp.sizes());
    REQUIRE(c.at(1) == tp.at(1));
  }
}

TEST_CASE("CodeN selection / slicing family: select / narrow / take", "[coden][patch13]") {
  const auto v6 = pluss::make_note({"a", "b", "c", "d", "e", "f"});
  const auto m23 = pluss::make_note(std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
                                    std::array<std::int64_t, 2>{2, 3});
  const auto t123 = v6.reshape(std::vector<std::int64_t>{1, 2, 3});

  // ---- select: drops the indexed axis ----
  REQUIRE(v6.select(0, 2).dim() == 0);
  REQUIRE(v6.select(0, 2).at(0) == "c");
  REQUIRE(m23.select(0, 1).sizes() == std::vector<std::int64_t>{3});   // row 1
  REQUIRE(m23.select(0, 1).at(0) == "d");
  REQUIRE(m23.select(1, 0).sizes() == std::vector<std::int64_t>{2});   // column 0
  REQUIRE(m23.select(1, 0).at(1) == "d");
  REQUIRE(m23.select(-1, 2).at(1) == "f");                             // negative dim
  REQUIRE(v6.select(0, -1).at(0) == "f");                             // negative index
  REQUIRE(t123.select(1, 0).sizes() == std::vector<std::int64_t>{1, 3});
  REQUIRE_THROWS_AS(pluss::make_note({"a", "b"}).select(0, 5), pluss::Error);
  REQUIRE_THROWS_AS(pluss::make_note("x").select(0, 0), pluss::Error);

  // ---- narrow: keeps a contiguous range, rank unchanged ----
  REQUIRE(v6.narrow(0, 1, 3).sizes() == std::vector<std::int64_t>{3});
  REQUIRE(v6.narrow(0, 1, 3).at(0) == "b");
  REQUIRE(m23.narrow(0, 1, 1).sizes() == std::vector<std::int64_t>{1, 3});
  REQUIRE(m23.narrow(1, 0, 2).sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(m23.narrow(-1, 1, 2).at(0) == "b");                          // negative dim
  REQUIRE(v6.narrow(0, 2, 0).numel() == 0);                            // zero length
  REQUIRE(v6.narrow(0, 0, 6).numel() == 6);                            // full range
  REQUIRE_THROWS_AS(pluss::make_note({"a", "b", "c"}).narrow(0, 2, 2), pluss::Error);
  REQUIRE_THROWS_AS(pluss::make_note({"a", "b", "c"}).narrow(0, -1, 1), pluss::Error);

  // ---- take: flat row-major gather, rank-1 result ----
  {
    auto g = v6.take(std::vector<std::int64_t>{0, 2, 4});
    REQUIRE(g.dim() == 1);
    REQUIRE(g.at(0) == "a");
    REQUIRE(g.at(2) == "e");
  }
  REQUIRE(v6.take(std::vector<std::int64_t>{-1, -1, 0}).at(0) == "f");  // negative + repeat
  REQUIRE(v6.take(std::vector<std::int64_t>{5, 4, 3, 2, 1, 0}).at(0) == "f");  // reverse
  REQUIRE(m23.take(std::vector<std::int64_t>{0, 3, 5}).at(1) == "d");  // 2-D flattened
  REQUIRE(t123.take(std::vector<std::int64_t>{1, 4}).at(1) == "e");    // 3-D flattened
  REQUIRE(pluss::make_note("solo").take(std::vector<std::int64_t>{0, 0}).numel() == 2);
  REQUIRE(v6.take(std::vector<std::int64_t>{}).numel() == 0);          // empty list
  REQUIRE_THROWS_AS(pluss::make_note({"a", "b"}).take(std::vector<std::int64_t>{5}), pluss::Error);
}
