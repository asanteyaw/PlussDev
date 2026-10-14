#pragma once

// Gene-protein-reaction (GPR) rules and gene knock-outs.
//   rule grammar:  gene | ( rule ) | rule and rule | rule or rule      (also &/&&, |/||; case-insensitive; "and" binds tighter)
//   an empty rule means "no gene association": the reaction is always available.
// A reaction is active iff its rule evaluates true given which genes are present. Rules are compiled once into a tree
// over gene INDICES; evaluating K gene-deletion lanes is a handful of elementwise Series ops on [K] vectors per node
// (min for AND, max for OR), giving the [K, r] activity matrix that feeds `metabolism::knockouts`.
// Reference: the semantics of COBRApy's `Gene.knock_out` / `single_gene_deletion` (GPR evaluated, inactive reactions
// forced to zero flux).

#include "pluss/subsp/natsci/biology/metabolism/fba.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism {

namespace detail {
struct RuleNode final {
  enum class Kind { Gene, And, Or } kind{Kind::Gene};
  std::int64_t gene{-1};
  std::vector<std::unique_ptr<RuleNode>> children;
};

struct RuleParser final {
  const std::string &text;
  std::map<std::string, std::int64_t> &index;
  std::vector<std::string> &names;
  std::size_t pos{0};

  void skip() {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
      ++pos;
  }
  static bool is_word(const std::string &w, const char *kw) {
    if (w.size() != std::char_traits<char>::length(kw))
      return false;
    for (std::size_t i = 0; i < w.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(w[i])) != kw[i])
        return false;
    return true;
  }
  std::string peek_word() {
    skip();
    std::size_t p = pos;
    if (p < text.size() && (text[p] == '&' || text[p] == '|')) {
      std::size_t q = p;
      while (q < text.size() && text[q] == text[p])
        ++q;
      return std::string(1, text[p]);
    }
    while (p < text.size() && !std::isspace(static_cast<unsigned char>(text[p])) && text[p] != '(' && text[p] != ')')
      ++p;
    return text.substr(pos, p - pos);
  }
  void consume_word() {
    skip();
    if (pos < text.size() && (text[pos] == '&' || text[pos] == '|')) {
      const char ch = text[pos];
      while (pos < text.size() && text[pos] == ch)
        ++pos;
      return;
    }
    while (pos < text.size() && !std::isspace(static_cast<unsigned char>(text[pos])) && text[pos] != '(' && text[pos] != ')')
      ++pos;
  }
  std::unique_ptr<RuleNode> parse_or() {
    auto left = parse_and();
    for (;;) {
      const std::string w = peek_word();
      if (is_word(w, "or") || w == "|") {
        consume_word();
        auto node = std::make_unique<RuleNode>();
        node->kind = RuleNode::Kind::Or;
        node->children.push_back(std::move(left));
        node->children.push_back(parse_and());
        left = std::move(node);
      } else
        return left;
    }
  }
  std::unique_ptr<RuleNode> parse_and() {
    auto left = parse_atom();
    for (;;) {
      const std::string w = peek_word();
      if (is_word(w, "and") || w == "&") {
        consume_word();
        auto node = std::make_unique<RuleNode>();
        node->kind = RuleNode::Kind::And;
        node->children.push_back(std::move(left));
        node->children.push_back(parse_atom());
        left = std::move(node);
      } else
        return left;
    }
  }
  std::unique_ptr<RuleNode> parse_atom() {
    skip();
    if (pos < text.size() && text[pos] == '(') {
      ++pos;
      auto inner = parse_or();
      skip();
      if (pos >= text.size() || text[pos] != ')')
        throw std::invalid_argument("metabolism: unbalanced parenthesis in gene rule '" + text + "'");
      ++pos;
      return inner;
    }
    const std::string w = peek_word();
    if (w.empty() || is_word(w, "and") || is_word(w, "or"))
      throw std::invalid_argument("metabolism: malformed gene rule '" + text + "'");
    consume_word();
    auto node = std::make_unique<RuleNode>();
    node->kind = RuleNode::Kind::Gene;
    auto it = index.find(w);
    if (it == index.end()) {
      it = index.emplace(w, static_cast<std::int64_t>(names.size())).first;
      names.push_back(w);
    }
    node->gene = it->second;
    return node;
  }
};
} // namespace detail

class GeneRules final {
public:
  // `rules[i]` is the GPR string of reaction i (empty = unconditional). Gene names are collected in order of appearance
  // unless `genes` is given (then unknown genes in a rule are an error-free extension appended at the end).
  explicit GeneRules(const std::vector<std::string> &rules, std::vector<std::string> genes = {}) : genes_(std::move(genes)) {
    for (std::size_t i = 0; i < genes_.size(); ++i)
      index_[genes_[i]] = static_cast<std::int64_t>(i);
    for (const auto &rule : rules) {
      if (rule.find_first_not_of(" \t\n") == std::string::npos) {
        trees_.push_back(nullptr);
        continue;
      }
      detail::RuleParser parser{rule, index_, genes_};
      auto tree = parser.parse_or();
      parser.skip();
      if (parser.pos != rule.size())
        throw std::invalid_argument("metabolism: trailing text in gene rule '" + rule + "'");
      trees_.push_back(std::move(tree));
    }
  }

  [[nodiscard]] std::int64_t gene_count() const { return static_cast<std::int64_t>(genes_.size()); }
  [[nodiscard]] std::int64_t reaction_count() const { return static_cast<std::int64_t>(trees_.size()); }
  [[nodiscard]] const std::vector<std::string> &genes() const { return genes_; }

  // present [K, G] (1 = gene present, 0 = deleted)  ->  reaction activity [K, r] in {0, 1}.
  [[nodiscard]] Series active(const Series &present) const {
    dynamics::require(present.dim() == 2 && present.size(1) == gene_count(), "GeneRules::active: present must be [K, genes]");
    std::vector<Series> cols;
    cols.reserve(trees_.size());
    const Series one = present.select(1, 0).mul(0.0).add(1.0);
    for (const auto &tree : trees_)
      cols.push_back((tree ? eval(*tree, present) : one).unsqueeze(-1));
    return cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), -1);
  }

  // knocked [K, G] (1 = deleted) -> knock-out mask over reactions [K, r] (1 = reaction forced inactive).
  [[nodiscard]] Series reaction_mask(const Series &knocked) const { return active(knocked.neg().add(1.0)).neg().add(1.0); }

private:
  [[nodiscard]] static Series eval(const detail::RuleNode &node, const Series &present) {
    if (node.kind == detail::RuleNode::Kind::Gene)
      return present.select(1, node.gene);
    Series acc = eval(*node.children[0], present);
    for (std::size_t i = 1; i < node.children.size(); ++i) {
      const Series rhs = eval(*node.children[i], present);
      acc = node.kind == detail::RuleNode::Kind::And ? acc.minimum(rhs) : acc.maximum(rhs);
    }
    return acc;
  }

  std::vector<std::string> genes_;
  std::map<std::string, std::int64_t> index_;
  std::vector<std::unique_ptr<detail::RuleNode>> trees_;
};

// Delete the gene sets in `knocked` [K, G]; every resulting LP is a lane of ONE batched solve.
inline Knockout gene_knockouts(const Network &net, const GeneRules &rules, const Series &objective, const Series &knocked,
                               const linprog::Options &opt = exact_options()) {
  dynamics::require(rules.reaction_count() == net.stoichiometry.size(1), "gene_knockouts: rules must cover every reaction");
  const Series k = knocked.to(net.stoichiometry.scalar_type()).to(net.stoichiometry.platform());
  return knockouts(net, objective, rules.reaction_mask(k), opt);
}

// Every single-gene deletion (K = G lanes).
inline Knockout single_gene_knockouts(const Network &net, const GeneRules &rules, const Series &objective, const linprog::Options &opt = exact_options()) {
  const Config cfg = Config{}.with_dtype(net.stoichiometry.scalar_type()).with_platform(net.stoichiometry.platform());
  return gene_knockouts(net, rules, objective, pluss::series::eye(rules.gene_count(), cfg), opt);
}

} // namespace pluss::subsp::natsci::biology::metabolism
