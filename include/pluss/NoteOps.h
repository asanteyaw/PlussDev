#pragma once

#include <pluss/Chord.h>
#include <pluss/ops/Dispatch.h>

namespace pluss::ops {

  inline Note to_lower(const Note& input) {
    return dispatch_unary(input, [](const Chord& c) {
      std::string s = c.str();
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return Chord(s);
    });
  }
  
  inline Note to_upper(const Note& input) {
    return dispatch_unary(input, [](const Chord& c) {
      std::string s = c.str();
      std::transform(s.begin(), s.end(), s.begin(), ::toupper);
      return Chord(s);
    });
  }
  
  inline std::vector<bool> is_alpha(const Note& input) {
    return dispatch_predicate(input, [](const Chord& c) {
      const std::string& s = c.str();
      return std::all_of(s.begin(), s.end(), ::isalpha);
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

// Add more ops as needed...

} // namespace pluss::ops