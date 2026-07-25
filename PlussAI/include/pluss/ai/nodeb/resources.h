#pragma once

#include "pluss/ai/nodeb/options.h"
#include "pluss/msharp/msharp.h"

#include <string>
#include <string_view>
#include <vector>

namespace pluss::ai::nodeb::resources {

struct LemmaRule {
  std::string_view surface{};
  std::string_view lemma{};
};

struct TokenMergeRule {
  std::string_view left{};
  std::string_view right{};
  std::string_view merged{};
};

struct LanguagePack {
  Language lang = Language::Unknown;
  std::string_view code{};
  std::string_view name{};
  pluss::msharp::RangeView::Of<std::string_view> abbreviations{};
  pluss::msharp::RangeView::Of<std::string_view> titles{};
  pluss::msharp::RangeView::Of<LemmaRule> lemma_rules{};
  pluss::msharp::RangeView::Of<TokenMergeRule> token_merges{};
};

[[nodiscard]] const LanguagePack& pack(Language lang) noexcept;
[[nodiscard]] std::vector<Language> available_languages();
[[nodiscard]] Language parse_language_code(std::string_view code);
[[nodiscard]] std::string_view language_code(Language lang) noexcept;
[[nodiscard]] std::string_view language_name(Language lang) noexcept;
[[nodiscard]] bool is_abbreviation(Language lang, std::string_view token) noexcept;
[[nodiscard]] bool is_title(Language lang, std::string_view token) noexcept;
[[nodiscard]] std::string lookup_lemma(Language lang, std::string_view token);
[[nodiscard]] bool try_merge_token_exception(Language lang,
                                             std::string_view left,
                                             std::string_view right,
                                             std::string& merged);

} // namespace pluss::ai::nodeb::resources
