#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <algorithm>
#include <array>
#include <type_traits>

namespace {

static_assert(std::is_same_v<pluss::Note::NlpAccessor,
                             decltype(std::declval<const pluss::Note&>().nlp())>);
static_assert(std::is_same_v<pluss::Note::RegexAccessor,
                             decltype(std::declval<const pluss::Note&>().regex())>);
static_assert(std::is_same_v<pluss::Note::TexAccessor,
                             decltype(std::declval<const pluss::Note&>().tex())>);

template <class T>
concept HasRootRemoveEmails = requires(const T& value) {
  value.nlp_remove_emails();
};

template <class T>
concept HasRootTokenizeWords = requires(const T& value) {
  value.tokenize_words();
};

template <class T>
concept HasWordAlias = requires(const T& value) {
  value.nlp().words();
};

template <class T>
concept HasBigramAlias = requires(const T& value) {
  value.nlp().bigrams();
};

template <class T>
concept HasLatinAlias = requires(const T& value) {
  value.nlp().to_latin();
};

template <class T>
concept HasDoubleMetaphoneAlias = requires(const T& value) {
  value.nlp().double_metaphone();
};

template <class T>
concept HasRootRegexFindall = requires(const T& value) {
  value.regex_findall("x");
};

template <class T>
concept HasRootRegexCount = requires(const T& value) {
  value.count_regex("x");
};

template <class T>
concept HasRootFindAllRegex = requires(const T& value) {
  value.find_all_regex("x");
};

template <class T>
concept HasRegexTokenAlias = requires(const T& value) {
  value.nlp().regex_tokens("x");
};

template <class T>
concept HasLegacyRootTextAliases =
    requires(const T& value) { value.trim(); } ||
    requires(const T& value) { value.trim_left(); } ||
    requires(const T& value) { value.trim_right(); } ||
    requires(const T& value) { value.expand_dims(0); } ||
    requires(const T& value) { value.normalize_spaces(); } ||
    requires(const T& value) { value.remove_non_ascii(); } ||
    requires(const T& value) { value.strip_diacritics(); } ||
    requires(const T& value) { value.normalize(0); } ||
    requires(const T& value) { value.latex_to_text(); } ||
    requires(const T& value) { value.split_graphemes(); } ||
    requires(const T& value) { value.phonetic_double_metaphone(); };

template <class T>
concept HasMisleadingNlpAliases =
    requires(const T& value) { value.nlp().bibliography(); } ||
    requires(const T& value) { value.nlp().noun_phrases(); } ||
    requires(const T& value) { value.nlp().without_punct(); } ||
    requires(const T& value) { value.nlp().join_tokens(" "); };

template <class T>
concept HasIgnoredNlpOptions =
    requires(const T& value) { value.nlp().sentences(1); } ||
    requires(const T& value) { value.nlp().pos_tags("custom"); } ||
    requires(const T& value) { value.nlp().named_entities("custom"); };

template <class T>
concept HasMisleadingTexAliases =
    requires(const T& value) { value.tex_equations(1); } ||
    requires(const T& value) { value.tex_strip_commands(); } ||
    requires(const T& value) { value.tex_strip_environments(); } ||
    requires(const T& value) { value.tex_resolve_includes(); } ||
    requires(const T& value) { value.tex_strip_math(); } ||
    requires(const T& value) { value.tex_strip_text(); };

template <class T>
concept HasLongTexNames =
    requires(const T& value) { value.tex().environment_names(); } ||
    requires(const T& value) { value.tex().environments(); } ||
    requires(const T& value) { value.tex().environments(false); } ||
    requires(const T& value) { value.tex().expand_inputs(); } ||
    requires(const T& value) { value.tex().expand_macros(); } ||
    requires(const T& value) { value.tex().inline_macros(); } ||
    requires(const T& value) { value.tex().standardize_dots(); } ||
    requires(const T& value) { value.tex().standardize_integrals(); } ||
    requires(const T& value) { value.tex().standardize_limits(); } ||
    requires(const T& value) { value.tex().standardize_matrices(); } ||
    requires(const T& value) { value.tex().standardize_primes(); } ||
    requires(const T& value) { value.tex().standardize_subscripts(); } ||
    requires(const T& value) { value.tex().standardize_summations(); } ||
    requires(const T& value) { value.tex().standardize_superscripts(); } ||
    requires(const T& value) { value.tex().standardize_vectors(); } ||
    requires(const T& value) { value.tex().extract_command_args("x"); } ||
    requires(const T& value) { value.tex().extract_environment("x"); } ||
    requires(const T& value) { value.tex().extract_functions(); } ||
    requires(const T& value) { value.tex().extract_greek(); } ||
    requires(const T& value) { value.tex().extract_identifiers(); } ||
    requires(const T& value) { value.tex().extract_numbers(); } ||
    requires(const T& value) { value.tex().extract_operators(); } ||
    requires(const T& value) { value.tex().extract_symbols(); };

} // namespace

static_assert(sizeof(pluss::Note) == sizeof(void*));
static_assert(!HasRootRemoveEmails<pluss::Note>);
static_assert(!HasRootTokenizeWords<pluss::Note>);
static_assert(!HasWordAlias<pluss::Note>);
static_assert(!HasBigramAlias<pluss::Note>);
static_assert(!HasLatinAlias<pluss::Note>);
static_assert(!HasDoubleMetaphoneAlias<pluss::Note>);
static_assert(!HasRootRegexFindall<pluss::Note>);
static_assert(!HasRootRegexCount<pluss::Note>);
static_assert(!HasRootFindAllRegex<pluss::Note>);
static_assert(!HasRegexTokenAlias<pluss::Note>);
static_assert(!HasLegacyRootTextAliases<pluss::Note>);
static_assert(!HasMisleadingNlpAliases<pluss::Note>);
static_assert(!HasIgnoredNlpOptions<pluss::Note>);
static_assert(!HasMisleadingTexAliases<pluss::Note>);
static_assert(!HasLongTexNames<pluss::Note>);

TEST_CASE("Note NLP accessor is the sole public linguistic gateway", "[coden][nlp_accessor]") {
  const auto note = pluss::make_note({"Email a@b.io", "Two words"});

  REQUIRE(note.nlp().remove_emails().at(0) == "Email");
  REQUIRE(note.nlp().tokens().tokens().at(0) == "Email");
  REQUIRE(note.nlp().tokens().offsets().integers()[2] == 4);
}

TEST_CASE("NLP token transforms never cross cell boundaries", "[coden][nlp_accessor]") {
  const auto note = pluss::make_note({"z a z", "b a b"});

  const auto sorted = note.nlp().sort_tokens();
  REQUIRE(sorted.tokens().vector() ==
          std::vector<std::string_view>{"a", "z", "z", "a", "b", "b"});
  REQUIRE(std::ranges::equal(sorted.offsets().integers(),
                             std::array<std::int64_t, 3>{0, 3, 6}));

  const auto unique = note.nlp().unique_tokens();
  REQUIRE(unique.tokens().vector() ==
          std::vector<std::string_view>{"z", "a", "b", "a"});
  REQUIRE(std::ranges::equal(unique.offsets().integers(),
                             std::array<std::int64_t, 3>{0, 2, 4}));

  const auto chars = pluss::make_note({"cafe", "go"}).nlp().char_ngrams(3);
  REQUIRE(chars.tokens().vector() ==
          std::vector<std::string_view>{"caf", "afe"});
  REQUIRE(std::ranges::equal(chars.offsets().integers(),
                             std::array<std::int64_t, 3>{0, 2, 2}));
}

TEST_CASE("NLP option arguments reject values without a defined meaning",
          "[coden][nlp_accessor]") {
  const auto note = pluss::make_note("one two three");
  REQUIRE_THROWS_AS(note.nlp().keywords(0), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().keyphrases(-1), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().char_ngrams(0), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().word_ngrams(-2), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().min_length(-1), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().max_length(-1), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().match_tokens(""), pluss::Error);
  REQUIRE_THROWS_AS(note.nlp().remove_stopwords("fr"), pluss::Error);
}

TEST_CASE("TokenBatch distinguishes handle state from ragged payload state",
          "[coden][token_batch]") {
  const pluss::TokenBatch absent;
  REQUIRE_FALSE(absent.defined());
  REQUIRE(absent.empty());
  REQUIRE_THROWS_AS(absent.tokens(), pluss::Error);
  REQUIRE_THROWS_AS(absent.offsets(), pluss::Error);

  const auto populated = pluss::note::make({"one two", "", "three"}).nlp().tokens();
  REQUIRE(populated.defined());
  REQUIRE_FALSE(populated.empty());
  REQUIRE(populated.tokens().is_text());
  REQUIRE(populated.tokens().sizes() == std::vector<std::int64_t>{3});
  REQUIRE(populated.offsets().is_int64());
  const std::array<std::int64_t, 4> populated_offsets{0, 2, 2, 3};
  REQUIRE(std::ranges::equal(populated.offsets().integers(), populated_offsets));

  const auto no_tokens = pluss::note::make({"", "!!!"}).nlp().tokens();
  REQUIRE(no_tokens.defined());
  REQUIRE(no_tokens.empty());
  REQUIRE(no_tokens.tokens().defined());
  REQUIRE(no_tokens.tokens().numel() == 0);
  const std::array<std::int64_t, 3> empty_offsets{0, 0, 0};
  REQUIRE(std::ranges::equal(no_tokens.offsets().integers(), empty_offsets));

  const auto propagated = pluss::Note::undefined().nlp().tokens();
  REQUIRE(propagated.defined());
  REQUIRE(propagated.empty());
  REQUIRE_FALSE(propagated.tokens().defined());
  const std::array<std::int64_t, 1> absent_offsets{0};
  REQUIRE(std::ranges::equal(propagated.offsets().integers(), absent_offsets));
}

TEST_CASE("Note regex accessor owns the canonical pattern operations", "[coden][regex_accessor]") {
  const auto note = pluss::make_note({"A1,b22", "none"});

  REQUIRE(note.regex().match(R"([A-Z]\d,\w+)").bools()[0] == 1);
  REQUIRE(note.regex().search(R"(\d+)").bools()[1] == 0);
  REQUIRE(note.regex().replace(R"(\d+)", "#").at(0) == "A#,b#");
  REQUIRE(note.regex().count(R"(\d+)").integers()[0] == 2);
  REQUIRE(note.regex().findall(R"(\d+)").tokens().at(1) == "22");
  REQUIRE(note.regex().split("[,]").offsets().integers()[1] == 2);
}
