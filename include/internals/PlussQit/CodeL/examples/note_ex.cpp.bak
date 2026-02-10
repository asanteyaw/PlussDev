#include <iostream>
#include <vector>
#include <pluss/pluss.h>

using namespace pluss;

int main() {
  std::cout << "--- Note examples (src/Note/examples/note_ex.cpp) ---\n\n";

  // Construction
  auto s = make_note("Hello World");                       // scalar
  auto v = make_note({"The", "quick", "brown", "fox"});
  auto m = make_note({ {"r1c1", "r1c2"}, {"r2c1", "r2c2"} });

  std::cout << "scalar: " << s.scalar().str() << "\n";

  std::cout << "vector (flatten): ";
  for (const auto &x : v.flatten()) std::cout << x << ", ";
  std::cout << "\n\n";

  std::cout << "matrix (debug):\n" << m.debug() << "\n";

  // Accessors
  auto v_vec = v.vector();
  std::cout << "vector()[1] as Chord: " << v_vec[1].str() << "\n";

  auto m_mat = m.matrix();
  std::cout << "matrix()[0][1] as Chord: " << m_mat[0][1].str() << "\n\n";

  // Text transforms
  std::cout << "to_lower: " << v.to_lower().join(" ") << "\n";
  std::cout << "to_upper: " << v.to_upper().join(" ") << "\n";

  auto mapped = v.map([](const Chord& c){ return c.to_upper(); });
  std::cout << "mapped upper: " << mapped.join(" ") << "\n\n";

  // Combine / concatenation using map (safe for different shapes)
  auto suffixed = v.map([](const Chord& c){ return Chord(c.str() + "_X"); });
  std::cout << "mapped concat suffix: " << suffixed.join(", ") << "\n";

  // Replace
  auto replaced = v.replace(make_note("brown"), make_note("green"));
  std::cout << "replace: " << replaced.join(", ") << "\n";

  // Equality / inequality
  auto v2 = make_note({"The", "quick", "brown", "dog"});
  auto eq = v.eq(v2);
  std::cout << "equal mask: "; for (auto b : eq) std::cout << b << ", "; std::cout << "\n";

  // where (cond, x, y)
  auto cond = make_note({"1","0","1","0"});
  auto sel = cond.where(v, make_note("NA"));
  std::cout << "where result: " << sel.join(",") << "\n\n";

  // NLP helpers
  auto bow = v.to_bow();
  std::cout << "BOW:\n";
  for (const auto &kv : bow) std::cout << "  '" << kv.first << "' -> " << kv.second << "\n";

  std::cout << "cosine similarity(v,v2): " << v.similarity(v2) << "\n";
  std::cout << "jaccard similarity(v,v2): " << v.similarity_jaccard(v2) << "\n";

  auto matches = v.regex_search("br\\w+");
  std::cout << "regex matches: "; for (const auto &mch : matches) std::cout << mch << ", "; std::cout << "\n";

  std::cout << "joined: " << v.join(" ") << "\n\n";

  // Shape / ndim
  std::cout << "v ndim=" << v.ndim() << " shape=[";
  for (auto d : v.shape()) std::cout << d << ",";
  std::cout << "]\n\n";

  // Transpose / T / stack / slice / squeeze / unsqueeze
  auto M = make_note({ {"a","b"}, {"c","d"} });
  std::cout << "M:\n" << M.debug() << "\n";
  std::cout << "M.T():\n" << M.T().debug() << "\n";

  auto stacked = M.stack({M, M}, 0);
  std::cout << "stacked (dim0) ndim=" << stacked.ndim() << " shape=[";
  for (auto d : stacked.shape()) std::cout << d << ",";
  std::cout << "]\n";

  auto sliced = M.slice({0}, 0); // first row
  std::cout << "sliced row0: " << sliced.debug() << "\n";

  auto squeezed = sliced.squeeze(0);
  std::cout << "squeezed ndim=" << squeezed.ndim() << " debug=" << squeezed.debug() << "\n";
  auto unsq = squeezed.unsqueeze(0);
  std::cout << "unsqueezed back ndim=" << unsq.ndim() << "\n\n";

  // repeat
  auto rpt = make_note({"x","y"}).repeat({2});
  std::cout << "repeat: " << rpt.debug() << "\n\n";

  // map example (uppercase each chord)
  auto mapped2 = make_note({"hello","world"}).map([](const Chord& c){ return c.title_case(); });
  std::cout << "map title_case: " << mapped2.join(", ") << "\n\n";

  std::cout << "--- done ---\n";
  return 0;
}
