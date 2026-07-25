#include <pluss/nlp.h>

#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_vocab_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {stage::tokenize(), stage::lemmatize()},
  };

  const auto doc = nlp.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");
  if (!doc.lexicon) {
    if (err) *err << "nodeb lexicon should exist on inferred docs\n";
    return false;
  }
  if (doc.lexicon->size() == 0 || doc.lexicon->texts().size() == 0 || doc.lexicon->morphs().size() == 0) {
    if (err) *err << "nodeb lexicon/text pool/morph bank should populate during tokenization\n";
    return false;
  }

  const auto* laurel = doc.lexicon->find("Laurel");
  const auto* nasa = doc.lexicon->find("NASA");
  const auto* email = doc.lexicon->find("help@pluss.dev");
  const auto* numeric = doc.lexicon->find("42");
  if (!laurel || !nasa || !email || !numeric) {
    if (err) *err << "nodeb lexicon should register observed orth forms\n";
    return false;
  }

  if (!laurel->morphology.is_titlecase ||
      laurel->morphology.kind != TokenKind::Word ||
      laurel->morphology.casing != CaseKind::Title ||
      laurel->morphology.shape != "Xxxxxx" ||
      laurel->prefix != "Lau" ||
      laurel->suffix != "rel" ||
      laurel->norm != "laurel") {
    if (err) *err << "nodeb lexicon should carry titlecase lexical morphology and cached forms\n";
    return false;
  }
  if (!nasa->morphology.is_uppercase || nasa->morphology.casing != CaseKind::Upper || !nasa->morphology.has_alpha) {
    if (err) *err << "nodeb lexicon should carry uppercase lexical morphology\n";
    return false;
  }
  if (!email->morphology.is_email || email->morphology.kind != TokenKind::Email || !email->morphology.has_alpha) {
    if (err) *err << "nodeb lexicon should detect email lexemes\n";
    return false;
  }
  if (!numeric->morphology.is_number_like || numeric->morphology.kind != TokenKind::Number || !numeric->morphology.has_digit) {
    if (err) *err << "nodeb lexicon should detect numeric-like lexemes\n";
    return false;
  }

  std::int64_t laurel_count = 0;
  StringId first_laurel_id = 0;
  StringId first_morphology_id = 0;
  for (const auto& token : doc.tokens) {
    if (token.orth == "Laurel") {
      ++laurel_count;
      if (first_laurel_id == 0) {
        first_laurel_id = token.orth_id;
        first_morphology_id = token.morphology_id;
      } else if (token.orth_id != first_laurel_id) {
        if (err) *err << "nodeb repeated tokens should reuse the same text id\n";
        return false;
      } else if (token.morphology_id != first_morphology_id) {
        if (err) *err << "nodeb repeated tokens should reuse the same morphology id\n";
        return false;
      }
    }
    if (!token.lemma.empty() && token.lemma_id == 0) {
      if (err) *err << "nodeb lemmatizer should back lemmas with string ids\n";
      return false;
    }
    if (token.norm_id == 0 || token.prefix_id == 0 || token.suffix_id == 0 || token.morphology_id == 0) {
      if (err) *err << "nodeb token stream should carry cached lexical ids beyond orth/lower/lemma\n";
      return false;
    }
  }
  if (laurel_count != 2 || first_laurel_id == 0) {
    if (err) *err << "nodeb test fixture should have two Laurel tokens with stable ids\n";
    return false;
  }

  if (doc.lexicon->lookup("Laurel") != first_laurel_id) {
    if (err) *err << "nodeb text pool lookup should return deterministic ids\n";
    return false;
  }
  if (doc.lexicon->text(first_laurel_id) != "Laurel") {
    if (err) *err << "nodeb text pool should map ids back to original text\n";
    return false;
  }
  const auto* stored_morphology = doc.lexicon->find_morphology(first_morphology_id);
  if (!stored_morphology || stored_morphology->spec != laurel->morphology.spec) {
    if (err) *err << "nodeb morph bank should map morphology ids back to normalized feature specs\n";
    return false;
  }

  const auto write_doc = nlp.infer("writes");
  if (write_doc.tokens.size() != 1 || write_doc.tokens[0].lemma != "write") {
    if (err) *err << "nodeb lemma fixture should normalize writes -> write\n";
    return false;
  }
  if (write_doc.lexicon->text(write_doc.tokens[0].lemma_id) != "write") {
    if (err) *err << "nodeb lemma ids should resolve through the text pool\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb vocab tranche", "[nodeb][vocab]") {
  if (!run_nodeb_vocab_checks(&std::cerr)) {
    FAIL("nodeb vocab checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_vocab_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb vocab checks passed\n";
  return 0;
}
#endif
