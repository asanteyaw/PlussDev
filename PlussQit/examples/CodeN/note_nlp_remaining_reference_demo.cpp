#include <pluss/coden/note_factory.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void show_note(std::string_view name, const pluss::Note& value) {
  std::cout << "-- " << name << " --\n" << pluss::note::print(value) << '\n';
}

void show_batch(std::string_view name, const pluss::TokenBatch& batch) {
  std::cout << "-- " << name << " --\n";
  std::cout << "tokens=" << pluss::note::print(batch.tokens()) << '\n';
  std::cout << "offsets=[";
  const auto offsets = batch.offsets().integers();
  for (std::size_t i = 0; i < offsets.size(); ++i)
    std::cout << (i == 0 ? "" : ", ") << offsets[i];
  std::cout << "]\n";
}

template <typename Fn>
void rejected(std::string_view name, Fn&& fn) {
  try {
    fn();
    std::cout << name << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << name << ": rejected\n";
  }
}

// Every NLP accessor method shares the same Text-only dtype guard and the
// same undefined-propagates-undefined rule (docs/sphinx/api/note-nlp/*.md's
// "Absence And Dtype Validation" examples), but each method's own page shows
// its own method being exercised rather than a generic stand-in.
template <typename Fn>
void dtype_contract(std::string_view name, Fn&& fn) {
  std::cout << name << " undefined-defined=" << std::boolalpha
            << fn(pluss::Note::undefined()).defined() << " Bytes=";
  try {
    (void)fn(pluss::note::bytes(std::vector<std::string>{"text"},
                                 std::array<std::int64_t, 1>{1}));
    std::cout << "accepted\n";
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}

// The TokenBatch-returning extraction/ranking/sequence-transform methods
// share the same undefined-structural-batch and Bool-rejection contract
// (docs/sphinx/api/note-nlp/*.md's "Absence And Dtype Validation" examples);
// each method's own page shows its own method, not a generic stand-in.
template <typename Fn>
void dtype_contract_tb(std::string_view name, Fn&& fn) {
  const auto batch = fn(pluss::Note::undefined());
  std::cout << name << " tokens-defined=" << std::boolalpha
            << batch.tokens().defined() << " offset0="
            << batch.offsets().integers()[0] << " Bool=";
  try {
    (void)fn(pluss::note::make({true}));
    std::cout << "accepted\n";
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}

} // namespace

int main() {
  std::cout << std::setprecision(17);

  std::cout << "=== analysis ===\n";
  const auto analysis = pluss::note::make({
      "I love this excellent release!",
      "FREE money now http://spam.test unsubscribe",
      "This ordinary sentence describes a system.",
      "Привет мир"});
  show_note("language", analysis.nlp().language());
  show_note("script", analysis.nlp().script());
  show_note("sentiment", analysis.nlp().sentiment());
  show_note("toxicity", pluss::note::make({"kind and helpful", "idiot hate stupid"}).nlp().toxicity());
  show_note("readability_grade", pluss::note::make({"Cats run.", "Extraordinary terminology complicates comprehension considerably."}).nlp().readability_grade());
  show_note("readability_level", pluss::note::make({"Cats run.", "Extraordinary terminology complicates comprehension considerably."}).nlp().readability_level());
  show_note("spam_score", analysis.nlp().spam_score());
  show_note("formality", pluss::note::make({"yo gonna chill", "The committee requests comprehensive documentation."}).nlp().formality());
  show_note("subjectivity", pluss::note::make({"The vessel weighs 20 tonnes.", "I believe this wonderful idea is awful."}).nlp().subjectivity());

  std::cout << "\n=== sentence and lexical analysis ===\n";
  const auto lexical = pluss::note::make({
      "Alice visited New York. Bob is running quickly!",
      "The systems were tested and studies improved."});
  show_batch("sentences", lexical.nlp().sentences());
  show_batch("punct_tokens", lexical.nlp().punct_tokens());
  show_batch("stems", lexical.nlp().stems());
  show_batch("lemmas", lexical.nlp().lemmas());
  show_batch("pos_tags", lexical.nlp().pos_tags());
  show_batch("named_entities", lexical.nlp().named_entities());

  std::cout << "\n=== ranking and ngrams ===\n";
  const auto corpus = pluss::note::make({
      "red blue red green red blue",
      "machine learning improves machine learning systems"});
  show_batch("keywords(2)", corpus.nlp().keywords(2));
  show_batch("keyphrases(2)", corpus.nlp().keyphrases(2));
  show_batch("char_ngrams(3)", pluss::note::make({"cafe", "go"}).nlp().char_ngrams(3));
  show_batch("word_ngrams(2)", corpus.nlp().word_ngrams(2));

  std::cout << "\n=== named extraction ===\n";
  const auto spans = pluss::note::make({
      "NASA met @ada about #AI at ada@example.com; see https://example.com. Cost 12.50 [7]. x = \\alpha + y.",
      "API means application interface. Robust parser terms remain."});
  show_batch("acronyms", spans.nlp().acronyms());
  show_batch("citations", spans.nlp().citations());
  show_batch("definitions", spans.nlp().definitions());
  show_batch("emails", spans.nlp().emails());
  show_batch("hashtags", spans.nlp().hashtags());
  show_batch("math_symbols", spans.nlp().math_symbols());
  show_batch("mentions", spans.nlp().mentions());
  show_batch("numbers", spans.nlp().numbers());
  show_batch("terms", spans.nlp().terms());
  show_batch("urls", spans.nlp().urls());

  std::cout << "\n=== token sequence transforms ===\n";
  const auto sequence = pluss::note::make({
      "The Fox fox 42 jumps quickly",
      "beta Alpha beta 7"});
  show_batch("alpha_tokens", sequence.nlp().alpha_tokens());
  show_batch("alnum_tokens", sequence.nlp().alnum_tokens());
  show_batch("lower_tokens", sequence.nlp().lower_tokens());
  show_batch("match_tokens('^[A-Z]')", sequence.nlp().match_tokens("^[A-Z]"));
  show_batch("max_length(4)", sequence.nlp().max_length(4));
  show_batch("min_length(5)", sequence.nlp().min_length(5));
  show_batch("shuffle_tokens", sequence.nlp().shuffle_tokens());
  show_batch("sort_tokens", sequence.nlp().sort_tokens());
  show_batch("unique_tokens", sequence.nlp().unique_tokens());
  show_batch("without_numbers", sequence.nlp().without_numbers());
  show_batch("without_stopwords", sequence.nlp().without_stopwords());

  std::cout << "\n=== removal ===\n";
  const auto markup = pluss::note::make({
      "Keep **bold** [label](https://x.test) and $x+y$; mention @ada.",
      "Quote \"remove me\" (draft) then keep.\\section{Hidden} Visible"});
  show_note("remove_latex", markup.nlp().remove_latex());
  show_note("remove_markdown", markup.nlp().remove_markdown());
  show_note("remove_math", markup.nlp().remove_math());
  show_note("remove_mentions", markup.nlp().remove_mentions());
  show_note("remove_parentheticals", markup.nlp().remove_parentheticals());
  show_note("remove_quotes", markup.nlp().remove_quotes());
  show_note("remove_stopwords", pluss::note::make({"This is the report from Ada", "A note with detail"}).nlp().remove_stopwords());
  show_note("remove_tables", pluss::note::make({"before\n| A | B |\n|---|---|\n| 1 | 2 |\nafter"}).nlp().remove_tables());

  std::cout << "\n=== shared contracts ===\n";
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"NASA [1]", "", "Alice runs.", "red red blue"},
      std::array<std::int64_t, 2>{2, 2});
  show_note("language matrix", matrix.nlp().language());
  show_batch("acronyms matrix", matrix.nlp().acronyms());
  show_note("empty score", pluss::note::make(std::vector<std::string>{}).nlp().sentiment());
  std::cout << "undefined language defined=" << std::boolalpha
            << pluss::Note::undefined().nlp().language().defined() << '\n';
  const auto undefined_terms = pluss::Note::undefined().nlp().terms();
  std::cout << "undefined terms: batch-defined=" << undefined_terms.defined()
            << " tokens-defined=" << undefined_terms.tokens().defined()
            << " offsets=[" << undefined_terms.offsets().integers()[0] << "]\n";
  rejected("Bytes analysis", [&] {
    (void)pluss::note::bytes(std::vector<std::string>{"text"},
                            std::array<std::int64_t, 1>{1}).nlp().language();
  });
  rejected("Bool extraction", [&] {
    (void)pluss::note::make({true}).nlp().terms();
  });
  rejected("keywords zero", [&] { (void)corpus.nlp().keywords(0); });
  rejected("keyphrases zero", [&] { (void)corpus.nlp().keyphrases(0); });
  rejected("char_ngrams zero", [&] { (void)corpus.nlp().char_ngrams(0); });
  rejected("word_ngrams zero", [&] { (void)corpus.nlp().word_ngrams(0); });
  rejected("min_length negative", [&] { (void)sequence.nlp().min_length(-1); });
  rejected("max_length negative", [&] { (void)sequence.nlp().max_length(-1); });
  rejected("empty match pattern", [&] { (void)sequence.nlp().match_tokens(""); });
  rejected("unsupported stopword language", [&] {
    (void)sequence.nlp().remove_stopwords("fr");
  });

  std::cout << "\n=== per-method dtype contracts ===\n";
  dtype_contract("script", [](const pluss::Note& n) { return n.nlp().script(); });
  dtype_contract("sentiment", [](const pluss::Note& n) { return n.nlp().sentiment(); });
  dtype_contract("toxicity", [](const pluss::Note& n) { return n.nlp().toxicity(); });
  dtype_contract("readability_grade", [](const pluss::Note& n) { return n.nlp().readability_grade(); });
  dtype_contract("readability_level", [](const pluss::Note& n) { return n.nlp().readability_level(); });
  dtype_contract("spam_score", [](const pluss::Note& n) { return n.nlp().spam_score(); });
  dtype_contract("formality", [](const pluss::Note& n) { return n.nlp().formality(); });
  dtype_contract("subjectivity", [](const pluss::Note& n) { return n.nlp().subjectivity(); });
  dtype_contract("remove_latex", [](const pluss::Note& n) { return n.nlp().remove_latex(); });
  dtype_contract("remove_markdown", [](const pluss::Note& n) { return n.nlp().remove_markdown(); });
  dtype_contract("remove_math", [](const pluss::Note& n) { return n.nlp().remove_math(); });
  dtype_contract("remove_mentions", [](const pluss::Note& n) { return n.nlp().remove_mentions(); });
  dtype_contract("remove_parentheticals", [](const pluss::Note& n) { return n.nlp().remove_parentheticals(); });
  dtype_contract("remove_quotes", [](const pluss::Note& n) { return n.nlp().remove_quotes(); });
  dtype_contract("remove_stopwords", [](const pluss::Note& n) { return n.nlp().remove_stopwords(); });
  dtype_contract("remove_tables", [](const pluss::Note& n) { return n.nlp().remove_tables(); });

  dtype_contract_tb("acronyms", [](const pluss::Note& n) { return n.nlp().acronyms(); });
  dtype_contract_tb("alnum_tokens", [](const pluss::Note& n) { return n.nlp().alnum_tokens(); });
  dtype_contract_tb("alpha_tokens", [](const pluss::Note& n) { return n.nlp().alpha_tokens(); });
  dtype_contract_tb("char_ngrams", [](const pluss::Note& n) { return n.nlp().char_ngrams(3); });
  dtype_contract_tb("citations", [](const pluss::Note& n) { return n.nlp().citations(); });
  dtype_contract_tb("definitions", [](const pluss::Note& n) { return n.nlp().definitions(); });
  dtype_contract_tb("emails", [](const pluss::Note& n) { return n.nlp().emails(); });
  dtype_contract_tb("hashtags", [](const pluss::Note& n) { return n.nlp().hashtags(); });
  dtype_contract_tb("keyphrases", [](const pluss::Note& n) { return n.nlp().keyphrases(2); });
  dtype_contract_tb("keywords", [](const pluss::Note& n) { return n.nlp().keywords(2); });
  dtype_contract_tb("lemmas", [](const pluss::Note& n) { return n.nlp().lemmas(); });
  dtype_contract_tb("lower_tokens", [](const pluss::Note& n) { return n.nlp().lower_tokens(); });
  dtype_contract_tb("match_tokens", [](const pluss::Note& n) { return n.nlp().match_tokens("^[A-Z]"); });
  dtype_contract_tb("math_symbols", [](const pluss::Note& n) { return n.nlp().math_symbols(); });
  dtype_contract_tb("max_length", [](const pluss::Note& n) { return n.nlp().max_length(4); });
  dtype_contract_tb("mentions", [](const pluss::Note& n) { return n.nlp().mentions(); });
  dtype_contract_tb("min_length", [](const pluss::Note& n) { return n.nlp().min_length(5); });
  dtype_contract_tb("named_entities", [](const pluss::Note& n) { return n.nlp().named_entities(); });
  dtype_contract_tb("numbers", [](const pluss::Note& n) { return n.nlp().numbers(); });
  dtype_contract_tb("pos_tags", [](const pluss::Note& n) { return n.nlp().pos_tags(); });
  dtype_contract_tb("punct_tokens", [](const pluss::Note& n) { return n.nlp().punct_tokens(); });
  dtype_contract_tb("sentences", [](const pluss::Note& n) { return n.nlp().sentences(); });
  dtype_contract_tb("shuffle_tokens", [](const pluss::Note& n) { return n.nlp().shuffle_tokens(); });
  dtype_contract_tb("sort_tokens", [](const pluss::Note& n) { return n.nlp().sort_tokens(); });
  dtype_contract_tb("stems", [](const pluss::Note& n) { return n.nlp().stems(); });
  dtype_contract_tb("unique_tokens", [](const pluss::Note& n) { return n.nlp().unique_tokens(); });
  dtype_contract_tb("urls", [](const pluss::Note& n) { return n.nlp().urls(); });
  dtype_contract_tb("without_numbers", [](const pluss::Note& n) { return n.nlp().without_numbers(); });
  dtype_contract_tb("without_stopwords", [](const pluss::Note& n) { return n.nlp().without_stopwords(); });
  dtype_contract_tb("word_ngrams", [](const pluss::Note& n) { return n.nlp().word_ngrams(2); });
}
