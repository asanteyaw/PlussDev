#pragma once

#include <pluss/Note.h>
#include <internals/core/include/b10/utils/functors.h>
#include <internals/core/include/b10/deets/operations.h>
#include <initializer_list>
#include <string>
#include <vector>

namespace pluss {

/**
 * Helper functions to convert vectors to Text
 */
inline Text vector_to_text(const std::vector<Chord>& vec) {
  std::vector<Text> texts;
  texts.reserve(vec.size());
  for (const auto& chord : vec) {
    texts.emplace_back(Text(chord));
  }
  return Text(std::move(texts));
}

inline Text matrix_to_text(const std::vector<std::vector<Chord>>& mat) {
  std::vector<Text> rows;
  rows.reserve(mat.size());
  for (const auto& row : mat) {
    rows.push_back(vector_to_text(row));
  }
  return Text(std::move(rows));
}

/**
 * make_note - Construct Notes from existing C++ containers (similar to PyTorch's from_blob())
 * These functions create Notes from existing data without copying when possible
 */

// Empty Note
inline Note make_note() {
  // Create an empty Note with an empty vector
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::vector<Text>{})));
}

// Scalar forms
inline Note make_note(const char* s) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
}

inline Note make_note(char c) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(Chord(c))));
}

inline Note make_note(const std::string& s) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
}

inline Note make_note(const Chord& chord) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(chord)));
}

// Vector forms from initializer lists
inline Note make_note(std::initializer_list<char> list) {
  std::vector<Chord> vec;
  vec.reserve(list.size());
  for (char c : list) vec.emplace_back(c);
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::initializer_list<const char*> list) {
  std::vector<Chord> vec;
  vec.reserve(list.size());
  for (const auto* s : list)
    vec.emplace_back(s);
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::initializer_list<Chord> list) {
  std::vector<Chord> vec(list);
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

// Matrix forms from initializer lists
inline Note make_note(std::initializer_list<std::initializer_list<char>> mat) {
  std::vector<std::vector<Chord>> temp;
  temp.reserve(mat.size());
  for (const auto& row : mat) {
    std::vector<Chord> chord_row;
    chord_row.reserve(row.size());
    for (char c : row)
      chord_row.emplace_back(c);
    temp.emplace_back(std::move(chord_row));
  }
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
}

inline Note make_note(std::initializer_list<std::initializer_list<const char*>> mat) {
  std::vector<std::vector<Chord>> temp;
  temp.reserve(mat.size());
  for (const auto& row : mat) {
    std::vector<Chord> chord_row;
    chord_row.reserve(row.size());
    for (const auto* s : row)
      chord_row.emplace_back(s);
    temp.emplace_back(std::move(chord_row));
  }
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
}

inline Note make_note(std::initializer_list<std::initializer_list<Chord>> mat) {
  std::vector<std::vector<Chord>> temp;
  temp.reserve(mat.size());
  for (const auto& row : mat)
    temp.emplace_back(row);
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
}

// From C++ containers
inline Note make_note(const std::vector<Chord>& vec) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::vector<Chord>&& vec) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(vector_to_text(std::move(vec))));
}

inline Note make_note(const std::vector<std::vector<Chord>>& mat) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(mat)));
}

inline Note make_note(std::vector<std::vector<Chord>>&& mat) {
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(std::move(mat))));
}

// Special case
inline Note make_note(std::initializer_list<int>) {
  return make_note();
}

/**
 * content - Create Notes on the fly (similar to PyTorch's series())
 * This is the primary way to create Notes from literal values
 */
inline Note content() { return make_note(); }

inline Note content(const char* s) { return make_note(s); }

inline Note content(char c) { return make_note(c); }

inline Note content(const std::string& s) { return make_note(s); }

inline Note content(const Chord& chord) { return make_note(chord); }

inline Note content(std::initializer_list<char> list) { return make_note(list); }

inline Note content(std::initializer_list<const char*> list) { return make_note(list); }

inline Note content(std::initializer_list<Chord> list) { return make_note(list); }

inline Note content(std::initializer_list<std::initializer_list<char>> mat) { return make_note(mat); }

inline Note content(std::initializer_list<std::initializer_list<const char*>> mat) { return make_note(mat); }

inline Note content(std::initializer_list<std::initializer_list<Chord>> mat) { return make_note(mat); }

inline Note content(std::initializer_list<std::initializer_list<std::initializer_list<const char*>>> tensor3d) {
  std::vector<Text> planes;
  planes.reserve(tensor3d.size());
  for (const auto& plane : tensor3d) {
    std::vector<std::vector<Chord>> temp;
    temp.reserve(plane.size());
    for (const auto& row : plane) {
      std::vector<Chord> chord_row;
      chord_row.reserve(row.size());
      for (const auto* s : row) chord_row.emplace_back(s);
      temp.emplace_back(std::move(chord_row));
    }
    planes.emplace_back(matrix_to_text(temp));
  }
  return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(planes))));
}

/**
 * Repetition and fill functions (analogous to zeros/ones/full for Series)
 */

// Create a 1D Note with n repetitions of the same chord
inline Note repeat(const Chord& chord, int64_t count) {
  std::vector<Chord> vec;
  vec.reserve(static_cast<size_t>(count));
  for (int64_t i = 0; i < count; ++i) {
    vec.push_back(chord);
  }
  return make_note(std::move(vec));
}

// Create a Note of given shape filled with the same chord
inline Note fill(const Chord& chord, const std::vector<int64_t>& shape) {
  if (shape.empty()) {
    return make_note(chord);  // Scalar
  } else if (shape.size() == 1) {
    return repeat(chord, shape[0]);  // 1D
  } else if (shape.size() == 2) {
    // 2D matrix
    std::vector<std::vector<Chord>> mat;
    mat.reserve(static_cast<size_t>(shape[0]));
    for (int64_t i = 0; i < shape[0]; ++i) {
      std::vector<Chord> row;
      row.reserve(static_cast<size_t>(shape[1]));
      for (int64_t j = 0; j < shape[1]; ++j) {
        row.push_back(chord);
      }
      mat.emplace_back(std::move(row));
    }
    return make_note(std::move(mat));
  } else {
    // 3D+ not yet supported, fall back to 1D
    int64_t total = 1;
    for (auto s : shape) total *= s;
    return repeat(chord, total);
  }
}

/**
 * Text-specific sequence generation (no conflicts with Series)
 */

// Create a Note with alphabetic sequence ["a", "b", "c", ...]
inline Note alphabetic(char start, char end) {
  if (end < start) return make_note();
  std::vector<Chord> vec;
  vec.reserve(static_cast<size_t>(end - start + 1));
  for (char c = start; c <= end; ++c) {
    vec.emplace_back(std::string(1, c));
  }
  return make_note(std::move(vec));
}

// Create uppercase alphabetic sequence
inline Note alphabetic_upper(char start = 'A', char end = 'Z') {
  return alphabetic(start, end);
}

// Create lowercase alphabetic sequence
inline Note alphabetic_lower(char start = 'a', char end = 'z') {
  return alphabetic(start, end);
}

// Create Note from repeated pattern
inline Note tile(const Note& pattern, int64_t repeats) {
  if (pattern.ndim() == 0) {
    return repeat(pattern.scalar(), repeats);
  } else if (pattern.ndim() == 1) {
    std::vector<Chord> result;
    const auto& vec = pattern.vector();
    result.reserve(vec.size() * static_cast<size_t>(repeats));
    for (int64_t i = 0; i < repeats; ++i) {
      result.insert(result.end(), vec.begin(), vec.end());
    }
    return make_note(std::move(result));
  } else {
    // For 2D+, flatten and tile
    std::vector<Chord> result;
    const auto& vec = pattern.vector();
    result.reserve(vec.size() * static_cast<size_t>(repeats));
    for (int64_t i = 0; i < repeats; ++i) {
      result.insert(result.end(), vec.begin(), vec.end());
    }
    return make_note(std::move(result));
  }
}

} // namespace pluss
