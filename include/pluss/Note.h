#pragma once

#include <pluss/Chord.h>
#include <internals/core/include/b10/utils/Memory.h>
#include <internals/aux/lexicon/nlp.h>
#include <regex>
#include <vector>
#include <variant>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace pluss {

class Note; // Forward declaration

// === Recursive Text Type (fixed) ===
struct Text;
using TextVariant = std::variant<Chord, std::vector<Text>>;

struct Text {
  TextVariant value;

  Text() = default;
  Text(const Chord& c) : value(c) {}
  Text(Chord&& c) : value(std::move(c)) {}
  Text(std::vector<Text> vec) : value(std::move(vec)) {}

  bool is_leaf() const { return std::holds_alternative<Chord>(value); }
  const Chord& leaf() const { return std::get<Chord>(value); }
  const std::vector<Text>& children() const { return std::get<std::vector<Text>>(value); }
  static Text make_list() { return Text{std::vector<Text>{}}; }

  void append(const Text& t) {
    PLUSS_VALIDATE(!is_leaf(), "Cannot append to a leaf Text node");
    std::get<std::vector<Text>>(value).push_back(t);
  }
};

namespace detail {

  struct NoteBook : intrusive_ptr_target {
    private:
      friend class Note;
      friend class TextStructure;
      friend class intrusive_ptr<NoteBook>;
    
      explicit NoteBook(Text val) : content(std::move(val)) {}
    
    public:
      Text content;
  };

} // namespace detail

class Note {
  intrusive_ptr<detail::NoteBook> handle_;

  // --- Private constructor: enforce factory-only usage ---
  explicit Note(intrusive_ptr<detail::NoteBook> handle)
      : handle_(std::move(handle)) {}

public:
  Note() = default;
  Note(const Note&) = default;
  Note(Note&&) noexcept = default;
  Note& operator=(const Note&) = default;
  Note& operator=(Note&&) noexcept = default;

  static Note wrap_note_impl(intrusive_ptr<detail::NoteBook> handle) { return Note(std::move(handle)); }

  bool defined() const noexcept { return static_cast<bool>(handle_); }
  const Text& tree() const { PLUSS_VALIDATE(defined(), "Note is undefined"); return handle_->content; }
  int ndim() const { return ndim_recursive(tree()); }

  std::vector<int64_t> shape() const {
    std::vector<int64_t> dims;
    shape_recursive(tree(), dims);
    return dims;
  }

  // --- Method-style ops ---
  Note to_lower() const;
  Note to_upper() const;
  std::vector<bool> is_alpha() const;
  std::vector<bool> eq(const Note& other) const;
  std::vector<bool> neq(const Note& other) const;
  std::vector<std::string> flatten() const;
  Note replace(const Note& pattern, const Note& replacement) const;
  Note add(const Note& other) const;
  Note where(const Note& x, const Note& y) const;
  Note T() const;
  Note transpose(int dim0, int dim1) const; // Alias for T()
  Note stack(const std::vector<Note>& others, int dim = 0) const;
  Note slice(const std::vector<int>& indices, int dim = 0) const;
  Note squeeze(int dim);
  Note unsqueeze(int dim);
  Note map(const std::function<Chord(const Chord&)>& fn) const;
  Note repeat(const std::vector<int64_t>& reps) const;

  // --- NLP / text utilities ---

  // --- Accessors ---
  Chord scalar() const {
    PLUSS_VALIDATE(defined(), "Note is undefined");
    const auto& t = tree();
    if (t.is_leaf()) {
      return t.leaf();
    } else if (!t.children().empty() && t.children()[0].is_leaf()) {
      return t.children()[0].leaf();
    }
    PLUSS_VALIDATE(false, "scalar() requires a scalar Note or a Note with a single scalar element");
    return Chord();
  }

  std::vector<Chord> vector() const {
    PLUSS_VALIDATE(defined(), "Note is undefined");
    const auto& t = tree();
    if (t.is_leaf()) {
      return {t.leaf()};
    }
    std::vector<Chord> result;
    const auto& children = t.children();
    result.reserve(children.size());
    for (const auto& child : children) {
      if (child.is_leaf()) {
        result.push_back(child.leaf());
      } else if (!child.children().empty() && child.children()[0].is_leaf()) {
        result.push_back(child.children()[0].leaf());
      } else {
        PLUSS_VALIDATE(false, "vector() requires leaf children or simple nested structure");
      }
    }
    return result;
  }

  std::vector<std::vector<Chord>> matrix() const {
    PLUSS_VALIDATE(defined(), "Note is undefined");
    const auto& t = tree();
    if (t.is_leaf()) {
      return {{t.leaf()}};
    }
    std::vector<std::vector<Chord>> result;
    const auto& rows = t.children();
    result.reserve(rows.size());
    for (const auto& row : rows) {
      std::vector<Chord> row_result;
      if (row.is_leaf()) {
        row_result.push_back(row.leaf());
      } else {
        const auto& cols = row.children();
        row_result.reserve(cols.size());
        for (const auto& col : cols) {
          if (col.is_leaf()) {
            row_result.push_back(col.leaf());
          } else if (!col.children().empty() && col.children()[0].is_leaf()) {
            row_result.push_back(col.children()[0].leaf());
          } else {
            PLUSS_VALIDATE(false, "matrix() requires leaf cells or simple nested structure");
          }
        }
      }
      result.push_back(std::move(row_result));
    }
    return result;
  }

  // --- NLP helpers implementation ---
  inline std::unordered_map<std::string, int64_t> to_bow() const {
    std::unordered_map<std::string, int64_t> counts;
    auto flat = flatten();
    for (const auto& s : flat) {
      // tokenize and normalize using lexicon utilities
      auto toks = lexicon::tokenize(s);
      auto filtered = lexicon::filter_stopwords(toks);
      for (auto& w : filtered) counts[w]++;
    }
    return counts;
  }

  inline double similarity(const Note& other) const {
    // Cosine similarity on bag-of-words (lowercased, stopwords removed)
    auto a = to_bow();
    auto b = other.to_bow();
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (const auto& kv : a) {
      na += double(kv.second) * double(kv.second);
      auto it = b.find(kv.first);
      if (it != b.end()) dot += double(kv.second) * double(it->second);
    }
    for (const auto& kv : b) nb += double(kv.second) * double(kv.second);
    if (na == 0.0 || nb == 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
  }

  inline double similarity_jaccard(const Note& other) const {
    auto a = to_bow();
    auto b = other.to_bow();
    std::unordered_set<std::string> sa, sb;
    for (const auto& kv : a) sa.insert(kv.first);
    for (const auto& kv : b) sb.insert(kv.first);
    size_t inter = 0;
    for (const auto& w : sa) if (sb.find(w) != sb.end()) ++inter;
    size_t uni = sa.size() + sb.size() - inter;
    if (uni == 0) return 0.0;
    return double(inter) / double(uni);
  }

  inline std::vector<std::string> regex_search(const std::string& pattern) const {
    std::vector<std::string> out;
    auto flat = flatten();
    std::regex re(pattern);
    for (const auto& s : flat) {
      try {
        std::sregex_iterator it(s.begin(), s.end(), re), end;
        for (; it != end; ++it) out.push_back(it->str());
      } catch (...) {}
    }
    return out;
  }

  inline std::string join(const std::string& sep = " ") const {
    auto flat = flatten();
    std::ostringstream oss;
    bool first = true;
    for (const auto& s : flat) {
      if (!first) oss << sep;
      oss << s;
      first = false;
    }
    return oss.str();
  }

  // --- Debug ---
  std::string debug() const {
    if (!defined()) return "<undefined>";
    
    // Special case for empty Note
    if (ndim() == 1 && shape().empty()) return "";
    
    std::ostringstream oss;
    
    // Format based on dimensionality
    if (ndim() == 0) {
      // Scalar - return without quotes
      return tree().leaf().str();
    } else if (ndim() == 1) {
      // Vector - comma-separated values without brackets
      const auto& children = tree().children();
      bool first = true;
      for (const auto& child : children) {
        if (!first) oss << ", ";
        oss << child.leaf().str();
        first = false;
      }
    } else if (ndim() == 2) {
      // Matrix - one row per line with brackets
      const auto& rows = tree().children();
      for (const auto& row : rows) {
        oss << "[";
        bool first = true;
        for (const auto& col : row.children()) {
          if (!first) oss << ", ";
          oss << col.leaf().str();
          first = false;
        }
        oss << "]\n";
      }
    } else {
      // Higher dimensions - use default pretty print
      pretty_print_recursive(tree(), oss, 0);
    }
    
    return oss.str();
  }

private:
  int ndim_recursive(const Text& t) const {
    if (t.is_leaf()) return 0;
    const auto& children = t.children();
    if (children.empty()) return 1;
    return 1 + ndim_recursive(children[0]);
  }

  void shape_recursive(const Text& t, std::vector<int64_t>& dims) const {
    if (t.is_leaf()) return;
    const auto& children = t.children();
    dims.push_back(static_cast<int64_t>(children.size()));
    if (!children.empty()) shape_recursive(children[0], dims);
  }

  void pretty_print_recursive(const Text& t, std::ostringstream& oss, int indent) const {
    if (t.is_leaf()) {
      oss << std::quoted(t.leaf().str());
      return;
    }
    const auto& vec = t.children();
    oss << "[";
    bool first = true;
    for (const auto& sub : vec) {
      if (!first) oss << ", ";
      if (!sub.is_leaf()) {
        oss << "\n" << std::string((indent + 1) * 2, ' ');
      }
      pretty_print_recursive(sub, oss, indent + 1);
      first = false;
    }
    oss << "]";
  }

  friend std::ostream& operator<<(std::ostream& os, const Note& note) {
    if (!note.defined()) return os << "<undefined>";

    const auto shape = note.shape();
    os << "Note (ndim=" << note.ndim() << ", shape=[";
    for (size_t i = 0; i < shape.size(); ++i) {
      os << shape[i];
      if (i + 1 < shape.size()) os << ", ";
    }
    os << "]):\n";

    std::ostringstream body;
    note.pretty_print_recursive(note.tree(), body, 1);
    os << std::string(2, ' ') << body.str() << "\n";
    return os;
  }
};

} // namespace pluss
