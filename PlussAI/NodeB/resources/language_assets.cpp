#include "pluss/ai/nodeb/resources.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <array>

namespace pluss::ai::nodeb::resources {

namespace {

using ::pluss::coden::native::cpu::lower_ascii;

constexpr std::string_view kEnglishAbbrevs[] = {
    "dr.", "mr.", "mrs.", "ms.", "prof.", "st.", "vs.", "etc.", "e.g.", "i.e.",
};
constexpr std::string_view kEnglishTitles[] = {
    "dr.", "mr.", "mrs.", "ms.", "prof.", "sir",
};
constexpr LemmaRule kEnglishLemmas[] = {
    {"wrote", "write"},
    {"written", "write"},
    {"was", "be"},
    {"were", "be"},
    {"is", "be"},
    {"are", "be"},
};
constexpr TokenMergeRule kEnglishMerges[] = {
    {"Dr", ".", "Dr."},
    {"Mr", ".", "Mr."},
    {"Mrs", ".", "Mrs."},
    {"Ms", ".", "Ms."},
    {"Prof", ".", "Prof."},
};

constexpr std::string_view kFrenchAbbrevs[] = {
    "m.", "mme.", "mlle.", "dr.", "pr.",
};
constexpr std::string_view kFrenchTitles[] = {
    "m.", "mme.", "mlle.", "dr.", "pr.",
};
constexpr LemmaRule kFrenchLemmas[] = {
    {"sommes", "etre"},
    {"etes", "etre"},
    {"suis", "etre"},
    {"allons", "aller"},
    {"avons", "avoir"},
};
constexpr TokenMergeRule kFrenchMerges[] = {
    {"M", ".", "M."},
    {"Mme", ".", "Mme."},
    {"Mlle", ".", "Mlle."},
    {"Dr", ".", "Dr."},
    {"Pr", ".", "Pr."},
};

constexpr std::string_view kSpanishAbbrevs[] = {
    "sr.", "sra.", "srta.", "dr.", "dra.", "ud.",
};
constexpr std::string_view kSpanishTitles[] = {
    "sr.", "sra.", "srta.", "dr.", "dra.",
};
constexpr LemmaRule kSpanishLemmas[] = {
    {"fue", "ser"},
    {"fueron", "ser"},
    {"somos", "ser"},
    {"estamos", "estar"},
    {"vamos", "ir"},
};
constexpr TokenMergeRule kSpanishMerges[] = {
    {"Sr", ".", "Sr."},
    {"Sra", ".", "Sra."},
    {"Srta", ".", "Srta."},
    {"Dr", ".", "Dr."},
    {"Dra", ".", "Dra."},
    {"Ud", ".", "Ud."},
};

constexpr std::string_view kGermanAbbrevs[] = {
    "hr.", "fr.", "dr.", "prof.",
};
constexpr std::string_view kGermanTitles[] = {
    "hr.", "fr.", "dr.", "prof.",
};
constexpr LemmaRule kGermanLemmas[] = {
    {"war", "sein"},
    {"waren", "sein"},
    {"ist", "sein"},
    {"sind", "sein"},
    {"ging", "gehen"},
};
constexpr TokenMergeRule kGermanMerges[] = {
    {"Hr", ".", "Hr."},
    {"Fr", ".", "Fr."},
    {"Dr", ".", "Dr."},
    {"Prof", ".", "Prof."},
};

const LanguagePack kUnknownPack{
    .lang = Language::Unknown,
    .code = "unknown",
    .name = "Unknown",
};
const LanguagePack kEnglishPack{
    .lang = Language::English,
    .code = "en",
    .name = "English",
    .abbreviations = pluss::msharp::RangeView::Of<std::string_view>(kEnglishAbbrevs),
    .titles = pluss::msharp::RangeView::Of<std::string_view>(kEnglishTitles),
    .lemma_rules = pluss::msharp::RangeView::Of<LemmaRule>(kEnglishLemmas),
    .token_merges = pluss::msharp::RangeView::Of<TokenMergeRule>(kEnglishMerges),
};
const LanguagePack kFrenchPack{
    .lang = Language::French,
    .code = "fr",
    .name = "French",
    .abbreviations = pluss::msharp::RangeView::Of<std::string_view>(kFrenchAbbrevs),
    .titles = pluss::msharp::RangeView::Of<std::string_view>(kFrenchTitles),
    .lemma_rules = pluss::msharp::RangeView::Of<LemmaRule>(kFrenchLemmas),
    .token_merges = pluss::msharp::RangeView::Of<TokenMergeRule>(kFrenchMerges),
};
const LanguagePack kSpanishPack{
    .lang = Language::Spanish,
    .code = "es",
    .name = "Spanish",
    .abbreviations = pluss::msharp::RangeView::Of<std::string_view>(kSpanishAbbrevs),
    .titles = pluss::msharp::RangeView::Of<std::string_view>(kSpanishTitles),
    .lemma_rules = pluss::msharp::RangeView::Of<LemmaRule>(kSpanishLemmas),
    .token_merges = pluss::msharp::RangeView::Of<TokenMergeRule>(kSpanishMerges),
};
const LanguagePack kGermanPack{
    .lang = Language::German,
    .code = "de",
    .name = "German",
    .abbreviations = pluss::msharp::RangeView::Of<std::string_view>(kGermanAbbrevs),
    .titles = pluss::msharp::RangeView::Of<std::string_view>(kGermanTitles),
    .lemma_rules = pluss::msharp::RangeView::Of<LemmaRule>(kGermanLemmas),
    .token_merges = pluss::msharp::RangeView::Of<TokenMergeRule>(kGermanMerges),
};

template <class T>
bool contains(pluss::msharp::RangeView::Of<T> values,
              std::string_view needle) noexcept {
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (values[i] == needle) return true;
  }
  return false;
}

} // namespace

const LanguagePack& pack(Language lang) noexcept {
  switch (lang) {
    case Language::English: return kEnglishPack;
    case Language::French: return kFrenchPack;
    case Language::Spanish: return kSpanishPack;
    case Language::German: return kGermanPack;
    case Language::Unknown:
    default:
      return kUnknownPack;
  }
}

std::vector<Language> available_languages() {
  return {
      Language::English,
      Language::French,
      Language::Spanish,
      Language::German,
  };
}

Language parse_language_code(std::string_view code) {
  if (code == "en" || code == "English") return Language::English;
  if (code == "fr" || code == "French") return Language::French;
  if (code == "es" || code == "Spanish") return Language::Spanish;
  if (code == "de" || code == "German") return Language::German;
  if (code == "unknown" || code == "Unknown") return Language::Unknown;
  return Language::Unknown;
}

std::string_view language_code(Language lang) noexcept {
  return pack(lang).code;
}

std::string_view language_name(Language lang) noexcept {
  return pack(lang).name;
}

bool is_abbreviation(Language lang, std::string_view token) noexcept {
  const auto norm = lower_ascii(token);
  return contains(pack(lang).abbreviations, norm);
}

bool is_title(Language lang, std::string_view token) noexcept {
  const auto norm = lower_ascii(token);
  return contains(pack(lang).titles, norm);
}

std::string lookup_lemma(Language lang, std::string_view token) {
  const auto norm = lower_ascii(token);
  const auto rules = pack(lang).lemma_rules;
  for (std::size_t i = 0; i < rules.size(); ++i) {
    if (rules[i].surface == norm) return std::string(rules[i].lemma);
  }
  return {};
}

bool try_merge_token_exception(Language lang,
                               std::string_view left,
                               std::string_view right,
                               std::string& merged) {
  const auto rules = pack(lang).token_merges;
  for (std::size_t i = 0; i < rules.size(); ++i) {
    if (rules[i].left == left && rules[i].right == right) {
      merged.assign(rules[i].merged.data(), rules[i].merged.size());
      return true;
    }
  }
  return false;
}

} // namespace pluss::ai::nodeb::resources
