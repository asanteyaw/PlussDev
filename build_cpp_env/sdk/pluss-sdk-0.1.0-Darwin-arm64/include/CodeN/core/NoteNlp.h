#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <string_view>

namespace pluss::coden {

class Note;
class TokenBatch;

// Namespace-like linguistic operations bound to one Note. The temporary
// accessor keeps no ownership and is intended for an immediate chained call.
class CODEN_API NoteNlpAccessor final {
public:
  NoteNlpAccessor() = default;
  explicit NoteNlpAccessor(const Note& owner) noexcept : owner_(&owner) {}

  Note clean() const;
  Note expand_contractions() const;
  Note normalize_numbers() const;
  Note normalize_punct(bool ascii_only = false) const;
  Note remove_stopwords(::std::string_view language = "en") const;

  Note language() const;
  Note script() const;
  Note sentiment() const;
  Note toxicity() const;
  Note readability_grade() const;
  Note readability_level() const;
  Note spam_score() const;
  Note formality() const;
  Note subjectivity() const;

  Note mask_currency() const;
  Note mask_dates() const;
  Note mask_emails() const;
  Note mask_hashtags() const;
  Note mask_mentions() const;
  Note mask_numbers() const;
  Note mask_pii(::std::int64_t flags = 0) const;
  Note mask_times() const;
  Note mask_urls() const;

  Note remove_boilerplate() const;
  Note remove_brackets() const;
  Note remove_bullets() const;
  Note remove_code() const;
  Note remove_emails() const;
  Note remove_footers() const;
  Note remove_hashtags() const;
  Note remove_headers() const;
  Note remove_html() const;
  Note remove_latex() const;
  Note remove_markdown() const;
  Note remove_math() const;
  Note remove_mentions() const;
  Note remove_parentheticals() const;
  Note remove_quotes() const;
  Note remove_tables() const;
  Note remove_urls() const;

  TokenBatch tokens(::std::int64_t flags = 0) const;
  TokenBatch sentences() const;
  TokenBatch punct_tokens() const;
  TokenBatch stems() const;
  TokenBatch lemmas() const;
  TokenBatch pos_tags() const;
  TokenBatch named_entities() const;
  TokenBatch keywords(::std::int64_t count) const;
  TokenBatch keyphrases(::std::int64_t count) const;
  TokenBatch char_ngrams(::std::int64_t width) const;
  TokenBatch word_ngrams(::std::int64_t width) const;
  TokenBatch hashtags() const;
  TokenBatch mentions() const;
  TokenBatch urls() const;
  TokenBatch emails() const;
  TokenBatch numbers() const;
  TokenBatch acronyms() const;
  TokenBatch definitions() const;
  TokenBatch terms() const;
  TokenBatch citations() const;
  TokenBatch math_symbols() const;

  TokenBatch lower_tokens() const;
  TokenBatch without_stopwords() const;
  TokenBatch without_numbers() const;
  TokenBatch alpha_tokens() const;
  TokenBatch alnum_tokens() const;
  TokenBatch min_length(::std::int64_t length) const;
  TokenBatch max_length(::std::int64_t length) const;
  TokenBatch match_tokens(::std::string_view pattern) const;
  TokenBatch shuffle_tokens() const;
  TokenBatch unique_tokens() const;
  TokenBatch sort_tokens() const;

  Note to_ascii() const;
  Note to_greek() const;
  Note to_cyrillic() const;
  Note soundex() const;
  Note metaphone() const;

  TokenBatch intersect_acronyms(const Note& other) const;
  TokenBatch intersect_citations(const Note& other) const;
  TokenBatch intersect_symbols(const Note& other) const;
  TokenBatch intersect_tokens(const Note& other) const;
  TokenBatch intersect_terms(const Note& other) const;
  Note jaccard_acronyms(const Note& other) const;
  Note jaccard_citations(const Note& other) const;
  Note jaccard_symbols(const Note& other) const;
  Note jaccard_tokens(const Note& other) const;
  Note jaccard_terms(const Note& other) const;
  Note overlap_acronyms(const Note& other) const;
  Note overlap_citations(const Note& other) const;
  Note overlap_symbols(const Note& other) const;
  Note overlap_tokens(const Note& other) const;
  Note overlap_terms(const Note& other) const;
  TokenBatch diff_words(const Note& other) const;
  TokenBatch align_tokens(const Note& other) const;
  TokenBatch align_sentences(const Note& other) const;

private:
  const Note& owner() const;
  const Note* owner_{nullptr};
};

} // namespace pluss::coden
