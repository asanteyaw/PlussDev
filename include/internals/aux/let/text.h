#pragma once

#include <vector>
#include <variant>
#include <string>
#include <initializer_list>
#include <stdexcept>
#include <sstream>
#include <pluss/Chord.h>
#include <pluss/Note.h>

namespace let {

class lex {
  std::string payload_;

public:
  lex(char c) : payload_(1, c) {}
  lex(const char* s) : payload_(s) {}
  lex(const std::string& s) : payload_(s) {}
  lex(std::string&& s) : payload_(std::move(s)) {}

  operator pluss::Chord() const {
    return pluss::Chord(payload_);
  }
};

class TextStructure {
  using Chord = pluss::Chord;
  using Text = pluss::Text;

  Text root_;

public:
  TextStructure() : root_(Text(std::vector<Text>{})) {}

  // TextStructure(const lex& value) {
  //   root_ = Text(static_cast<Chord>(value));
  // }

  template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, lex>>>
TextStructure(const T& value) {
  root_ = Text(static_cast<Chord>(lex(value)));
}

  TextStructure(std::initializer_list<lex> list) {
    // Create a vector to hold all the Text objects
    std::vector<Text> texts;
    texts.reserve(list.size());
    
    // Convert each lex to a Chord and then to a Text
    for (auto it = list.begin(); it != list.end(); ++it) {
      // Create a Chord with the string value
      Chord chord = static_cast<Chord>(*it);
      
      // Create a Text with the Chord
      Text text(chord);
      
      // Add the Text to the vector
      texts.push_back(text);
    }
    
    // Create the root Text with the vector of Text objects
    root_ = Text(std::move(texts));
  }

  TextStructure(std::initializer_list<std::initializer_list<lex>> list) {
    // Create a vector to hold all the row Text objects
    std::vector<Text> rows;
    rows.reserve(list.size());
    
    // Get the number of columns from the first row
    size_t cols = list.begin()->size();
    
    // Process each row
    for (auto row_it = list.begin(); row_it != list.end(); ++row_it) {
      PLUSS_VALIDATE(row_it->size() == cols, "All rows in 2D Note must have same width");
      
      // Create a vector to hold all the column Text objects for this row
      std::vector<Text> cols_text;
      cols_text.reserve(row_it->size());
      
      // Process each column in this row
      for (auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it) {
        // Create a Chord with the string value
        Chord chord = static_cast<Chord>(*col_it);
        
        // Create a Text with the Chord
        Text text(chord);
        
        // Add the Text to the vector
        cols_text.push_back(text);
      }
      
      // Create a Text with the vector of column Text objects
      Text row_text(std::move(cols_text));
      
      // Add this row to the rows vector
      rows.push_back(row_text);
    }
    
    // Create the root Text with the vector of row Text objects
    root_ = Text(std::move(rows));
  }

  TextStructure(std::initializer_list<std::initializer_list<std::initializer_list<lex>>> list) {
    // Create a vector to hold all the matrix Text objects
    std::vector<Text> matrices;
    matrices.reserve(list.size());
    
    // Get the dimensions from the first matrix
    const auto& first = *list.begin();
    size_t rows = first.size();
    size_t cols = first.begin()->size();
    
    // Process each matrix
    for (auto mat_it = list.begin(); mat_it != list.end(); ++mat_it) {
      PLUSS_VALIDATE(mat_it->size() == rows, "All 3D Note matrices must have same number of rows");
      
      // Create a vector to hold all the row Text objects for this matrix
      std::vector<Text> matrix_rows;
      matrix_rows.reserve(mat_it->size());
      
      // Process each row in this matrix
      for (auto row_it = mat_it->begin(); row_it != mat_it->end(); ++row_it) {
        PLUSS_VALIDATE(row_it->size() == cols, "All rows in 3D Note must have same number of columns");
        
        // Create a vector to hold all the column Text objects for this row
        std::vector<Text> row_cols;
        row_cols.reserve(row_it->size());
        
        // Process each column in this row
        for (auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it) {
          // Create a Chord with the string value
          Chord chord = static_cast<Chord>(*col_it);
          
          // Create a Text with the Chord
          Text text(chord);
          
          // Add the Text to the vector
          row_cols.push_back(text);
        }
        
        // Create a Text with the vector of column Text objects
        Text row_text(std::move(row_cols));
        
        // Add this row to the matrix_rows vector
        matrix_rows.push_back(row_text);
      }
      
      // Create a Text with the vector of row Text objects
      Text matrix_text(std::move(matrix_rows));
      
      // Add this matrix to the matrices vector
      matrices.push_back(matrix_text);
    }
    
    // Create the root Text with the vector of matrix Text objects
    root_ = Text(std::move(matrices));
  }

  pluss::Note convert_to_note() const {
    try {
      // Create a copy of the root_ to ensure it stays in scope
      pluss::Text root_copy = root_;
      
      // Use the copy to create the Note
      return pluss::Note::wrap_note_impl(
          pluss::intrusive_ptr<pluss::detail::NoteBook>::make(root_copy));
    } catch (const std::exception& e) {
      std::cerr << "Error in convert_to_note: " << e.what() << std::endl;
      // Return an empty Note in case of error
      return pluss::Note();
    }
  }
};

} // namespace let
