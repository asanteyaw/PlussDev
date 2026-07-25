#include "pluss/ai/nodeb/tasks.h"

#include <algorithm>
#include <array>

namespace pluss::ai::nodeb {

namespace {

[[nodiscard]] bool is_nominal(const Token& token) noexcept {
  return token.pos == "NOUN" || token.pos == "PROPN" || token.pos == "PRON";
}

[[nodiscard]] bool is_verbish(const Token& token) noexcept {
  return token.pos == "VERB" || token.pos == "AUX";
}

[[nodiscard]] bool is_modifier(const Token& token) noexcept {
  return token.pos == "ADJ" || token.pos == "ADV" || token.pos == "DET" || token.pos == "NUM";
}

[[nodiscard]] bool is_content(const Token& token) noexcept {
  return !token.is_punct && token.pos != "CCONJ" && token.pos != "SCONJ";
}

[[nodiscard]] std::int64_t choose_root(const Doc& doc,
                                       std::int64_t token_start,
                                       std::int64_t token_end,
                                       const SyntaxOptions& options) {
  if (options.prefer_verbs) {
    for (std::int64_t i = token_start; i < token_end; ++i) {
      if (doc.tokens[static_cast<std::size_t>(i)].pos == "VERB") return i;
    }
    for (std::int64_t i = token_start; i < token_end; ++i) {
      if (doc.tokens[static_cast<std::size_t>(i)].pos == "AUX") return i;
    }
  }
  for (std::int64_t i = token_start; i < token_end; ++i) {
    if (is_nominal(doc.tokens[static_cast<std::size_t>(i)])) return i;
  }
  for (std::int64_t i = token_start; i < token_end; ++i) {
    if (!doc.tokens[static_cast<std::size_t>(i)].is_punct) return i;
  }
  return token_start;
}

void attach(Token& token,
            std::int64_t chief,
            std::string_view role) {
  token.chief = chief;
  token.role.assign(role);
}

[[nodiscard]] std::int64_t find_prev(const Doc& doc,
                                     std::int64_t from,
                                     std::int64_t token_start,
                                     bool (*pred)(const Token&) noexcept) {
  for (std::int64_t i = from; i >= token_start; --i) {
    if (pred(doc.tokens[static_cast<std::size_t>(i)])) return i;
  }
  return -1;
}

[[nodiscard]] std::int64_t find_next(const Doc& doc,
                                     std::int64_t from,
                                     std::int64_t token_end,
                                     bool (*pred)(const Token&) noexcept) {
  for (std::int64_t i = from; i < token_end; ++i) {
    if (pred(doc.tokens[static_cast<std::size_t>(i)])) return i;
  }
  return -1;
}

[[nodiscard]] std::int64_t nominal_head_of(const Doc& doc,
                                           std::int64_t token_index) noexcept {
  if (token_index < 0 ||
      token_index >= static_cast<std::int64_t>(doc.tokens.size())) {
    return -1;
  }
  const auto& token = doc.tokens[static_cast<std::size_t>(token_index)];
  if (token.role == "compound" && token.chief >= 0) {
    return token.chief;
  }
  return token.index;
}

[[nodiscard]] std::int64_t find_next_nominal_head(const Doc& doc,
                                                  std::int64_t from,
                                                  std::int64_t token_end) {
  const auto nominal = find_next(doc, from, token_end, is_nominal);
  return nominal_head_of(doc, nominal);
}

[[nodiscard]] std::int64_t fallback_chief(const Doc& doc,
                                          std::int64_t token_start,
                                          std::int64_t token_end,
                                          std::int64_t root,
                                          std::int64_t pivot) {
  if (const auto left_verb = find_prev(doc, pivot - 1, token_start, is_verbish); left_verb >= 0) return left_verb;
  if (const auto left_nominal = find_prev(doc, pivot - 1, token_start, is_nominal); left_nominal >= 0) return left_nominal;
  if (const auto right_verb = find_next(doc, pivot + 1, token_end, is_verbish); right_verb >= 0) return right_verb;
  if (const auto right_nominal = find_next(doc, pivot + 1, token_end, is_nominal); right_nominal >= 0) return right_nominal;
  return root;
}

void attach_compound_runs(Doc& doc,
                          std::int64_t token_start,
                          std::int64_t token_end) {
  std::int64_t run_start = -1;
  for (std::int64_t i = token_start; i <= token_end; ++i) {
    const bool in_bounds = i < token_end;
    const bool nominal = in_bounds && is_nominal(doc.tokens[static_cast<std::size_t>(i)]);
    if (nominal && run_start < 0) {
      run_start = i;
      continue;
    }
    if (nominal) continue;
    if (run_start >= 0 && i - run_start >= 2) {
      const auto head = i - 1;
      for (std::int64_t j = run_start; j < head; ++j) {
        attach(doc.tokens[static_cast<std::size_t>(j)], head, "compound");
      }
    }
    run_start = -1;
  }
}

void attach_modifiers(Doc& doc,
                      std::int64_t token_start,
                      std::int64_t token_end,
                      std::int64_t root) {
  for (std::int64_t i = token_start; i < token_end; ++i) {
    auto& token = doc.tokens[static_cast<std::size_t>(i)];
    if (!token.role.empty()) continue;
    if (token.pos == "DET") {
      const auto chief = find_next_nominal_head(doc, i + 1, token_end);
      attach(token, chief >= 0 ? chief : root, "det");
      continue;
    }
    if (token.pos == "ADJ") {
      const auto right_nominal = find_next_nominal_head(doc, i + 1, token_end);
      const auto left_nominal = find_prev(doc, i - 1, token_start, is_nominal);
      const auto chief = right_nominal >= 0 ? right_nominal : (left_nominal >= 0 ? left_nominal : root);
      attach(token, chief, "amod");
      continue;
    }
    if (token.pos == "NUM") {
      const auto chief = find_next_nominal_head(doc, i + 1, token_end);
      attach(token, chief >= 0 ? chief : root, "nummod");
      continue;
    }
    if (token.pos == "ADV") {
      const auto left_verb = find_prev(doc, i - 1, token_start, is_verbish);
      const auto right_verb = find_next(doc, i + 1, token_end, is_verbish);
      const auto chief = left_verb >= 0 ? left_verb : (right_verb >= 0 ? right_verb : root);
      attach(token, chief, "advmod");
      continue;
    }
  }
}

void attach_function_words(Doc& doc,
                           std::int64_t token_start,
                           std::int64_t token_end,
                           std::int64_t root) {
  for (std::int64_t i = token_start; i < token_end; ++i) {
    auto& token = doc.tokens[static_cast<std::size_t>(i)];
    if (!token.role.empty()) continue;
    if (token.pos == "AUX") {
      attach(token, root, "aux");
      continue;
    }
    if (token.pos == "PART") {
      attach(token, root, "mark");
      continue;
    }
    if (token.pos == "SCONJ") {
      attach(token, root, "mark");
      continue;
    }
    if (token.pos == "CCONJ") {
      const auto chief = fallback_chief(doc, token_start, token_end, root, i);
      attach(token, chief, "cc");
      const auto next_content = find_next(doc, i + 1, token_end, is_content);
      if (next_content >= 0 && doc.tokens[static_cast<std::size_t>(next_content)].role.empty()) {
        attach(doc.tokens[static_cast<std::size_t>(next_content)], chief, "conj");
      }
      continue;
    }
    if (token.pos == "ADP") {
      const auto chief = fallback_chief(doc, token_start, token_end, root, i);
      attach(token, chief, "prep");
      const auto pobj = find_next_nominal_head(doc, i + 1, token_end);
      if (pobj >= 0 && doc.tokens[static_cast<std::size_t>(pobj)].role.empty()) {
        attach(doc.tokens[static_cast<std::size_t>(pobj)], i, "pobj");
      }
      continue;
    }
  }
}

void attach_content_words(Doc& doc,
                          std::int64_t token_start,
                          std::int64_t token_end,
                          std::int64_t root) {
  for (std::int64_t i = token_start; i < token_end; ++i) {
    auto& token = doc.tokens[static_cast<std::size_t>(i)];
    if (i == root) {
      attach(token, root, "root");
      continue;
    }
    if (!token.role.empty()) continue;
    if (token.is_punct) continue;
    if (is_nominal(token)) {
      attach(token, root, i < root ? "nsubj" : "obj");
      continue;
    }
    if (is_verbish(token)) {
      attach(token, root, i < root ? "aux" : "xcomp");
      continue;
    }
    attach(token, root, "dep");
  }
}

void attach_punctuation(Doc& doc,
                        std::int64_t token_start,
                        std::int64_t token_end,
                        std::int64_t root,
                        bool attach_punct) {
  if (!attach_punct) return;
  for (std::int64_t i = token_start; i < token_end; ++i) {
    auto& token = doc.tokens[static_cast<std::size_t>(i)];
    if (!token.is_punct) continue;
    const auto chief = fallback_chief(doc, token_start, token_end, root, i);
    attach(token, chief, "punct");
  }
}

void build_phrases(Doc& doc,
                   std::int64_t token_start,
                   std::int64_t token_end) {
  std::int64_t last_end = token_start;
  for (std::int64_t i = token_start; i < token_end; ++i) {
    const auto& token = doc.tokens[static_cast<std::size_t>(i)];
    if (!is_nominal(token)) continue;
    if (token.role == "compound" || token.role == "det" || token.role == "amod" || token.role == "nummod") {
      continue;
    }
    auto left = i;
    while (left - 1 >= token_start) {
      const auto& prev = doc.tokens[static_cast<std::size_t>(left - 1)];
      if (prev.chief != i) break;
      if (prev.role == "compound" || prev.role == "det" || prev.role == "amod" || prev.role == "nummod") {
        --left;
        continue;
      }
      break;
    }
    auto right = i;
    while (right + 1 < token_end) {
      const auto& next = doc.tokens[static_cast<std::size_t>(right + 1)];
      if (next.chief == i && next.role == "compound") {
        ++right;
        continue;
      }
      break;
    }
    if (left < last_end) continue;
    doc.phrases.push_back(Span{
        .token_start = left,
        .token_end = right + 1,
        .label = "phrase",
    });
    last_end = right + 1;
  }
}

void parse_sentence(Doc& doc,
                    std::int64_t token_start,
                    std::int64_t token_end,
                    const SyntaxOptions& options) {
  if (token_end <= token_start) return;
  const auto root = choose_root(doc, token_start, token_end, options);
  attach_compound_runs(doc, token_start, token_end);
  attach_modifiers(doc, token_start, token_end, root);
  attach_function_words(doc, token_start, token_end, root);
  attach_content_words(doc, token_start, token_end, root);
  attach_punctuation(doc, token_start, token_end, root, options.attach_punct);
  if (options.build_phrases) {
    build_phrases(doc, token_start, token_end);
  }
}

} // namespace

SyntaxStage::SyntaxStage(SyntaxOptions options)
    : options_(options) {}

std::string_view SyntaxStage::name() const noexcept {
  return "syntax";
}

void SyntaxStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  doc.phrases.clear();
  if (doc.tokens.empty()) return;

  for (auto& token : doc.tokens) {
    token.chief = token.index;
    token.role.clear();
  }

  if (doc.sentences.empty()) {
    parse_sentence(doc, 0, static_cast<std::int64_t>(doc.tokens.size()), options_);
    return;
  }

  for (const auto& sentence : doc.sentences) {
    parse_sentence(doc, sentence.token_start, sentence.token_end, options_);
  }
}

std::vector<std::string> SyntaxStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  return {
      std::string("prefer_verbs=") + (options_.prefer_verbs ? "1" : "0"),
      std::string("attach_punct=") + (options_.attach_punct ? "1" : "0"),
      std::string("build_phrases=") + (options_.build_phrases ? "1" : "0"),
  };
}

void SyntaxStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const auto line = lines[i];
    if (line == "prefer_verbs=1") {
      options_.prefer_verbs = true;
      continue;
    }
    if (line == "prefer_verbs=0") {
      options_.prefer_verbs = false;
      continue;
    }
    if (line == "attach_punct=1") {
      options_.attach_punct = true;
      continue;
    }
    if (line == "attach_punct=0") {
      options_.attach_punct = false;
      continue;
    }
    if (line == "build_phrases=1") {
      options_.build_phrases = true;
      continue;
    }
    if (line == "build_phrases=0") {
      options_.build_phrases = false;
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::SyntaxStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }
}

namespace stage {

Stage syntax(SyntaxOptions options) {
  return std::make_shared<SyntaxStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
