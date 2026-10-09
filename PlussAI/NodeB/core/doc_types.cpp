#include "pluss/ai/nodeb/workflow.h"

#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::ai::nodeb::core {

namespace {

[[nodiscard]] StringId stable_hash64(std::string_view value,
                                     std::uint64_t salt = 0) noexcept {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t hash = kOffset;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= kPrime;
  }
  hash ^= salt;
  hash *= kPrime;
  return hash == 0 ? 1 : hash;
}

[[nodiscard]] std::string case_name(CaseKind value) {
  switch (value) {
    case CaseKind::Lower: return "lower";
    case CaseKind::Upper: return "upper";
    case CaseKind::Title: return "title";
    case CaseKind::Mixed: return "mixed";
    case CaseKind::None:
    default:
      return "none";
  }
}

[[nodiscard]] std::string kind_name(TokenKind value) {
  switch (value) {
    case TokenKind::Word: return "word";
    case TokenKind::Number: return "number";
    case TokenKind::Url: return "url";
    case TokenKind::Email: return "email";
    case TokenKind::Punct: return "punct";
    case TokenKind::Mixed: return "mixed";
    case TokenKind::Unknown:
    default:
      return "unknown";
  }
}

[[nodiscard]] CaseKind case_from_traits(const ::pluss::coden::native::cpu::AsciiTokenTraits& traits) noexcept {
  if (!traits.has_alpha) return CaseKind::None;
  if (traits.is_titlecase) return CaseKind::Title;
  if (traits.is_uppercase) return CaseKind::Upper;
  if (traits.is_lowercase && !traits.is_uppercase) return CaseKind::Lower;
  return CaseKind::Mixed;
}

[[nodiscard]] TokenKind kind_from_traits(const ::pluss::coden::native::cpu::AsciiTokenTraits& traits) noexcept {
  if (traits.is_email) return TokenKind::Email;
  if (traits.is_url) return TokenKind::Url;
  if (traits.is_number_like) return TokenKind::Number;
  if (traits.is_punct_only) return TokenKind::Punct;
  if (traits.has_alpha && !traits.has_digit) return TokenKind::Word;
  if (traits.has_alpha || traits.has_digit) return TokenKind::Mixed;
  return TokenKind::Unknown;
}

[[nodiscard]] std::string morphology_spec_from(const ::pluss::coden::native::cpu::AsciiTokenTraits& traits,
                                               CaseKind casing,
                                               TokenKind kind) {
  return "kind=" + kind_name(kind) +
         "|case=" + case_name(casing) +
         "|ascii=" + std::string(traits.is_ascii ? "1" : "0") +
         "|alpha=" + std::string(traits.has_alpha ? "1" : "0") +
         "|digit=" + std::string(traits.has_digit ? "1" : "0") +
         "|hyphen=" + std::string(traits.has_hyphen ? "1" : "0") +
         "|apostrophe=" + std::string(traits.has_apostrophe ? "1" : "0");
}

[[nodiscard]] Morphology morphology_from_text(std::string_view text) {
  const auto traits = ::pluss::coden::native::cpu::analyze_ascii_token(text);
  const auto casing = case_from_traits(traits);
  const auto kind = kind_from_traits(traits);
  return Morphology{
      .casing = casing,
      .kind = kind,
      .length = static_cast<std::int32_t>(text.size()),
      .is_ascii = traits.is_ascii,
      .has_alpha = traits.has_alpha,
      .has_digit = traits.has_digit,
      .has_hyphen = traits.has_hyphen,
      .has_apostrophe = traits.has_apostrophe,
      .is_titlecase = traits.is_titlecase,
      .is_uppercase = traits.is_uppercase,
      .is_lowercase = traits.is_lowercase,
      .is_number_like = traits.is_number_like,
      .is_url = traits.is_url,
      .is_email = traits.is_email,
      .is_punct = traits.is_punct_only,
      .norm = ::pluss::coden::native::cpu::lower_ascii(text),
      .shape = ::pluss::coden::native::cpu::token_shape_ascii(text),
      .prefix = ::pluss::coden::native::cpu::prefix_ascii(text, 3),
      .suffix = ::pluss::coden::native::cpu::suffix_ascii(text, 3),
      .spec = morphology_spec_from(traits, casing, kind),
  };
}

[[nodiscard]] bool valid_token_index(const Doc& doc,
                                     std::int64_t token_index) noexcept {
  return token_index >= 0 &&
         token_index < static_cast<std::int64_t>(doc.tokens.size());
}

void collect_branch_indexes(const Doc& doc,
                            std::int64_t token_index,
                            std::vector<std::int64_t>& out,
                            std::vector<bool>& seen) {
  if (!valid_token_index(doc, token_index)) return;
  const auto offset = static_cast<std::size_t>(token_index);
  if (seen[offset]) return;
  seen[offset] = true;
  out.push_back(token_index);
  for (const auto& token : doc.tokens) {
    if (token.index != token_index && token.chief == token_index) {
      collect_branch_indexes(doc, token.index, out, seen);
    }
  }
}

} // namespace

StringId TextPool::intern(std::string_view value) {
  PLUSS_CALL_GUARD();
  if (value.empty()) return 0;
  if (const auto it = text_to_id_.find(value); it != text_to_id_.end()) {
    return it->second;
  }

  std::uint64_t salt = 0;
  StringId id = stable_hash64(value, salt);
  while (true) {
    const auto existing = id_to_index_.find(id);
    if (existing == id_to_index_.end()) break;
    if (values_[existing->second] == value) break;
    ++salt;
    id = stable_hash64(value, salt);
  }

  values_.emplace_back(value);
  text_to_id_.emplace(values_.back(), id);
  id_to_index_.emplace(id, values_.size() - 1);
  return id;
}

StringId TextPool::lookup(std::string_view value) const {
  PLUSS_CALL_GUARD();
  if (value.empty()) return 0;
  if (const auto it = text_to_id_.find(value); it != text_to_id_.end()) {
    return it->second;
  }
  return 0;
}

bool TextPool::contains(StringId id) const noexcept {
  return id != 0 && id_to_index_.contains(id);
}

const std::string& TextPool::at(StringId id) const {
  PLUSS_CALL_GUARD();
  if (id == 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::TextPool::at: id 0 does not name a stored string.");
  }
  const auto it = id_to_index_.find(id);
  if (it == id_to_index_.end()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::TextPool::at: unknown string id.");
  }
  return values_[it->second];
}

std::size_t TextPool::size() const noexcept {
  return values_.size();
}

void TextPool::clear() {
  PLUSS_CALL_GUARD();
  values_.clear();
  text_to_id_.clear();
  id_to_index_.clear();
}

StringId MorphBank::intern(const Morphology& value) {
  PLUSS_CALL_GUARD();
  if (text_pool_ == nullptr) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MorphBank::intern: text pool is not attached.");
  }
  if (value.spec.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MorphBank::intern: morphology spec must not be empty.");
  }
  const auto id = text_pool_->intern(value.spec);
  // A morphology id names a feature class, not a surface form. Preserve the
  // first representative so observing another member cannot mutate an
  // already-interned record behind existing tokens.
  forms_.try_emplace(id, value);
  return id;
}

const Morphology* MorphBank::find(StringId id) const noexcept {
  if (id == 0) return nullptr;
  const auto it = forms_.find(id);
  return it == forms_.end() ? nullptr : &it->second;
}

std::size_t MorphBank::size() const noexcept {
  return forms_.size();
}

void MorphBank::clear() {
  PLUSS_CALL_GUARD();
  forms_.clear();
}

StringId Lexicon::intern(std::string_view value) {
  PLUSS_CALL_GUARD();
  return texts_.intern(value);
}

StringId Lexicon::lookup(std::string_view value) const {
  PLUSS_CALL_GUARD();
  return texts_.lookup(value);
}

const std::string& Lexicon::text(StringId id) const {
  PLUSS_CALL_GUARD();
  return texts_.at(id);
}

const Lexeme& Lexicon::observe(std::string_view value) {
  PLUSS_CALL_GUARD();
  if (value.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::Lexicon::observe: surface text must not be empty.");
  }
  const auto id = texts_.intern(value);
  if (const auto it = lexemes_.find(id); it != lexemes_.end()) {
    return it->second;
  }
  const auto lower = ::pluss::coden::native::cpu::lower_ascii(value);
  const auto norm = lower;
  const auto prefix = ::pluss::coden::native::cpu::prefix_ascii(value, 3);
  const auto suffix = ::pluss::coden::native::cpu::suffix_ascii(value, 3);
  const auto morphology = morphology_from_text(value);
  const auto lower_id = texts_.intern(lower);
  const auto norm_id = texts_.intern(norm);
  const auto prefix_id = texts_.intern(prefix);
  const auto suffix_id = texts_.intern(suffix);
  const auto morphology_id = morphs_.intern(morphology);
  const auto [it, inserted] = lexemes_.emplace(id, Lexeme{
      .id = id,
      .lower_id = lower_id,
      .norm_id = norm_id,
      .prefix_id = prefix_id,
      .suffix_id = suffix_id,
      .morphology_id = morphology_id,
      .text = std::string(value),
      .lower = lower,
      .lemma = ::pluss::coden::native::cpu::simple_lemma_ascii(value),
      .norm = norm,
      .prefix = prefix,
      .suffix = suffix,
      .morphology = morphology,
  });
  static_cast<void>(inserted);
  return it->second;
}

const Lexeme* Lexicon::find(StringId id) const noexcept {
  if (id == 0) return nullptr;
  const auto it = lexemes_.find(id);
  return it == lexemes_.end() ? nullptr : &it->second;
}

const Lexeme* Lexicon::find(std::string_view value) const noexcept {
  return find(texts_.lookup(value));
}

StringId Lexicon::intern_morphology(const Morphology& value) {
  PLUSS_CALL_GUARD();
  return morphs_.intern(value);
}

const Morphology* Lexicon::find_morphology(StringId id) const noexcept {
  return morphs_.find(id);
}

std::size_t Lexicon::size() const noexcept {
  return lexemes_.size();
}

const TextPool& Lexicon::texts() const noexcept {
  return texts_;
}

const MorphBank& Lexicon::morphs() const noexcept {
  return morphs_;
}

void Lexicon::clear() {
  PLUSS_CALL_GUARD();
  texts_.clear();
  morphs_.attach(&texts_);
  morphs_.clear();
  lexemes_.clear();
}

Lexicon& Doc::ensure_lexicon() {
  PLUSS_CALL_GUARD();
  if (!lexicon) lexicon = std::make_shared<Lexicon>();
  return *lexicon;
}

const Lexicon* Doc::maybe_lexicon() const noexcept {
  return lexicon.get();
}

std::string Span::text(const Doc& doc) const {
  PLUSS_CALL_GUARD();
  if (empty() || token_start < 0 || token_end > static_cast<std::int64_t>(doc.tokens.size())) {
    return {};
  }
  const auto start = doc.tokens[static_cast<std::size_t>(token_start)].char_start;
  const auto end = doc.tokens[static_cast<std::size_t>(token_end - 1)].char_end;
  if (start < 0 || end < start || end > static_cast<std::int64_t>(doc.text.size())) {
    return {};
  }
  return doc.text.substr(static_cast<std::size_t>(start),
                         static_cast<std::size_t>(end - start));
}

std::vector<std::string> Doc::token_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(tokens.size());
  for (const auto& token : tokens) {
    out.push_back(token.orth);
  }
  return out;
}

std::vector<std::string> Doc::lemma_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(tokens.size());
  for (const auto& token : tokens) {
    out.push_back(token.lemma);
  }
  return out;
}

std::vector<std::string> Doc::sentence_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(sentences.size());
  for (const auto& sentence : sentences) {
    out.push_back(sentence.text(*this));
  }
  return out;
}

std::vector<std::string> Doc::phrase_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(phrases.size());
  for (const auto& phrase : phrases) {
    out.push_back(phrase.text(*this));
  }
  return out;
}

std::vector<std::string> Doc::match_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(matches.size());
  for (const auto& match : matches) {
    out.push_back(match.text(*this));
  }
  return out;
}

std::vector<std::string> Doc::entity_texts() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> out;
  out.reserve(entities.size());
  for (const auto& entity : entities) {
    out.push_back(entity.text(*this));
  }
  return out;
}

std::vector<std::int64_t> Doc::children_of(std::int64_t token_index) const {
  PLUSS_CALL_GUARD();
  std::vector<std::int64_t> out;
  if (!valid_token_index(*this, token_index)) return out;
  for (const auto& token : tokens) {
    if (token.index != token_index && token.chief == token_index) {
      out.push_back(token.index);
    }
  }
  return out;
}

std::vector<std::int64_t> Doc::lineage_of(std::int64_t token_index) const {
  PLUSS_CALL_GUARD();
  std::vector<std::int64_t> out;
  if (!valid_token_index(*this, token_index)) return out;
  std::vector<bool> seen(tokens.size(), false);
  std::int64_t current = token_index;
  while (valid_token_index(*this, current)) {
    const auto offset = static_cast<std::size_t>(current);
    if (seen[offset]) break;
    seen[offset] = true;
    out.push_back(current);
    const auto chief = tokens[offset].chief;
    if (chief == current || chief < 0) break;
    current = chief;
  }
  return out;
}

Span Doc::branch_of(std::int64_t token_index) const {
  PLUSS_CALL_GUARD();
  if (!valid_token_index(*this, token_index)) {
    return Span{
        .token_start = 0,
        .token_end = 0,
        .label = "branch",
    };
  }
  std::vector<std::int64_t> branch{};
  std::vector<bool> seen(tokens.size(), false);
  collect_branch_indexes(*this, token_index, branch, seen);
  auto left = token_index;
  auto right = token_index;
  for (const auto idx : branch) {
    if (idx < left) left = idx;
    if (idx > right) right = idx;
  }
  return Span{
      .token_start = left,
      .token_end = right + 1,
      .label = "branch",
  };
}

std::int64_t Doc::left_edge_of(std::int64_t token_index) const {
  PLUSS_CALL_GUARD();
  return branch_of(token_index).token_start;
}

std::int64_t Doc::right_edge_of(std::int64_t token_index) const {
  PLUSS_CALL_GUARD();
  const auto branch = branch_of(token_index);
  return branch.empty() ? branch.token_start : branch.token_end - 1;
}

} // namespace pluss::ai::nodeb::core
