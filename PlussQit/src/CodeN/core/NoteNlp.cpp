#include "CodeN/core/NoteNlp.h"

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatch.h"

#include <cctype>
#include <string>

namespace pluss::coden {

const Note& NoteNlpAccessor::owner() const {
  PLUSS_CHECK(owner_ != nullptr, "Note.nlp: unbound accessor");
  return *owner_;
}

#define NOTE_NLP_NOTE0(public_name, dispatch_name) \
  Note NoteNlpAccessor::public_name() const { \
    return _compat::invoke<Note>(dispatch_name, owner()); \
  }
#define NOTE_NLP_BATCH0(public_name, dispatch_name) \
  TokenBatch NoteNlpAccessor::public_name() const { \
    return _compat::invoke<TokenBatch>(dispatch_name, owner()); \
  }
#define NOTE_NLP_OTHER(ret, public_name, dispatch_name) \
  ret NoteNlpAccessor::public_name(const Note& other) const { \
    return _compat::invoke<ret>(dispatch_name, owner(), {other}); \
  }

NOTE_NLP_NOTE0(clean, "nlp_clean")
NOTE_NLP_NOTE0(expand_contractions, "nlp_expand_contractions")
NOTE_NLP_NOTE0(normalize_numbers, "nlp_normalize_numbers")
NOTE_NLP_NOTE0(language, "nlp_language")
NOTE_NLP_NOTE0(script, "nlp_script")
NOTE_NLP_NOTE0(sentiment, "nlp_sentiment")
NOTE_NLP_NOTE0(toxicity, "nlp_toxicity")
NOTE_NLP_NOTE0(readability_grade, "nlp_readability_grade")
NOTE_NLP_NOTE0(readability_level, "nlp_readability_level")
NOTE_NLP_NOTE0(spam_score, "nlp_spam_score")
NOTE_NLP_NOTE0(formality, "nlp_formality")
NOTE_NLP_NOTE0(subjectivity, "nlp_subjectivity")

NOTE_NLP_NOTE0(mask_currency, "nlp_mask_currency")
NOTE_NLP_NOTE0(mask_dates, "nlp_mask_dates")
NOTE_NLP_NOTE0(mask_emails, "nlp_mask_emails")
NOTE_NLP_NOTE0(mask_hashtags, "nlp_mask_hashtags")
NOTE_NLP_NOTE0(mask_mentions, "nlp_mask_mentions")
NOTE_NLP_NOTE0(mask_numbers, "nlp_mask_numbers")
NOTE_NLP_NOTE0(mask_times, "nlp_mask_times")
NOTE_NLP_NOTE0(mask_urls, "nlp_mask_urls")

NOTE_NLP_NOTE0(remove_boilerplate, "nlp_remove_boilerplate")
NOTE_NLP_NOTE0(remove_brackets, "nlp_remove_brackets")
NOTE_NLP_NOTE0(remove_bullets, "nlp_remove_bullets")
NOTE_NLP_NOTE0(remove_code, "nlp_remove_code")
NOTE_NLP_NOTE0(remove_emails, "nlp_remove_emails")
NOTE_NLP_NOTE0(remove_footers, "nlp_remove_footers")
NOTE_NLP_NOTE0(remove_hashtags, "nlp_remove_hashtags")
NOTE_NLP_NOTE0(remove_headers, "nlp_remove_headers")
NOTE_NLP_NOTE0(remove_html, "nlp_remove_html")
NOTE_NLP_NOTE0(remove_latex, "nlp_remove_latex")
NOTE_NLP_NOTE0(remove_markdown, "nlp_remove_markdown")
NOTE_NLP_NOTE0(remove_math, "nlp_remove_math")
NOTE_NLP_NOTE0(remove_mentions, "nlp_remove_mentions")
NOTE_NLP_NOTE0(remove_parentheticals, "nlp_remove_parentheticals")
NOTE_NLP_NOTE0(remove_quotes, "nlp_remove_quotes")
NOTE_NLP_NOTE0(remove_tables, "nlp_remove_tables")
NOTE_NLP_NOTE0(remove_urls, "nlp_remove_urls")

NOTE_NLP_BATCH0(punct_tokens, "tokenize_punct")
NOTE_NLP_BATCH0(stems, "nlp_stems")
NOTE_NLP_BATCH0(lemmas, "nlp_lemmas")
NOTE_NLP_BATCH0(hashtags, "nlp_hashtags")
NOTE_NLP_BATCH0(mentions, "nlp_mentions")
NOTE_NLP_BATCH0(urls, "nlp_urls")
NOTE_NLP_BATCH0(emails, "nlp_emails")
NOTE_NLP_BATCH0(numbers, "nlp_numbers")
NOTE_NLP_BATCH0(acronyms, "nlp_extract_acronyms")
NOTE_NLP_BATCH0(definitions, "nlp_extract_definitions")
NOTE_NLP_BATCH0(terms, "nlp_extract_terms")
NOTE_NLP_BATCH0(citations, "nlp_extract_citations")
NOTE_NLP_BATCH0(math_symbols, "nlp_extract_math_symbols")
NOTE_NLP_BATCH0(lower_tokens, "nlp_lowercase_tokens")
NOTE_NLP_BATCH0(without_stopwords, "nlp_remove_stopword_tokens")
NOTE_NLP_BATCH0(without_numbers, "nlp_remove_number_tokens")
NOTE_NLP_BATCH0(alpha_tokens, "nlp_filter_tokens_alpha")
NOTE_NLP_BATCH0(alnum_tokens, "nlp_filter_tokens_alnum")
NOTE_NLP_BATCH0(shuffle_tokens, "nlp_shuffle_tokens")
NOTE_NLP_BATCH0(unique_tokens, "nlp_unique_tokens")
NOTE_NLP_BATCH0(sort_tokens, "nlp_sort_tokens")

NOTE_NLP_NOTE0(to_ascii, "transliterate_ascii")
NOTE_NLP_NOTE0(to_greek, "transliterate_greek")
NOTE_NLP_NOTE0(to_cyrillic, "transliterate_cyrillic")
NOTE_NLP_NOTE0(soundex, "phonetic_soundex")
NOTE_NLP_NOTE0(metaphone, "phonetic_metaphone")

NOTE_NLP_OTHER(TokenBatch, intersect_acronyms, "intersect_acronyms")
NOTE_NLP_OTHER(TokenBatch, intersect_citations, "intersect_citations")
NOTE_NLP_OTHER(TokenBatch, intersect_symbols, "intersect_math_symbols")
NOTE_NLP_OTHER(TokenBatch, intersect_tokens, "intersect_tokens")
NOTE_NLP_OTHER(TokenBatch, intersect_terms, "intersect_terms")
NOTE_NLP_OTHER(Note, jaccard_acronyms, "jaccard_acronyms")
NOTE_NLP_OTHER(Note, jaccard_citations, "jaccard_citations")
NOTE_NLP_OTHER(Note, jaccard_symbols, "jaccard_math_symbols")
NOTE_NLP_OTHER(Note, jaccard_tokens, "jaccard_tokens")
NOTE_NLP_OTHER(Note, jaccard_terms, "jaccard_terms")
NOTE_NLP_OTHER(Note, overlap_acronyms, "overlap_acronyms")
NOTE_NLP_OTHER(Note, overlap_citations, "overlap_citations")
NOTE_NLP_OTHER(Note, overlap_symbols, "overlap_math_symbols")
NOTE_NLP_OTHER(Note, overlap_tokens, "overlap_tokens")
NOTE_NLP_OTHER(Note, overlap_terms, "overlap_terms")
NOTE_NLP_OTHER(TokenBatch, diff_words, "diff_words")
NOTE_NLP_OTHER(TokenBatch, align_tokens, "align_tokens")
NOTE_NLP_OTHER(TokenBatch, align_sentences, "align_sentences")

Note NoteNlpAccessor::normalize_punct(bool ascii_only) const {
  return _compat::invoke<Note>("nlp_normalize_punct", owner(), {ascii_only});
}

Note NoteNlpAccessor::remove_stopwords(::std::string_view language) const {
  ::std::string normalized(language);
  for (char& c : normalized)
    c = static_cast<char>(::std::tolower(static_cast<unsigned char>(c)));
  PLUSS_CHECK(normalized.empty() || normalized == "en",
              "Note.nlp().remove_stopwords: unsupported language");
  return _compat::invoke<Note>("nlp_remove_stopwords", owner(), {language});
}

Note NoteNlpAccessor::mask_pii(::std::int64_t flags) const {
  return _compat::invoke<Note>("nlp_mask_pii", owner(), {flags});
}

TokenBatch NoteNlpAccessor::tokens(::std::int64_t flags) const {
  return _compat::invoke<TokenBatch>("tokenize_words", owner(), {flags});
}

TokenBatch NoteNlpAccessor::sentences() const {
  return _compat::invoke<TokenBatch>("tokenize_sentences", owner());
}

TokenBatch NoteNlpAccessor::pos_tags() const {
  return _compat::invoke<TokenBatch>("nlp_pos_tags", owner());
}

TokenBatch NoteNlpAccessor::named_entities() const {
  return _compat::invoke<TokenBatch>("nlp_named_entities", owner());
}

TokenBatch NoteNlpAccessor::keywords(::std::int64_t count) const {
  PLUSS_CHECK(count > 0, "Note.nlp().keywords: count must be positive");
  return _compat::invoke<TokenBatch>("nlp_keywords", owner(), {count});
}
TokenBatch NoteNlpAccessor::keyphrases(::std::int64_t count) const {
  PLUSS_CHECK(count > 0, "Note.nlp().keyphrases: count must be positive");
  return _compat::invoke<TokenBatch>("nlp_keyphrases", owner(), {count});
}
TokenBatch NoteNlpAccessor::char_ngrams(::std::int64_t width) const {
  PLUSS_CHECK(width > 0, "Note.nlp().char_ngrams: width must be positive");
  return _compat::invoke<TokenBatch>("nlp_char_ngrams", owner(), {width});
}
TokenBatch NoteNlpAccessor::word_ngrams(::std::int64_t width) const {
  PLUSS_CHECK(width > 0, "Note.nlp().word_ngrams: width must be positive");
  return _compat::invoke<TokenBatch>("nlp_word_ngrams", owner(), {width});
}
TokenBatch NoteNlpAccessor::min_length(::std::int64_t length) const {
  PLUSS_CHECK(length >= 0, "Note.nlp().min_length: length must be non-negative");
  return _compat::invoke<TokenBatch>("nlp_filter_tokens_len_ge", owner(), {length});
}
TokenBatch NoteNlpAccessor::max_length(::std::int64_t length) const {
  PLUSS_CHECK(length >= 0, "Note.nlp().max_length: length must be non-negative");
  return _compat::invoke<TokenBatch>("nlp_filter_tokens_len_le", owner(), {length});
}
TokenBatch NoteNlpAccessor::match_tokens(::std::string_view pattern) const {
  PLUSS_CHECK(!pattern.empty(), "Note.nlp().match_tokens: pattern must not be empty");
  return _compat::invoke<TokenBatch>("nlp_filter_tokens_regex", owner(), {pattern});
}
#undef NOTE_NLP_OTHER
#undef NOTE_NLP_BATCH0
#undef NOTE_NLP_NOTE0

} // namespace pluss::coden
