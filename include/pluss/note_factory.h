#pragma once

#include <pluss/Note.h>
#include <internals/aux/let/text.h>
#include <internals/aux/utils/functors.h>
#include <functional>
#include <memory>

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
 * content - Create Notes on the fly (similar to PyTorch's tensor())
 * This is the primary way to create Notes from literal values
 */
inline Note content(const let::TextStructure& structure) {
  return structure.convert_to_note();
}

/**
 * make_note - Construct Notes from existing C++ containers (similar to PyTorch's from_blob())
 * These functions create Notes from existing data without copying when possible
 */

// Empty Note
inline Note make_note() {
  // Create an empty Note with an empty vector
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(std::vector<Text>{})));
}

// Scalar forms
inline Note make_note(const char* s) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
}

inline Note make_note(char c) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(c))));
}

inline Note make_note(const std::string& s) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
}

// Vector forms from initializer lists
inline Note make_note(std::initializer_list<char> list) {
  std::vector<Chord> vec;
  vec.reserve(list.size());
  for (char c : list) vec.emplace_back(c);
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::initializer_list<const char*> list) {
  std::vector<Chord> vec;
  vec.reserve(list.size());
  for (const auto* s : list)
    vec.emplace_back(s);
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::initializer_list<Chord> list) {
  std::vector<Chord> vec(list);
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
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
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
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
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
}

inline Note make_note(std::initializer_list<std::initializer_list<Chord>> mat) {
  std::vector<std::vector<Chord>> temp;
  temp.reserve(mat.size());
  for (const auto& row : mat)
    temp.emplace_back(row);
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
}

// From C++ containers
inline Note make_note(const std::vector<Chord>& vec) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
}

inline Note make_note(std::vector<Chord>&& vec) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(std::move(vec))));
}

inline Note make_note(const std::vector<std::vector<Chord>>& mat) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(mat)));
}

inline Note make_note(std::vector<std::vector<Chord>>&& mat) {
  return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(std::move(mat))));
}

// Special case
inline Note make_note(std::initializer_list<int>) {
  return Note();
}

// TODO: Add from_blob style functions that take raw data pointers and shapes
// For example:
// template <typename T>
// inline Note from_blob(T* data, std::vector<int64_t> sizes, std::function<void(void*)> deleter = nullptr) {
//   // Implementation would create a Note from the raw data without copying
// }

} // namespace pluss
