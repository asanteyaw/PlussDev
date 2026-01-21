// #include <pluss/Note.h>
// #include <b10/let/text.h>
// #include <b10/utils/functors.h>

// namespace pluss{
//    // Helper functions to convert vectors to Text
//    inline Text vector_to_text(const std::vector<Chord>& vec) {
//      std::vector<Text> texts;
//      texts.reserve(vec.size());
//      for (const auto& chord : vec) {
//        texts.emplace_back(Text(chord));
//      }
//      return Text(std::move(texts));
//    }
   
//    inline Text matrix_to_text(const std::vector<std::vector<Chord>>& mat) {
//      std::vector<Text> rows;
//      rows.reserve(mat.size());
//      for (const auto& row : mat) {
//        rows.push_back(vector_to_text(row));
//      }
//      return Text(std::move(rows));
//    }

//    // === Scalar forms ===
//  inline Note make_timeline(const char* s) {
//     return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
//  }
 
//  inline Note make_timeline(char c) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(c))));
//  }
 
//  inline Note make_timeline(const std::string& s) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(Text(Chord(s))));
//  }

// inline Timeline point_in_time(const let::TextStructure& structure) {
//   return structure.convert_to_note();
// }
 
//  // === Vector forms ===
//  inline Note make_timeline(std::initializer_list<char> list) {
//    std::vector<Chord> vec;
//    for (char c : list) vec.emplace_back(c);
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
//  }
 
//  inline Note make_timeline(std::initializer_list<const char*> list) {
//    std::vector<Chord> vec;
//    vec.reserve(list.size());
//    for (const auto* s : list)
//      vec.emplace_back(s);
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
//  }
 
//  inline Note make_timeline(std::initializer_list<Chord> list) {
//    std::vector<Chord> vec(list);
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
//  }
 
//  // === Matrix forms ===
//  inline Note make_timeline(std::initializer_list<std::initializer_list<char>> mat) {
//    std::vector<std::vector<Chord>> temp;
//    for (const auto& row : mat) {
//      std::vector<Chord> chord_row;
//      for (char c : row)
//        chord_row.emplace_back(c);
//      temp.emplace_back(std::move(chord_row));
//    }
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
//  }
 
//  inline Note make_timeline(std::initializer_list<std::initializer_list<const char*>> mat) {
//    std::vector<std::vector<Chord>> temp;
//    for (const auto& row : mat) {
//      std::vector<Chord> chord_row;
//      for (const auto* s : row)
//        chord_row.emplace_back(s);
//      temp.emplace_back(std::move(chord_row));
//    }
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
//  }
 
//  inline Note make_timeline(std::initializer_list<std::initializer_list<Chord>> mat) {
//    std::vector<std::vector<Chord>> temp;
//    for (const auto& row : mat)
//      temp.emplace_back(row);
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(temp)));
//  }
 
//  // === Containers ===
//  inline Note make_timeline(const std::vector<Chord>& vec) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(vec)));
//  }
 
//  inline Note make_timeline(std::vector<Chord>&& vec) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(vector_to_text(std::move(vec))));
//  }
 
//  inline Note make_timeline(const std::vector<std::vector<Chord>>& mat) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(mat)));
//  }
 
//  inline Note make_timeline(std::vector<std::vector<Chord>>&& mat) {
//    return Note::wrap_note_impl(intrusive_ptr<detail::NoteBook>::make(matrix_to_text(std::move(mat))));
//  }
 
//  // === Empty ===
//  inline Note make_timeline() {
//    return Note();
//  }
 
//  inline Note make_timeline(std::initializer_list<int>) {
//    return Note();
//  }
// }
