#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <algorithm>

namespace lexicon {

inline std::string normalize_token(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  size_t i = 0, j = s.size();
  while (i < j && !std::isalnum(static_cast<unsigned char>(s[i]))) ++i;
  while (j > i && !std::isalnum(static_cast<unsigned char>(s[j-1]))) --j;
  if (i == 0 && j == s.size()) return s;
  return s.substr(i, j - i);
}

inline std::vector<std::string> tokenize(const std::string& text) {
  std::istringstream iss(text);
  std::vector<std::string> out;
  std::string tok;
  while (iss >> tok) {
    auto n = normalize_token(tok);
    if (!n.empty()) out.push_back(std::move(n));
  }
  return out;
}

inline const std::unordered_set<std::string>& default_stopwords() {
  static const std::unordered_set<std::string> s{
    "the","a","an","and","or","but","if","then","else","of","in","on","for","with","to","from","by","is","are","was","were","it","this","that","as","at","be","have","has","had","not","no","yes"
  };
  return s;
}

inline std::string stem_naive(std::string w) {
  static const std::vector<std::string> suf{"ing","ed","ly","es","s"};
  for (const auto& s : suf) {
    if (w.size() > s.size() + 1) {
      if (w.rfind(s) == w.size() - s.size()) {
        w.resize(w.size() - s.size());
        break;
      }
    }
  }
  return w;
}

inline std::vector<std::string> filter_stopwords(const std::vector<std::string>& toks,
                                                  const std::unordered_set<std::string>& stop = default_stopwords(),
                                                  bool do_stem = false) {
  std::vector<std::string> out;
  out.reserve(toks.size());
  for (auto t : toks) {
    if (stop.find(t) != stop.end()) continue;
    if (do_stem) t = stem_naive(std::move(t));
    if (!t.empty()) out.push_back(std::move(t));
  }
  return out;
}

} // namespace lexicon
