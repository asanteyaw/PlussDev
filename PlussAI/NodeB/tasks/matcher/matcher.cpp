#include "pluss/ai/nodeb/tasks.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace pluss::ai::nodeb {

namespace {

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
        "nlp::MatcherStage: invalid hex payload length.");
  }
  auto from_hex = [](char ch) -> unsigned char {
    if (ch >= '0' && ch <= '9') return static_cast<unsigned char>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<unsigned char>(10 + ch - 'a');
    if (ch >= 'A' && ch <= 'F') return static_cast<unsigned char>(10 + ch - 'A');
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage: invalid hex payload.");
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

void validate_pattern(const MatcherPattern& pattern) {
  if (pattern.label.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage: pattern labels must not be empty.");
  }
  if (pattern.tokens.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage: patterns must contain at least one token.");
  }
  if (std::ranges::any_of(pattern.tokens, &std::string::empty)) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage: pattern tokens must not be empty.");
  }
}

[[nodiscard]] bool intersects_any(const std::vector<Span>& spans,
                                  std::int64_t token_start,
                                  std::int64_t token_end) noexcept {
  return std::ranges::any_of(spans, [&](const Span& span) {
    return span.token_start < token_end && token_start < span.token_end;
  });
}

void validate_patterns(const std::vector<MatcherPattern>& patterns) {
  for (const auto& pattern : patterns) validate_pattern(pattern);
}

} // namespace

MatcherStage::MatcherStage(std::vector<MatcherPattern> patterns,
                           MatcherOptions options)
    : patterns_(std::move(patterns)),
      options_(options) {
  validate_patterns(patterns_);
}

MatcherStage::MatcherStage(std::initializer_list<MatcherPattern> patterns,
                           MatcherOptions options)
    : patterns_(patterns),
      options_(options) {
  validate_patterns(patterns_);
}

std::string_view MatcherStage::name() const noexcept {
  return "matcher";
}

MatcherStage& MatcherStage::add_pattern(MatcherPattern pattern) {
  PLUSS_CALL_GUARD();
  validate_pattern(pattern);
  patterns_.push_back(std::move(pattern));
  return *this;
}

void MatcherStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  doc.matches.clear();
  if (doc.tokens.empty() || patterns_.empty()) return;

  for (const auto& pattern : patterns_) {
    if (pattern.tokens.empty()) continue;
    const auto width = static_cast<std::int64_t>(pattern.tokens.size());
    if (width > static_cast<std::int64_t>(doc.tokens.size())) continue;
    std::vector<std::string> needle;
    needle.reserve(pattern.tokens.size());
    for (const auto& token : pattern.tokens) {
      needle.push_back(normalize_token(token, options_.lowercase));
    }
    std::int64_t i = 0;
    while (i + width <= static_cast<std::int64_t>(doc.tokens.size())) {
      bool match = true;
      for (std::int64_t j = 0; j < width; ++j) {
        const auto& token = doc.tokens[static_cast<std::size_t>(i + j)];
        if (token_view_for_match(token, pattern.use_lemmas, options_.lowercase) !=
            needle[static_cast<std::size_t>(j)]) {
          match = false;
          break;
        }
      }
      // With overlap disallowed, a candidate is rejected when it intersects
      // any match already accepted, including one from an earlier pattern.
      if (match && (options_.allow_overlap || !intersects_any(doc.matches, i, i + width))) {
        doc.matches.push_back(Span{
            .token_start = i,
            .token_end = i + width,
            .label = pattern.label,
        });
        i += options_.allow_overlap ? 1 : width;
      } else {
        ++i;
      }
    }
  }
  std::stable_sort(doc.matches.begin(), doc.matches.end(), [](const Span& a, const Span& b) {
    if (a.token_start != b.token_start) return a.token_start < b.token_start;
    if (a.token_end != b.token_end) return a.token_end < b.token_end;
    return a.label < b.label;
  });
}

std::vector<std::string> MatcherStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{
      std::string("lowercase=") + (options_.lowercase ? "1" : "0"),
      std::string("allow_overlap=") + (options_.allow_overlap ? "1" : "0"),
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
  return lines;
}

void MatcherStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  patterns_.clear();
  std::int64_t pattern_count = -1;
  std::vector<std::string> labels{};
  std::vector<bool> use_lemmas{};
  std::vector<std::vector<std::string>> tokens{};

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) {
      ::pluss::ai::nodeb::detail::fail(
          "nlp::MatcherStage::load_state_lines: expected key=value line.");
    }
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == "lowercase") {
      options_.lowercase = value == "1";
      continue;
    }
    if (key == "allow_overlap") {
      options_.allow_overlap = value == "1";
      continue;
    }
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
            "nlp::MatcherStage::load_state_lines: pattern label index out of range.");
      }
      labels[idx] = hex_decode(value);
      continue;
    }
    if (key.rfind("pattern_use_lemmas_", 0) == 0 &&
        std::all_of(key.begin() + 19, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(19)));
      if (idx >= use_lemmas.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::MatcherStage::load_state_lines: pattern use_lemmas index out of range.");
      }
      use_lemmas[idx] = value == "1";
      continue;
    }
    if (key.rfind("pattern_tokens_", 0) == 0 &&
        std::all_of(key.begin() + 15, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(15)));
      if (idx >= tokens.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nlp::MatcherStage::load_state_lines: pattern token index out of range.");
      }
      const auto encoded = split_csv(value);
      auto& dest = tokens[idx];
      dest.reserve(encoded.size());
      for (const auto& item : encoded) {
        dest.push_back(hex_decode(item));
      }
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }

  if (pattern_count < 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::MatcherStage::load_state_lines: missing pattern_count.");
  }
  for (std::size_t i = 0; i < labels.size(); ++i) {
    MatcherPattern pattern{
        .label = labels[i],
        .tokens = tokens[i],
        .use_lemmas = use_lemmas[i],
    };
    validate_pattern(pattern);
    patterns_.push_back(std::move(pattern));
  }
}

namespace stage {

Stage matcher(std::vector<MatcherPattern> patterns,
              MatcherOptions options) {
  return std::make_shared<MatcherStage>(std::move(patterns), options);
}

Stage matcher(std::initializer_list<MatcherPattern> patterns,
              MatcherOptions options) {
  return std::make_shared<MatcherStage>(patterns, options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
