#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>

int main() {
  pluss::Note scalar = pluss::make_note("alpha beta beta");
  std::cout << "replace_first: " << scalar.replace_first("beta", "X").at(0) << "\n";
  std::cout << "remove_all:    " << scalar.remove_all("beta").at(0) << "\n";
  std::cout << "quoted:        " << scalar.quote().at(0) << "\n";

  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note docs = pluss::make_note(
      std::vector<std::string>{"One.\nTwo!", "A,b;c"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto words = docs.tokenize_words();
  std::cout << "tokenize_words offsets:";
  for (std::int64_t off : words.offsets()) std::cout << " " << off;
  std::cout << "\n";
  for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
    std::cout << "  word[" << i << "] = " << words.tokens().at(i) << "\n";
  }
  auto limited = docs.split(",", 1);
  std::cout << "split(',',1) offsets:";
  for (std::int64_t off : limited.offsets()) std::cout << " " << off;
  std::cout << "\n";

  pluss::Note tex = pluss::make_note(
      "\\documentclass{article}\n"
      "% comment\n"
      "\\begin{document}\n"
      "Text \\textbf{bold} and $x+y$.\n"
      "\\label{l1}\\ref{l1}\\cite{k}\\href{https://x.y}{link}\n"
      "\\end{document}\n");
  std::cout << "tex_to_text:   " << tex.tex_to_text().at(0) << "\n";
  std::cout << "tex_removers:  "
            << tex.tex_remove_labels().tex_remove_refs().tex_remove_citations().tex_remove_urls().at(0)
            << "\n";

  pluss::Note tex2 = pluss::make_note("\\left( x \\right) + \\Bigl[ y \\Bigr] + \\alpha + \\beta");
  std::cout << "tex_delims:    "
            << tex2.tex_remove_left_right().tex_remove_big_delims().tex_remove_spacing_cmds().at(0)
            << "\n";
  std::cout << "tex_replace:   "
            << tex2.tex_replace_alpha("A").tex_replace_beta("B").at(0)
            << "\n";

  pluss::Note pii = pluss::make_note("Email bob@example.com at 9:30am on 2025-01-03, pay $12.50 to @alice #ops");
  std::cout << "nlp_mask_pii:  " << pii.nlp_mask_pii().at(0) << "\n";
  std::cout << "nlp_mask_num:  " << pii.nlp_mask_numbers().at(0) << "\n";

  pluss::Note ext = pluss::make_note(
      "\\section{Intro} \\begin{equation}E=mc^2\\end{equation} URL https://openai.com NASA GPT");
  auto tex_cmds = ext.tex_commands();
  std::cout << "tex_commands:  ";
  for (std::int64_t i = 0; i < tex_cmds.tokens().numel(); ++i) std::cout << tex_cmds.tokens().at(i) << " ";
  std::cout << "\n";
  auto acr = ext.nlp_extract_acronyms();
  std::cout << "nlp_acronyms:  ";
  for (std::int64_t i = 0; i < acr.tokens().numel(); ++i) std::cout << acr.tokens().at(i) << " ";
  std::cout << "\n";

  pluss::Note tok = pluss::make_note("foo_bar CamelCase baz 2025");
  auto uniq = tok.nlp_unique_tokens();
  std::cout << "nlp_unique:    ";
  for (std::int64_t i = 0; i < uniq.tokens().numel(); ++i) std::cout << uniq.tokens().at(i) << " ";
  std::cout << "\n";
  auto len_ge = tok.nlp_filter_tokens_len_ge(4);
  std::cout << "nlp_len_ge4:   ";
  for (std::int64_t i = 0; i < len_ge.tokens().numel(); ++i) std::cout << len_ge.tokens().at(i) << " ";
  std::cout << "\n";

  pluss::Note tex_alias = pluss::make_note("\\begin{theorem}A\\end{theorem} \\caption{Cap}");
  auto th = tex_alias.tex_theorems();
  std::cout << "tex_theorems:  ";
  for (std::int64_t i = 0; i < th.tokens().numel(); ++i) std::cout << th.tokens().at(i) << " ";
  std::cout << "\n";

  pluss::Note d1 = pluss::make_note("alpha beta gamma");
  pluss::Note d2 = pluss::make_note("alpha theta gamma delta");
  auto diff = d1.diff_words(d2);
  std::cout << "diff_words:    ";
  for (std::int64_t i = 0; i < diff.tokens().numel(); ++i) std::cout << diff.tokens().at(i) << " ";
  std::cout << "\n";
  auto align = d1.align_tokens(d2);
  std::cout << "align_tokens:  ";
  for (std::int64_t i = 0; i < align.tokens().numel(); ++i) std::cout << align.tokens().at(i) << " ";
  std::cout << "\n";

  auto inter = d1.intersect_tokens(d2);
  std::cout << "inter_tokens:  ";
  for (std::int64_t i = 0; i < inter.tokens().numel(); ++i) std::cout << inter.tokens().at(i) << " ";
  std::cout << "\n";
  std::cout << "jacc_tokens:   " << d1.jaccard_tokens(d2).at(0) << "\n";
  std::cout << "overlap_tok:   " << d1.overlap_tokens(d2).at(0) << "\n";

  auto inter_terms = pluss::make_note("The quantum method is robust")
                         .intersect_terms(pluss::make_note("A robust quantum pipeline"));
  std::cout << "inter_terms:   ";
  for (std::int64_t i = 0; i < inter_terms.tokens().numel(); ++i) std::cout << inter_terms.tokens().at(i) << " ";
  std::cout << "\n";

  auto punct = pluss::make_note("One, two; three!!").split_on_punct();
  std::cout << "split_punct:   ";
  for (std::int64_t i = 0; i < punct.tokens().numel(); ++i) std::cout << punct.tokens().at(i) << " ";
  std::cout << "\n";

  auto sents = pluss::make_note("Hello!!! Next one? version2.0 works.").split_sentences();
  std::cout << "split_sent:    ";
  for (std::int64_t i = 0; i < sents.tokens().numel(); ++i) std::cout << "[" << sents.tokens().at(i) << "] ";
  std::cout << "\n";

  pluss::Note e1 = pluss::make_note("NASA GPT [12] x+y \\alpha");
  pluss::Note e2 = pluss::make_note("GPT NASA [12] y/x \\alpha +");
  auto ia = e1.intersect_acronyms(e2);
  std::cout << "inter_acr:     ";
  for (std::int64_t i = 0; i < ia.tokens().numel(); ++i) std::cout << ia.tokens().at(i) << " ";
  std::cout << "\n";
  std::cout << "jac_cite:      " << e1.jaccard_citations(e2).at(0) << "\n";
  std::cout << "ov_math:       " << e1.overlap_math_symbols(e2).at(0) << "\n";

  pluss::Note codec = pluss::make_note("Hi!");
  std::cout << "b64_roundtrip: " << codec.base64_encode().base64_decode().at(0) << "\n";
  std::cout << "hex_roundtrip: " << codec.hex_encode().hex_decode().at(0) << "\n";
  std::cout << "url_roundtrip: " << pluss::make_note("a b+c").url_encode().url_decode().at(0) << "\n";

  const std::array<std::int64_t, 1> sort_sizes{3};
  auto sorted = pluss::make_note(
      std::vector<std::string>{"b", "a", "b"},
      std::span<const std::int64_t>(sort_sizes.data(), sort_sizes.size())).sort();
  std::cout << "sort_unique:   ";
  for (std::int64_t i = 0; i < sorted.numel(); ++i) std::cout << sorted.at(i) << " ";
  std::cout << "| unique: ";
  auto uniq_note = sorted.unique();
  for (std::int64_t i = 0; i < uniq_note.numel(); ++i) std::cout << uniq_note.at(i) << " ";
  std::cout << "\n";

  pluss::Note nlp = pluss::make_note("I'm emailing Bob@example.com from São Paulo in 2026. NASA was GREAT.");
  std::cout << "expand_contr:  " << nlp.nlp_expand_contractions().at(0) << "\n";
  std::cout << "norm_numbers:  " << nlp.nlp_normalize_numbers().at(0) << "\n";
  std::cout << "lang/script:   " << nlp.nlp_language().at(0) << " / " << nlp.nlp_script().at(0) << "\n";
  std::cout << "sent/tox:      " << nlp.nlp_sentiment().at(0) << " / " << nlp.nlp_toxicity().at(0) << "\n";
  std::cout << "readability:   " << nlp.nlp_readability_grade().at(0) << " (" << nlp.nlp_readability_level().at(0) << ")\n";
  std::cout << "form/subj:     " << nlp.nlp_formality().at(0) << " / " << nlp.nlp_subjectivity().at(0) << "\n";
  std::cout << "translit:      " << nlp.transliterate_ascii().at(0) << "\n";
  std::cout << "soundex:       " << nlp.phonetic_soundex().at(0) << "\n";

  const std::array<std::int64_t, 2> grid_sizes{2, 3};
  pluss::Note grid = pluss::make_note(
      std::vector<std::string>{
          "alpha", "beta", "gamma",
          "line1\\nline2", "emoji 🙂", "quoted \"text\"",
      },
      std::span<const std::int64_t>(grid_sizes.data(), grid_sizes.size()));
  pluss::NotePrintOptions popt;
  popt.max_elements = 6;
  popt.max_cell_chars = 24;
  std::cout << "note::print:\n" << pluss::note::print(grid, popt) << "\n";
  return 0;
}
