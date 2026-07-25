#include "pluss/ai/nodeb/tasks.h"
#include "../deep/sequence_heads.h"
#include "../deep/token_classifier.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <array>
#include <span>
#include <sstream>
#include <unordered_map>

namespace pluss::ai::nodeb {

namespace {

using LabelCounts = std::unordered_map<std::string, std::int64_t>;

[[nodiscard]] bool contains_word(std::string_view value,
                                 std::span<const std::string_view> words) {
  for (const auto word : words) {
    if (value == word) return true;
  }
  return false;
}

[[nodiscard]] bool ends_with_any(std::string_view value,
                                 std::span<const std::string_view> suffixes) {
  for (const auto suffix : suffixes) {
    if (value.size() >= suffix.size() &&
        value.substr(value.size() - suffix.size()) == suffix) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::string normalized_form(const Token& token,
                                          const TaggerOptions& options) {
  const std::string_view base =
      (options.use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                                   : std::string_view(token.orth);
  return options.lowercase ? ::pluss::coden::native::cpu::lower_ascii(base)
                           : std::string(base);
}

[[nodiscard]] StringId learned_form_key(const Token& token,
                                        const TaggerOptions& options) noexcept {
  if (options.use_lemmas && token.lemma_id != 0) return token.lemma_id;
  return options.lowercase ? token.lower_id : token.orth_id;
}

[[nodiscard]] std::string pick_majority_label(const LabelCounts& counts) {
  std::string best{};
  std::int64_t best_count = std::numeric_limits<std::int64_t>::min();
  for (const auto& [label, count] : counts) {
    if (count > best_count || (count == best_count && label < best)) {
      best = label;
      best_count = count;
    }
  }
  return best;
}

[[nodiscard]] std::string heuristic_tag(const Token& token,
                                        const TaggerOptions& options) {
  static constexpr std::array<std::string_view, 9> kDet{
      "a", "an", "the", "this", "that", "these", "those", "each", "every",
  };
  static constexpr std::array<std::string_view, 18> kPron{
      "i", "you", "he", "she", "it", "we", "they", "me", "him",
      "her", "us", "them", "my", "your", "our", "their", "who", "whom",
  };
  static constexpr std::array<std::string_view, 16> kAux{
      "am", "is", "are", "was", "were", "be", "been", "being",
      "do", "does", "did", "have", "has", "had", "will", "would",
  };
  static constexpr std::array<std::string_view, 7> kCconj{
      "and", "or", "but", "nor", "yet", "so", "plus",
  };
  static constexpr std::array<std::string_view, 11> kSconj{
      "if", "because", "while", "although", "though", "when",
      "after", "before", "since", "unless", "whereas",
  };
  static constexpr std::array<std::string_view, 17> kAdp{
      "in", "on", "at", "by", "for", "with", "from", "of", "over",
      "under", "into", "through", "during", "between", "about", "around", "within",
  };
  static constexpr std::array<std::string_view, 8> kAdv{
      "not", "also", "very", "here", "there", "then", "now", "well",
  };
  static constexpr std::array<std::string_view, 16> kVerb{
      "be", "do", "have", "write", "read", "make", "build", "cook",
      "bake", "document", "work", "support", "live", "break", "need", "email",
  };
  static constexpr std::array<std::string_view, 11> kAdjSuffix{
      "ous", "ful", "ive", "al", "able", "ible", "less", "ic", "ary", "ish", "like",
  };
  static constexpr std::array<std::string_view, 8> kVerbSuffix{
      "ing", "ed", "ize", "ise", "ify", "en", "ate", "s",
  };
  static constexpr std::array<std::string_view, 1> kAdvSuffix{
      "ly",
  };

  const std::string base = normalized_form(token, options);
  if (token.is_punct) return "PUNCT";
  if (token.morphology.is_url || token.morphology.is_email) return "X";
  if (token.morphology.is_number_like) return "NUM";
  if (contains_word(base, kDet)) return "DET";
  if (contains_word(base, kPron)) return "PRON";
  if (contains_word(base, kAux)) return "AUX";
  if (contains_word(base, kCconj)) return "CCONJ";
  if (base == "to") return "PART";
  if (contains_word(base, kSconj)) return "SCONJ";
  if (contains_word(base, kAdp)) return "ADP";
  if (contains_word(base, kAdv) || ends_with_any(base, kAdvSuffix)) return "ADV";
  if (ends_with_any(base, kAdjSuffix)) return "ADJ";
  if (contains_word(base, kVerb) || ends_with_any(base, kVerbSuffix)) return "VERB";
  if (token.morphology.is_titlecase || token.morphology.is_uppercase) return "PROPN";
  return "NOUN";
}

[[nodiscard]] std::string hex_encode(std::string_view value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string out;
  out.reserve(value.size() * 2U);
  for (unsigned char ch : value) {
    out.push_back(digits[(ch >> 4) & 0x0f]);
    out.push_back(digits[ch & 0x0f]);
  }
  return out;
}

[[nodiscard]] std::string hex_decode(std::string_view value) {
  if ((value.size() & 1U) != 0U) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TaggerStage: invalid hex payload length.");
  }
  auto from_hex = [](char ch) -> unsigned char {
    if (ch >= '0' && ch <= '9') return static_cast<unsigned char>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<unsigned char>(10 + ch - 'a');
    if (ch >= 'A' && ch <= 'F') return static_cast<unsigned char>(10 + ch - 'A');
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TaggerStage: invalid hex payload.");
  };
  std::string out;
  out.reserve(value.size() / 2U);
  for (std::size_t i = 0; i < value.size(); i += 2U) {
    const unsigned char hi = from_hex(value[i]);
    const unsigned char lo = from_hex(value[i + 1U]);
    out.push_back(static_cast<char>((hi << 4) | lo));
  }
  return out;
}

template <class Map>
void dump_best_table(const char* prefix,
                     const Map& table,
                     std::vector<std::string>& lines) {
  lines.push_back(std::string(prefix) + "_count=" + std::to_string(table.size()));
  std::size_t i = 0;
  for (const auto& [key, label] : table) {
    lines.push_back(std::string(prefix) + "_" + std::to_string(i) + "=" +
                    std::to_string(key) + "|" + hex_encode(label));
    ++i;
  }
}

template <class Map>
void load_best_table(const char* prefix,
                     pluss::msharp::RangeView::Of<std::string_view> lines,
                     Map& table) {
  const std::string count_key = std::string(prefix) + "_count";
  const std::string row_prefix = std::string(prefix) + "_";
  std::int64_t count = -1;
  table.clear();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const auto line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == count_key) {
      count = std::stoll(std::string(value));
      continue;
    }
    if (key.rfind(row_prefix, 0) != 0 || key == count_key) continue;
    const auto pipe = value.find('|');
    if (pipe == std::string_view::npos) {
      ::pluss::ai::nodeb::detail::fail(
          std::string("nodeb::TaggerStage::load_state_lines: malformed ") + prefix + " row.");
    }
    const auto numeric = std::stoull(std::string(value.substr(0, pipe)));
    table.emplace(static_cast<StringId>(numeric), hex_decode(value.substr(pipe + 1)));
  }
  if (count >= 0 && static_cast<std::int64_t>(table.size()) != count) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nodeb::TaggerStage::load_state_lines: ") + prefix + " table size mismatch.");
  }
}

} // namespace

struct TaggerStage::Impl {
  std::unordered_map<StringId, std::string> form_best{};
  std::unordered_map<StringId, std::string> suffix_best{};
  std::unordered_map<StringId, std::string> morph_best{};
  detail::TokenClassifierState sequence{};
  bool trained = false;
};

TaggerStage::TaggerStage(TaggerOptions options)
    : impl_(std::make_shared<Impl>()),
      options_(options) {}

std::string_view TaggerStage::name() const noexcept {
  return "tagger";
}

void TaggerStage::learn(const std::vector<Doc>& docs,
                        const TrainOptions&) {
  PLUSS_CALL_GUARD();
  auto next = std::make_shared<Impl>();
  std::unordered_map<StringId, LabelCounts> form_counts{};
  std::unordered_map<StringId, LabelCounts> suffix_counts{};
  std::unordered_map<StringId, LabelCounts> morph_counts{};

  for (const auto& doc : docs) {
    for (const auto& token : doc.tokens) {
      if (token.pos.empty()) continue;
      const auto form_key = learned_form_key(token, options_);
      if (form_key != 0) ++form_counts[form_key][token.pos];
      if (token.suffix_id != 0) ++suffix_counts[token.suffix_id][token.pos];
      if (token.morphology_id != 0) ++morph_counts[token.morphology_id][token.pos];
      next->trained = true;
    }
  }

  for (const auto& [key, counts] : form_counts) {
    next->form_best.emplace(key, pick_majority_label(counts));
  }
  for (const auto& [key, counts] : suffix_counts) {
    next->suffix_best.emplace(key, pick_majority_label(counts));
  }
  for (const auto& [key, counts] : morph_counts) {
    next->morph_best.emplace(key, pick_majority_label(counts));
  }

  if (options_.backend == TaskBackend::Sequence) {
    std::vector<std::vector<std::string>> token_labels{};
    token_labels.reserve(docs.size());
    for (const auto& doc : docs) {
      std::vector<std::string> labels{};
      labels.reserve(doc.tokens.size());
      for (const auto& token : doc.tokens) labels.push_back(token.pos);
      token_labels.push_back(std::move(labels));
    }
    detail::train_token_classifier(next->sequence, docs, token_labels,
                                   detail::TokenClassifierOptions{
                                       .use_lemmas = options_.use_lemmas,
                                       .lowercase = options_.lowercase,
                                       .width = options_.sequence_width,
                                       .seed = options_.sequence_seed,
                                   });
  }
  impl_ = std::move(next);
}

void TaggerStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  const auto* learned = impl_.get();
  std::vector<std::pair<std::string, double>> sequence_predictions{};
  if (options_.backend == TaskBackend::Sequence && learned && learned->sequence.ready) {
    sequence_predictions = detail::predict_token_labels(learned->sequence, doc);
  }
  for (auto& token : doc.tokens) {
    token.pos.clear();

    if (options_.prefer_learned && learned && learned->trained) {
      const auto form_key = learned_form_key(token, options_);
      if (const auto it = learned->form_best.find(form_key);
          it != learned->form_best.end()) {
        token.pos = it->second;
        continue;
      }
      if (const auto it = learned->suffix_best.find(token.suffix_id);
          it != learned->suffix_best.end()) {
        token.pos = it->second;
        continue;
      }
      if (const auto it = learned->morph_best.find(token.morphology_id);
          it != learned->morph_best.end()) {
        token.pos = it->second;
        continue;
      }
    }

    if (options_.use_heuristics) {
      token.pos = heuristic_tag(token, options_);
    }

    if (token.pos.empty() && token.index >= 0 &&
        token.index < static_cast<std::int64_t>(sequence_predictions.size())) {
      token.pos = sequence_predictions[static_cast<std::size_t>(token.index)].first;
    }
  }
}

std::vector<std::string> TaggerStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{
      std::string("lowercase=") + (options_.lowercase ? "1" : "0"),
      std::string("use_lemmas=") + (options_.use_lemmas ? "1" : "0"),
      std::string("prefer_learned=") + (options_.prefer_learned ? "1" : "0"),
      std::string("use_heuristics=") + (options_.use_heuristics ? "1" : "0"),
      "backend=" + std::string(detail::to_string(options_.backend)),
      "sequence_head=" + std::string(detail::to_string(options_.sequence_head)),
      "sequence_width=" + std::to_string(options_.sequence_width),
      "sequence_seed=" + std::to_string(options_.sequence_seed),
      "quantization=" + std::string(detail::to_string(options_.quantization)),
      "distillation=" + std::string(detail::to_string(options_.distillation)),
      std::string("trained=") + ((impl_ && impl_->trained) ? "1" : "0"),
  };
  if (!impl_ || !impl_->trained) {
    lines.push_back("form_count=0");
    lines.push_back("suffix_count=0");
    lines.push_back("morph_count=0");
    const auto sequence_lines = detail::dump_token_classifier_state_lines("sequence", detail::TokenClassifierState{});
    lines.insert(lines.end(), sequence_lines.begin(), sequence_lines.end());
    return lines;
  }
  dump_best_table("form", impl_->form_best, lines);
  dump_best_table("suffix", impl_->suffix_best, lines);
  dump_best_table("morph", impl_->morph_best, lines);
  const auto sequence_lines = detail::dump_token_classifier_state_lines("sequence", impl_->sequence);
  lines.insert(lines.end(), sequence_lines.begin(), sequence_lines.end());
  return lines;
}

void TaggerStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  options_ = {};
  impl_ = std::make_shared<Impl>();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const auto line = lines[i];
    if (line == "lowercase=1") { options_.lowercase = true; continue; }
    if (line == "lowercase=0") { options_.lowercase = false; continue; }
    if (line == "use_lemmas=1") { options_.use_lemmas = true; continue; }
    if (line == "use_lemmas=0") { options_.use_lemmas = false; continue; }
    if (line == "prefer_learned=1") { options_.prefer_learned = true; continue; }
    if (line == "prefer_learned=0") { options_.prefer_learned = false; continue; }
    if (line == "use_heuristics=1") { options_.use_heuristics = true; continue; }
    if (line == "use_heuristics=0") { options_.use_heuristics = false; continue; }
    if (line == "trained=1") { impl_->trained = true; continue; }
    if (line == "trained=0") { impl_->trained = false; continue; }
    if (line.rfind("backend=", 0) == 0) { options_.backend = detail::parse_task_backend(std::string_view(line).substr(8)); continue; }
    if (line.rfind("sequence_head=", 0) == 0) { options_.sequence_head = detail::parse_sequence_head(std::string_view(line).substr(14)); continue; }
    if (line.rfind("sequence_width=", 0) == 0) { options_.sequence_width = std::stoll(std::string(std::string_view(line).substr(15))); continue; }
    if (line.rfind("sequence_seed=", 0) == 0) { options_.sequence_seed = static_cast<std::uint64_t>(std::stoull(std::string(std::string_view(line).substr(14)))); continue; }
    if (line.rfind("quantization=", 0) == 0) { options_.quantization = detail::parse_quantization_hint(std::string_view(line).substr(13)); continue; }
    if (line.rfind("distillation=", 0) == 0) { options_.distillation = detail::parse_distillation_hint(std::string_view(line).substr(13)); continue; }
  }
  load_best_table("form", lines, impl_->form_best);
  load_best_table("suffix", lines, impl_->suffix_best);
  load_best_table("morph", lines, impl_->morph_best);
  detail::load_token_classifier_state_lines("sequence", lines, impl_->sequence);
}

namespace stage {

Stage tagger(TaggerOptions options) {
  return std::make_shared<TaggerStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
