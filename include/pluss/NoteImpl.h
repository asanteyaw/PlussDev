#pragma once

#include <pluss/Note.h>
#include <pluss/NoteOps.h>

// #include <iomanip>

namespace pluss::helpers {
  inline Text get_element_by_index(const Text& t, const std::vector<int>& index, int level = 0) {
    if (level == index.size()) return t;
    return get_element_by_index(t.children()[index[level]], index, level + 1);
  }

}

namespace pluss {

inline Note Note::to_lower() const { return ops::to_lower(*this); }
inline Note Note::to_upper() const { return ops::to_upper(*this); }
inline std::vector<bool> Note::is_alpha() const { return ops::is_alpha(*this); }
inline Note Note::add(const Note& other) const { return ops::add(*this, other); }
inline Note Note::where(const Note& x, const Note& y) const { return ops::where(*this, x, y); }
inline Note Note::replace(const Note& pattern, const Note& replacement) const { return ops::replace(*this, pattern, replacement); }
inline std::vector<bool> Note::eq(const Note& other) const { return ops::equal(*this, other); }
inline std::vector<bool> Note::neq(const Note& other) const { return ops::notequal(*this, other); }
inline Note Note::stack(const std::vector<Note>& others, int dim) const { return ops::stack(*this, others, dim); }
inline Note Note::squeeze(int dim) { return ops::squeeze(*this, dim); }
inline Note Note::unsqueeze(int dim) { return ops::unsqueeze(*this, dim); }
inline Note Note::map(const std::function<Chord(const Chord&)>& fn) const { return ops::map(*this, fn); }
inline Note Note::repeat(const std::vector<int64_t>& reps) const { return ops::repeat(*this, reps); }

inline Note Note::slice(const std::vector<int>& indices, int dim) const {
  Note result = *this;
  for (int i : indices) {
    result = ops::slice(result, dim, i, i + 1);  // slice from i to i+1
  }
  return result;
}

inline Note Note::T() const {
  const auto& t = tree();
  PLUSS_VALIDATE(ndim() == 2, "transpose() requires 2D Note");

  const auto& mat = t.children();
  if (mat.empty() || mat[0].children().empty()) {
    // Return empty 2D Note
    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(std::vector<Text>{})));
  }

  const size_t rows = mat.size();
  const size_t cols = mat[0].children().size();

  // Validate all rows are same width (rectangular matrix)
  for (const auto& row : mat)
    PLUSS_VALIDATE(row.children().size() == cols, "transpose() requires rectangular matrix");

  // Initialize transposed structure
  std::vector<Text> transposed(cols);
  for (auto& col : transposed)
    col = Text::make_list();

  // Fill transposed[i][j] = original[j][i]
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      transposed[j].append(mat[i].children()[j]);

  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(std::move(transposed))));
}

inline Note Note::transpose(int dim0, int dim1) const {
  const int n = ndim();
  PLUSS_VALIDATE(dim0 >= 0 && dim0 < n, "transpose(): dim0 out of bounds");
  PLUSS_VALIDATE(dim1 >= 0 && dim1 < n, "transpose(): dim1 out of bounds");
  PLUSS_VALIDATE(dim0 != dim1, "transpose(): dims must differ");

  std::vector<int64_t> old_shape = shape();
  std::vector<int64_t> new_shape = old_shape;
  std::swap(new_shape[dim0], new_shape[dim1]);

  std::function<Text(const Text&, std::vector<int>&, int)> transpose_recursive;
  transpose_recursive = [&](const Text& t, std::vector<int>& index, int level) -> Text {
    if (level == n) return pluss::helpers::get_element_by_index(tree(), index);

    int target_dim = (level == dim0) ? dim1 : (level == dim1) ? dim0 : level;

    Text result = Text::make_list();
    for (int i = 0; i < old_shape[target_dim]; ++i) {
      index[target_dim] = i;
      result.append(transpose_recursive(t, index, level + 1));
    }
    return result;
  };

  std::vector<int> index(n, 0);
  Text transposed = transpose_recursive(tree(), index, 0);
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(std::move(transposed)));
}

inline std::vector<std::string> Note::flatten() const {
  std::vector<std::string> out;

  std::function<void(const Text&)> collect = [&](const Text& t) {
    if (t.is_leaf()) {
      out.push_back(t.leaf().str());
    } else {
      for (const auto& sub : t.children())
        collect(sub);
    }
  };

  collect(tree());
  return out;
}


} // namespace pluss