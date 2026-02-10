#pragma once

#include <pluss/Chord.h>
#include <pluss/ops/Dispatch.h>
#include <algorithm>

namespace pluss::ops {

  // Vectorized string transformations with parallel execution
  inline Note to_lower(const Note& input) {
    return dispatch_unary_parallel(input, [](const Chord& c) {
      std::string s = c.str();
      size_t n = s.size();
      #pragma omp parallel for if(n > 64)
      for (size_t i = 0; i < n; ++i) {
        s[i] = ::tolower(static_cast<unsigned char>(s[i]));
      }
      return Chord(s);
    });
  }
  
  inline Note to_upper(const Note& input) {
    return dispatch_unary_parallel(input, [](const Chord& c) {
      std::string s = c.str();
      size_t n = s.size();
      #pragma omp parallel for if(n > 64)
      for (size_t i = 0; i < n; ++i) {
        s[i] = ::toupper(static_cast<unsigned char>(s[i]));
      }
      return Chord(s);
    });
  }
  
  // Vectorized predicate with parallel execution
  inline std::vector<bool> is_alpha(const Note& input) {
    return dispatch_predicate_parallel(input, [](const Chord& c) {
      const std::string& s = c.str();
      for (char ch : s) {
        if (!::isalpha(static_cast<unsigned char>(ch))) return false;
      }
      return true;
    });
  }
  
  inline Note add(const Note& a, const Note& b) {
    return dispatch_binary(a, b, [](const Chord& lhs, const Chord& rhs) {
      return Chord(lhs.str() + rhs.str());
    });
  }
  
  inline Note replace(const Note& input, const Note& pattern, const Note& replacement) {
    return dispatch_ternary(input, pattern, replacement, [](const Chord& src, const Chord& pat, const Chord& repl) {
      std::string s = src.str();
      std::string::size_type pos = 0;
      while ((pos = s.find(pat.str(), pos)) != std::string::npos) {
        s.replace(pos, pat.str().length(), repl.str());
        pos += repl.str().length();
      }
      return Chord(s);
    });
  }
  
  inline std::vector<bool> equal(const Note& a, const Note& b) {
    return dispatch_predicate_binary(a, b, [](const Chord& lhs, const Chord& rhs) {
      return lhs.str() == rhs.str();
    });
  }
  
  inline std::vector<bool> notequal(const Note& a, const Note& b) {
    return dispatch_predicate_binary(a, b, [](const Chord& lhs, const Chord& rhs) {
      return lhs.str() != rhs.str();
    });
  }
  
  inline Note where(const Note& cond, const Note& x, const Note& y) {
    return dispatch_where(cond, x, y);
  }

  inline Note stack(const Note& base, const std::vector<Note>& others, int dim) {
    std::vector<Note> all = {base};
    all.insert(all.end(), others.begin(), others.end());
  
    const auto& shape0 = base.shape();
    for (const auto& n : others) {
      PLUSS_VALIDATE(n.shape() == shape0, "stack: all Notes must have the same shape");
    }
  
    return dispatch_stack(all, dim);
  }
  
  inline Note slice(const Note& note, int dim, int start, int end) {
    return dispatch_slice(note, dim, start, end);
  }
  
  inline Note squeeze(const Note& note, int dim = -1) {
    return dispatch_squeeze(note, dim);
  }
  
  inline Note unsqueeze(const Note& note, int dim) {
    return dispatch_unsqueeze(note, dim);
  }
  
  inline Note map(const Note& note, const std::function<Chord(const Chord&)>& fn) {
    return dispatch_map(note, fn);
  }
  
  inline Note repeat(const Note& note, const std::vector<int64_t>& repeats) {
    return dispatch_repeat(note, repeats);
  }
  
  // Concatenation operations
  inline Note concat(const Note& base, const Note& other) {
    std::vector<Chord> result;
    
    // Flatten base
    if (base.ndim() == 0) {
      result.push_back(base.scalar());
    } else if (base.ndim() == 1) {
      const auto& vec = base.vector();
      result.insert(result.end(), vec.begin(), vec.end());
    } else {
      const auto& vec = base.vector();
      result.insert(result.end(), vec.begin(), vec.end());
    }
    
    // Flatten other
    if (other.ndim() == 0) {
      result.push_back(other.scalar());
    } else if (other.ndim() == 1) {
      const auto& vec = other.vector();
      result.insert(result.end(), vec.begin(), vec.end());
    } else {
      const auto& vec = other.vector();
      result.insert(result.end(), vec.begin(), vec.end());
    }
    
    // Convert back to Text structure
    std::vector<Text> texts;
    texts.reserve(result.size());
    for (const auto& chord : result) {
      texts.emplace_back(Text(chord));
    }
    return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(texts))));
  }
  
  inline Note concat_multiple(const Note& base, const std::vector<Note>& others) {
    Note result = base;
    for (const auto& other : others) {
      result = concat(result, other);
    }
    return result;
  }
  
  inline Note join_notes(const Note& base, const std::vector<Note>& others, const Chord& delimiter) {
    std::vector<Chord> result;
    
    // Add base elements
    if (base.ndim() == 0) {
      result.push_back(base.scalar());
    } else if (base.ndim() == 1) {
      const auto& vec = base.vector();
      result.insert(result.end(), vec.begin(), vec.end());
    }
    
    // Add other elements with delimiter
    for (const auto& other : others) {
      result.push_back(delimiter);
      
      if (other.ndim() == 0) {
        result.push_back(other.scalar());
      } else if (other.ndim() == 1) {
        const auto& vec = other.vector();
        result.insert(result.end(), vec.begin(), vec.end());
      }
    }
    
    // Convert back to Text structure
    std::vector<Text> texts;
    texts.reserve(result.size());
    for (const auto& chord : result) {
      texts.emplace_back(Text(chord));
    }
    return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(texts))));
  }
  
  inline Note empty_like(const Note& other) {
    const auto& shape = other.shape();
    if (shape.empty() || other.ndim() == 0) {
      // Empty scalar
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(Chord(""))));
    } else if (shape[0] == 0 || other.ndim() == 1) {
      // Empty 1D
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::vector<Text>{})));
    } else {
      // Empty higher dimensions
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::vector<Text>{})));
    }
  }
  
  inline Note fill_like(const Note& other, const Chord& chord) {
    const auto& shape = other.shape();
    
    if (shape.empty() || other.ndim() == 0) {
      // Scalar
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(chord)));
    } else if (other.ndim() == 1) {
      // 1D
      std::vector<Text> texts;
      texts.reserve(static_cast<size_t>(shape[0]));
      for (int64_t i = 0; i < shape[0]; ++i) {
        texts.emplace_back(Text(chord));
      }
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(texts))));
    } else if (other.ndim() == 2) {
      // 2D matrix
      std::vector<Text> rows;
      rows.reserve(static_cast<size_t>(shape[0]));
      for (int64_t i = 0; i < shape[0]; ++i) {
        std::vector<Text> row;
        row.reserve(static_cast<size_t>(shape[1]));
        for (int64_t j = 0; j < shape[1]; ++j) {
          row.emplace_back(Text(chord));
        }
        rows.emplace_back(Text(std::move(row)));
      }
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(rows))));
    } else {
      // Higher dimensions: fall back to 1D with total size
      int64_t total = 1;
      for (auto s : shape) total *= s;
      std::vector<Text> texts;
      texts.reserve(static_cast<size_t>(total));
      for (int64_t i = 0; i < total; ++i) {
        texts.emplace_back(Text(chord));
      }
      return b10::detail::note_gen(intrusive_ptr<detail::NoteBook>::make(Text(std::move(texts))));
    }
  }

// Add more ops as needed...

} // namespace pluss::ops