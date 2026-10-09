#include "pluss/ai/nodeb/tasks.h"
#include "../deep/sequence_heads.h"
#include "../deep/token_classifier.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace pluss::ai::nodeb {

namespace {

void validate_pattern(const EntityPattern& pattern) {
  if (pattern.label.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage: entity-pattern labels must not be empty.");
  }
  if (pattern.tokens.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage: entity patterns must contain at least one token.");
  }
  if (std::ranges::any_of(pattern.tokens, &std::string::empty)) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage: entity-pattern tokens must not be empty.");
  }
}

void validate_patterns(const std::vector<EntityPattern>& patterns) {
  for (const auto& pattern : patterns) validate_pattern(pattern);
}

using EntityCounts = std::unordered_map<std::string, std::int64_t>;

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
        "nlp::NerStage: invalid hex payload length.");
  }
  auto from_hex = [](char ch) -> unsigned char {
    if (ch >= '0' && ch <= '9') return static_cast<unsigned char>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<unsigned char>(10 + ch - 'a');
    if (ch >= 'A' && ch <= 'F') return static_cast<unsigned char>(10 + ch - 'A');
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage: invalid hex payload.");
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

[[nodiscard]] std::vector<std::string> split_csv(std::string_view value) {
  std::vector<std::string> out;
  if (value.empty()) return out;
  std::size_t start = 0;
  while (start <= value.size()) {
    const auto comma = value.find(',', start);
    if (comma == std::string_view::npos) {
      out.emplace_back(value.substr(start));
      break;
    }
    out.emplace_back(value.substr(start, comma - start));
    start = comma + 1;
  }
  return out;
}

[[nodiscard]] std::string join_csv(const std::vector<std::string>& values) {
  std::string out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out.push_back(',');
    out += values[i];
  }
  return out;
}

[[nodiscard]] std::string normalize_token(std::string_view token, bool lowercase) {
  return lowercase ? ::pluss::coden::native::cpu::lower_ascii(token) : std::string(token);
}

[[nodiscard]] std::string token_view_for_match(const Token& token,
                                               bool use_lemmas,
                                               bool lowercase) {
  const std::string_view base =
      (use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                           : std::string_view(token.orth);
  return normalize_token(base, lowercase);
}

[[nodiscard]] bool overlaps_existing(const std::vector<Span>& spans,
                                     std::int64_t start,
                                     std::int64_t end) {
  for (const auto& span : spans) {
    if (start < span.token_end && end > span.token_start) return true;
  }
  return false;
}

void maybe_push_entity(Doc& doc,
                       const NerOptions& options,
                       std::int64_t start,
                       std::int64_t end,
                       std::string label) {
  if (end <= start) return;
  if (!options.allow_overlap && overlaps_existing(doc.entities, start, end)) return;
  doc.entities.push_back(Span{
      .token_start = start,
      .token_end = end,
      .label = std::move(label),
  });
}

[[nodiscard]] std::string phrase_key(const Doc& doc,
                                     std::int64_t token_start,
                                     std::int64_t token_end,
                                     bool lowercase) {
  std::string key;
  for (std::int64_t i = token_start; i < token_end; ++i) {
    if (!key.empty()) key.push_back(' ');
    key += normalize_token(doc.tokens[static_cast<std::size_t>(i)].orth, lowercase);
  }
  return key;
}

[[nodiscard]] std::string pick_majority_label(const EntityCounts& counts) {
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

// Reduces NER to per-token BIO tagging so it can reuse the same trainable
// per-token classifier as TaggerStage: decodes contiguous B-<label>/I-<label>
// runs back into spans, capping run length at `max_span_width` and
// requiring every token in the span to clear `min_score` (the minimum
// per-token confidence along the run), mirroring what those two options
// meant under the old span-enumeration approach.
[[nodiscard]] std::vector<Span> decode_bio_predictions(
    const std::vector<std::pair<std::string, double>>& predictions,
    std::int64_t max_span_width,
    double min_score) {
  std::vector<Span> out{};
  const auto n = static_cast<std::int64_t>(predictions.size());
  std::int64_t i = 0;
  while (i < n) {
    const auto& label = predictions[static_cast<std::size_t>(i)].first;
    if (label.size() > 2 && label.rfind("B-", 0) == 0) {
      const auto entity_label = label.substr(2);
      const std::string inside = "I-" + entity_label;
      double min_prob = predictions[static_cast<std::size_t>(i)].second;
      std::int64_t j = i + 1;
      while (j < n && (j - i) < max_span_width && predictions[static_cast<std::size_t>(j)].first == inside) {
        min_prob = std::min(min_prob, predictions[static_cast<std::size_t>(j)].second);
        ++j;
      }
      if (min_prob >= min_score) {
        out.push_back(Span{.token_start = i, .token_end = j, .label = entity_label});
      }
      i = j;
    } else {
      ++i;
    }
  }
  return out;
}

} // namespace

struct NerStage::Impl {
  std::unordered_map<std::int64_t, std::unordered_map<std::string, std::string>> phrase_best{};
  detail::TokenClassifierState sequence{};
  std::int64_t max_width = 0;
  std::int64_t doc_count = 0;
  bool trained = false;
};

NerStage::NerStage(std::vector<EntityPattern> patterns,
                   NerOptions options)
    : impl_(std::make_shared<Impl>()),
      patterns_(std::move(patterns)),
      options_(options) {
  validate_patterns(patterns_);
}

NerStage::NerStage(std::initializer_list<EntityPattern> patterns,
                   NerOptions options)
    : impl_(std::make_shared<Impl>()),
      patterns_(patterns),
      options_(options) {
  validate_patterns(patterns_);
}

std::string_view NerStage::name() const noexcept {
  return "ner";
}

NerStage& NerStage::add_pattern(EntityPattern pattern) {
  PLUSS_CALL_GUARD();
  validate_pattern(pattern);
  patterns_.push_back(std::move(pattern));
  return *this;
}

void NerStage::learn(const std::vector<Doc>& docs,
                     const TrainOptions&) {
  PLUSS_CALL_GUARD();
  auto next = std::make_shared<Impl>();
  next->doc_count = static_cast<std::int64_t>(docs.size());
  std::unordered_map<std::int64_t, std::unordered_map<std::string, EntityCounts>> counts{};

  for (const auto& doc : docs) {
    for (const auto& entity : doc.entities) {
      if (entity.empty() || entity.label.empty()) continue;
      const auto width = entity.token_end - entity.token_start;
      if (width <= 0) continue;
      const auto key = phrase_key(doc, entity.token_start, entity.token_end, options_.lowercase);
      if (key.empty()) continue;
      ++counts[width][key][entity.label];
      if (width > next->max_width) next->max_width = width;
      next->trained = true;
    }
  }

  for (auto& [width, phrases] : counts) {
    auto& best = next->phrase_best[width];
    for (auto& [key, label_counts] : phrases) {
      best.emplace(key, pick_majority_label(label_counts));
    }
  }

  if (options_.backend == TaskBackend::Sequence) {
    // Reduce span-based gold entities to per-token BIO labels so the
    // shared trainable classifier (also used by TaggerStage) can learn
    // them via ordinary per-token cross-entropy.
    std::vector<std::vector<std::string>> bio_labels{};
    bio_labels.reserve(docs.size());
    for (const auto& doc : docs) {
      std::vector<std::string> token_labels(doc.tokens.size(), "O");
      for (const auto& entity : doc.entities) {
        if (entity.empty() || entity.label.empty()) continue;
        const auto start = std::max<std::int64_t>(0, entity.token_start);
        const auto end =
            std::min<std::int64_t>(static_cast<std::int64_t>(doc.tokens.size()), entity.token_end);
        for (std::int64_t i = start; i < end; ++i) {
          token_labels[static_cast<std::size_t>(i)] = (i == start ? "B-" : "I-") + entity.label;
        }
      }
      bio_labels.push_back(std::move(token_labels));
    }
    detail::train_token_classifier(next->sequence, docs, bio_labels,
                                   detail::TokenClassifierOptions{
                                       .use_lemmas = true,
                                       .lowercase = options_.lowercase,
                                       .width = options_.sequence_width,
                                       .seed = options_.sequence_seed,
                                   });
  }
  impl_ = std::move(next);
}

TrainingReport NerStage::training_report() const {
  PLUSS_CALL_GUARD();
  TrainingReport report{};
  if (!impl_ || !impl_->trained) return report;
  report.doc_count = impl_->doc_count;
  for (const auto& [width, phrases] : impl_->phrase_best) {
    report.entry_count += static_cast<std::int64_t>(phrases.size());
  }
  report.loss_history = impl_->sequence.loss_history;
  return report;
}

void NerStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  static constexpr std::array<std::string_view, 6> kTitles{
      "dr.", "mr.", "mrs.", "ms.", "prof.", "sir",
  };

  doc.entities.clear();
  for (auto& token : doc.tokens) {
    token.entity_label.clear();
  }
  if (doc.tokens.empty()) return;

  for (const auto& pattern : patterns_) {
    if (pattern.tokens.empty()) continue;
    const auto width = static_cast<std::int64_t>(pattern.tokens.size());
    if (width > static_cast<std::int64_t>(doc.tokens.size())) continue;
    std::vector<std::string> needle;
    needle.reserve(pattern.tokens.size());
    for (const auto& token : pattern.tokens) {
      needle.push_back(normalize_token(token, options_.lowercase));
    }
    for (std::int64_t i = 0; i + width <= static_cast<std::int64_t>(doc.tokens.size()); ++i) {
      bool match = true;
      for (std::int64_t j = 0; j < width; ++j) {
        if (token_view_for_match(doc.tokens[static_cast<std::size_t>(i + j)],
                                 pattern.use_lemmas,
                                 options_.lowercase) != needle[static_cast<std::size_t>(j)]) {
          match = false;
          break;
        }
      }
      if (match) {
        maybe_push_entity(doc, options_, i, i + width, pattern.label);
      }
    }
  }

  if (options_.prefer_learned && impl_ && impl_->trained) {
    const auto max_width = std::min<std::int64_t>(
        impl_->max_width, static_cast<std::int64_t>(doc.tokens.size()));
    for (std::int64_t width = max_width; width >= 1; --width) {
      const auto by_width = impl_->phrase_best.find(width);
      if (by_width == impl_->phrase_best.end()) continue;
      for (std::int64_t i = 0; i + width <= static_cast<std::int64_t>(doc.tokens.size()); ++i) {
        const auto key = phrase_key(doc, i, i + width, options_.lowercase);
        const auto it = by_width->second.find(key);
        if (it == by_width->second.end()) continue;
        maybe_push_entity(doc, options_, i, i + width, it->second);
      }
    }
  }

  if (options_.use_heuristics) {
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(doc.tokens.size()); ++i) {
      const auto& token = doc.tokens[static_cast<std::size_t>(i)];
      if (token.morphology.is_email) {
        maybe_push_entity(doc, options_, i, i + 1, "EMAIL");
        continue;
      }
      if (token.morphology.is_url) {
        maybe_push_entity(doc, options_, i, i + 1, "URL");
        continue;
      }
      if (token.morphology.is_uppercase && token.orth.size() >= 2U) {
        maybe_push_entity(doc, options_, i, i + 1, "ORG");
      }
    }

    for (std::int64_t i = 0; i < static_cast<std::int64_t>(doc.tokens.size()); ++i) {
      const auto title = normalize_token(doc.tokens[static_cast<std::size_t>(i)].orth, true);
      if (std::find(kTitles.begin(), kTitles.end(), title) != kTitles.end()) {
        std::int64_t end = i + 1;
        while (end < static_cast<std::int64_t>(doc.tokens.size()) &&
               doc.tokens[static_cast<std::size_t>(end)].morphology.is_titlecase) {
          ++end;
        }
        if (end > i + 1) maybe_push_entity(doc, options_, i, end, "PERSON");
      }
    }

    for (std::int64_t i = 0; i + 1 < static_cast<std::int64_t>(doc.tokens.size()); ++i) {
      if (!doc.tokens[static_cast<std::size_t>(i)].morphology.is_titlecase) continue;
      std::int64_t end = i + 1;
      while (end < static_cast<std::int64_t>(doc.tokens.size()) &&
             doc.tokens[static_cast<std::size_t>(end)].morphology.is_titlecase) {
        ++end;
      }
      if (end > i + 1) maybe_push_entity(doc, options_, i, end, "PERSON");
    }
  }

  if (options_.backend == TaskBackend::Sequence && impl_ && impl_->sequence.ready) {
    const auto predictions = detail::predict_token_labels(impl_->sequence, doc);
    const auto spans =
        decode_bio_predictions(predictions, options_.sequence_max_span_width, options_.sequence_min_score);
    for (const auto& span : spans) {
      maybe_push_entity(doc, options_, span.token_start, span.token_end, span.label);
    }
  }

  std::stable_sort(doc.entities.begin(), doc.entities.end(), [](const Span& a, const Span& b) {
    if (a.token_start != b.token_start) return a.token_start < b.token_start;
    if (a.token_end != b.token_end) return a.token_end < b.token_end;
    return a.label < b.label;
  });

  for (const auto& entity : doc.entities) {
    for (std::int64_t i = entity.token_start; i < entity.token_end; ++i) {
      auto& token = doc.tokens[static_cast<std::size_t>(i)];
      if (token.entity_label.empty()) token.entity_label = entity.label;
    }
  }
}

std::vector<std::string> NerStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{
      std::string("lowercase=") + (options_.lowercase ? "1" : "0"),
      std::string("allow_overlap=") + (options_.allow_overlap ? "1" : "0"),
      std::string("use_heuristics=") + (options_.use_heuristics ? "1" : "0"),
      std::string("prefer_learned=") + (options_.prefer_learned ? "1" : "0"),
      "backend=" + std::string(detail::to_string(options_.backend)),
      "sequence_width=" + std::to_string(options_.sequence_width),
      "sequence_max_span_width=" + std::to_string(options_.sequence_max_span_width),
      "sequence_min_score=" + std::to_string(options_.sequence_min_score),
      "sequence_seed=" + std::to_string(options_.sequence_seed),
      std::string("trained=") + ((impl_ && impl_->trained) ? "1" : "0"),
      "pattern_count=" + std::to_string(patterns_.size()),
  };
  lines.reserve(lines.size() + patterns_.size() * 3U);
  for (std::size_t i = 0; i < patterns_.size(); ++i) {
    const auto& pattern = patterns_[i];
    lines.push_back("pattern_label_" + std::to_string(i) + "=" + hex_encode(pattern.label));
    lines.push_back("pattern_use_lemmas_" + std::to_string(i) + "=" + (pattern.use_lemmas ? std::string("1") : std::string("0")));
    std::vector<std::string> encoded_tokens;
    encoded_tokens.reserve(pattern.tokens.size());
    for (const auto& token : pattern.tokens) {
      encoded_tokens.push_back(hex_encode(token));
    }
    lines.push_back("pattern_tokens_" + std::to_string(i) + "=" + join_csv(encoded_tokens));
  }

  if (!impl_ || !impl_->trained) {
    lines.push_back("learned_count=0");
    lines.push_back("learned_max_width=0");
    return lines;
  }

  std::vector<std::string> learned_rows{};
  for (const auto& [width, table] : impl_->phrase_best) {
    for (const auto& [key, label] : table) {
      learned_rows.push_back(std::to_string(width) + "|" + hex_encode(label) + "|" + hex_encode(key));
    }
  }
  lines.push_back("learned_count=" + std::to_string(learned_rows.size()));
  lines.push_back("learned_max_width=" + std::to_string(impl_->max_width));
  for (std::size_t i = 0; i < learned_rows.size(); ++i) {
    lines.push_back("learned_" + std::to_string(i) + "=" + learned_rows[i]);
  }
  const auto sequence_lines = detail::dump_token_classifier_state_lines("sequence", impl_->sequence);
  lines.insert(lines.end(), sequence_lines.begin(), sequence_lines.end());
  return lines;
}

void NerStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  patterns_.clear();
  options_ = {};
  impl_ = std::make_shared<Impl>();

  std::int64_t pattern_count = -1;
  std::vector<std::string> labels{};
  std::vector<bool> use_lemmas{};
  std::vector<std::vector<std::string>> tokens{};
  std::int64_t learned_count = -1;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) {
      ::pluss::ai::nodeb::detail::fail(
          "nlp::NerStage::load_state_lines: expected key=value line.");
    }
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == "lowercase") { options_.lowercase = value == "1"; continue; }
    if (key == "allow_overlap") { options_.allow_overlap = value == "1"; continue; }
    if (key == "use_heuristics") { options_.use_heuristics = value == "1"; continue; }
    if (key == "prefer_learned") { options_.prefer_learned = value == "1"; continue; }
    if (key == "backend") { options_.backend = detail::parse_task_backend(value); continue; }
    if (key == "sequence_head") { continue; }
    if (key == "sequence_width") { options_.sequence_width = std::stoll(std::string(value)); continue; }
    if (key == "sequence_max_span_width") { options_.sequence_max_span_width = std::stoll(std::string(value)); continue; }
    if (key == "sequence_min_score") { options_.sequence_min_score = std::stod(std::string(value)); continue; }
    if (key == "sequence_seed") { options_.sequence_seed = static_cast<std::uint64_t>(std::stoull(std::string(value))); continue; }
    if (key == "quantization" || key == "distillation") { continue; }
    if (key == "trained") { impl_->trained = value == "1"; continue; }
    if (key == "pattern_count") {
      pattern_count = std::stoll(std::string(value));
      labels.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, pattern_count)), {});
      use_lemmas.assign(labels.size(), true);
      tokens.assign(labels.size(), {});
      continue;
    }
    if (key.rfind("pattern_label_", 0) == 0 &&
        std::all_of(key.begin() + 14, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(14)));
      if (idx >= labels.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::NerStage::load_state_lines: pattern label index out of range.");
      }
      labels[idx] = hex_decode(value);
      continue;
    }
    if (key.rfind("pattern_use_lemmas_", 0) == 0 &&
        std::all_of(key.begin() + 19, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(19)));
      if (idx >= use_lemmas.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::NerStage::load_state_lines: pattern use_lemmas index out of range.");
      }
      use_lemmas[idx] = value == "1";
      continue;
    }
    if (key.rfind("pattern_tokens_", 0) == 0 &&
        std::all_of(key.begin() + 15, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(15)));
      if (idx >= tokens.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::NerStage::load_state_lines: pattern token index out of range.");
      }
      const auto encoded = split_csv(value);
      auto& dest = tokens[idx];
      dest.reserve(encoded.size());
      for (const auto& item : encoded) {
        dest.push_back(hex_decode(item));
      }
      continue;
    }
    if (key == "learned_count") {
      learned_count = std::stoll(std::string(value));
      continue;
    }
    if (key == "learned_max_width") {
      impl_->max_width = std::stoll(std::string(value));
      continue;
    }
    if (key.rfind("learned_", 0) == 0 &&
        std::all_of(key.begin() + 8, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto first = value.find('|');
      const auto second = value.find('|', first == std::string_view::npos ? first : first + 1);
      if (first == std::string_view::npos || second == std::string_view::npos) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::NerStage::load_state_lines: malformed learned row.");
      }
      const auto width = std::stoll(std::string(value.substr(0, first)));
      const auto label = hex_decode(value.substr(first + 1, second - first - 1));
      const auto phrase = hex_decode(value.substr(second + 1));
      impl_->phrase_best[width].emplace(std::move(phrase), std::move(label));
      continue;
    }
    if (key.rfind("sequence_", 0) == 0) {
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }

  if (pattern_count < 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage::load_state_lines: missing pattern_count.");
  }
  for (std::size_t i = 0; i < labels.size(); ++i) {
    EntityPattern pattern{
        .label = labels[i],
        .tokens = tokens[i],
        .use_lemmas = use_lemmas[i],
    };
    validate_pattern(pattern);
    patterns_.push_back(std::move(pattern));
  }
  if (learned_count >= 0 &&
      static_cast<std::int64_t>([&]() {
        std::int64_t total = 0;
        for (const auto& [width, table] : impl_->phrase_best) {
          static_cast<void>(width);
          total += static_cast<std::int64_t>(table.size());
        }
        return total;
      }()) != learned_count) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::NerStage::load_state_lines: learned pattern count mismatch.");
  }
  detail::load_token_classifier_state_lines("sequence", lines, impl_->sequence);
}

namespace stage {

Stage ner(std::vector<EntityPattern> patterns,
          NerOptions options) {
  return std::make_shared<NerStage>(std::move(patterns), options);
}

Stage ner(std::initializer_list<EntityPattern> patterns,
          NerOptions options) {
  return std::make_shared<NerStage>(patterns, options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
