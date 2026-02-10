#pragma once

#include <vector>
#include <variant>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <stdexcept>

#include <pluss/Chord.h>
#include <internals/core/include/b10/utils/Macros.h>

// forward-declare pluss::Note to avoid pulling Note.h into this header
namespace pluss { class Note; }

namespace lexicon {

class lex {
  std::string payload_;

public:
  lex(char c) : payload_(1, c) {}
  lex(const char* s) : payload_(s) {}
  lex(const std::string& s) : payload_(s) {}
  lex(std::string&& s) : payload_(std::move(s)) {}

  operator pluss::Chord() const { return pluss::Chord(payload_); }
};

class TextStructure {
  using Chord = pluss::Chord;

public:
  struct TextNode {
    std::variant<Chord, std::vector<TextNode>> value;
    TextNode() = default;
    explicit TextNode(const Chord& c) : value(c) {}
    explicit TextNode(std::vector<TextNode>&& v) : value(std::move(v)) {}
    bool is_leaf() const { return std::holds_alternative<Chord>(value); }
  };

private:
  TextNode root_;

public:
  TextStructure() : root_(std::vector<TextNode>{}) {}

  template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, lex>>>
  TextStructure(const T& value) { root_ = TextNode(Chord(static_cast<Chord>(lex(value)))); }

  TextStructure(std::initializer_list<lex> list) {
    std::vector<TextNode> texts;
    texts.reserve(list.size());
    for (auto it = list.begin(); it != list.end(); ++it) texts.emplace_back(Chord(static_cast<Chord>(*it)));
    root_ = TextNode(std::move(texts));
  }

  TextStructure(std::initializer_list<std::initializer_list<lex>> list) {
    std::vector<TextNode> rows;
    rows.reserve(list.size());
    size_t cols = list.begin()->size();
    for (auto row_it = list.begin(); row_it != list.end(); ++row_it) {
      PLUSS_VALIDATE(row_it->size() == cols, "All rows in 2D Note must have same width");
      std::vector<TextNode> cols_text;
      cols_text.reserve(row_it->size());
      for (auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it)
        cols_text.emplace_back(Chord(static_cast<Chord>(*col_it)));
      rows.emplace_back(std::move(cols_text));
    }
    root_ = TextNode(std::move(rows));
  }

  TextStructure(std::initializer_list<std::initializer_list<std::initializer_list<lex>>> list) {
    std::vector<TextNode> matrices;
    matrices.reserve(list.size());
    const auto& first = *list.begin();
    size_t rows = first.size();
    size_t cols = first.begin()->size();
    for (auto mat_it = list.begin(); mat_it != list.end(); ++mat_it) {
      PLUSS_VALIDATE(mat_it->size() == rows, "All 3D Note matrices must have same number of rows");
      std::vector<TextNode> matrix_rows;
      matrix_rows.reserve(mat_it->size());
      for (auto row_it = mat_it->begin(); row_it != mat_it->end(); ++row_it) {
        PLUSS_VALIDATE(row_it->size() == cols, "All rows in 3D Note must have same number of columns");
        std::vector<TextNode> row_cols;
        row_cols.reserve(row_it->size());
        for (auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it)
          row_cols.emplace_back(Chord(static_cast<Chord>(*col_it)));
        matrix_rows.emplace_back(std::move(row_cols));
      }
      matrices.emplace_back(std::move(matrix_rows));
    }
    root_ = TextNode(std::move(matrices));
  }

  // Convert to pluss::Note -- implemented in a .cpp to avoid tight header coupling
  pluss::Note convert_to_note() const;
  const TextNode& root_node() const { return root_; }
};

} // namespace lexicon
