#include <iostream>
#include <pluss/pluss.h>

using namespace pluss;

int main() {
  std::cout << "--- Note examples (src/Note/examples/note_ex.cpp) ---\n";

  // Construction
  let s = make_note("Hello World");
  auto v = make_note({"The", "quick", "brown", "fox"});
  auto m = make_note({ {"r1c1", "r1c2"}, {"r2c1", "r2c2"} });

  std::cout << "scalar: " << s.scalar().str() << "\n";
  std::cout << "vector flatten: ";
  for (auto &x : v.flatten()) std::cout << x << ", ";
  std::cout << "\n";

  std::cout << "matrix debug:\n" << m.debug() << "\n";

  // NLP helpers
  auto bow = v.to_bow();
  std::cout << "BOW:\n";
  for (auto &kv : bow) std::cout << "  '" << kv.first << "' -> " << kv.second << "\n";

  auto v2 = make_note({"quick", "brown", "dog"});
  std::cout << "cosine similarity(v,v2): " << v.similarity(v2) << "\n";
  std::cout << "jaccard similarity(v,v2): " << v.similarity_jaccard(v2) << "\n";

  auto matches = v.regex_search("br\\w+");
  std::cout << "regex matches: "; for (auto &mch : matches) std::cout << mch << ", "; std::cout << "\n";

  std::cout << "joined: " << v.join(" ") << "\n";

  // Transformations
  std::cout << "to_lower: " << v.to_lower().join(" ") << "\n";
  std::cout << "to_upper: " << v.to_upper().join(" ") << "\n";

  auto mapped = v.map([](const Chord& c){ return c.to_upper(); });
  std::cout << "mapped upper: " << mapped.join(" ") << "\n";

  // Factory via lexicon TextStructure
  auto t = content(lexicon::TextStructure{"alpha", "beta", "gamma"});
  std::cout << "from TextStructure flatten: ";
  for (auto &x : t.flatten()) std::cout << x << ", ";
  std::cout << "\n";

  // shape / ndim
  std::cout << "v ndim=" << v.ndim() << " shape=[";
  for (auto d : v.shape()) std::cout << d << ",";
  std::cout << "]\n";

  // where example
  auto cond = make_note({"1","0","1","0"});
  auto sel = dispatch_where(cond, v, make_note("NA"));
  std::cout << "where result: " << sel.join(",") << "\n";

  std::cout << "--- done ---\n";
  return 0;
}
