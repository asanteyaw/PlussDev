#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void expect_true(bool cond, const std::string& msg) {
  if (!cond) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << "\n";
  }
}

bool has_token(const pluss::Note& toks, const std::string& needle) {
  for (std::int64_t i = 0; i < toks.numel(); ++i) {
    if (toks.at(i) == needle) return true;
  }
  return false;
}

double as_score(const pluss::Note& n, std::int64_t i = 0) {
  return std::stod(std::string(n.at(i)));
}

void test_basic_extracts() {
  std::cout << "[RUN] coden:tex_extract\n";
  pluss::Note tex = pluss::content(
      "\\section{Intro}\n"
      "\\begin{equation}E=mc^2\\end{equation}\n"
      "See \\ref{eq:1}\n");
  auto refs = tex.tex_refs();
  expect_true(has_token(refs.tokens(), "eq:1"), "tex_refs extracts reference key");

  auto cmds = tex.tex_commands();
  expect_true(has_token(cmds.tokens(), "\\section"), "tex_commands extracts command tokens");
}

void test_nlp_extracts_and_filters() {
  std::cout << "[RUN] coden:nlp_extract\n";
  pluss::Note n = pluss::content("OpenAI GPT NASA shipped in 2025. Visit https://openai.com");

  auto acr = n.nlp_extract_acronyms();
  expect_true(has_token(acr.tokens(), "GPT"), "nlp_extract_acronyms finds GPT");
  expect_true(has_token(acr.tokens(), "NASA"), "nlp_extract_acronyms finds NASA");

  auto urls = n.nlp_urls();
  expect_true(has_token(urls.tokens(), "https://openai.com"), "nlp_urls finds URL");

  auto filtered = n.nlp_filter_tokens_len_ge(4);
  expect_true(filtered.tokens().numel() > 0, "nlp_filter_tokens_len_ge returns tokens");
}

void test_diff_align_family() {
  std::cout << "[RUN] coden:diff_align\n";
  pluss::Note a = pluss::content("alpha beta gamma");
  pluss::Note b = pluss::content("alpha theta gamma delta");

  auto dw = a.diff_words(b);
  expect_true(has_token(dw.tokens(), "-beta"), "diff_words marks removed tokens");
  expect_true(has_token(dw.tokens(), "+theta"), "diff_words marks inserted tokens");
  expect_true(has_token(dw.tokens(), "+delta"), "diff_words captures tail insertions");

  auto at = a.align_tokens(b);
  expect_true(has_token(at.tokens(), "=alpha"), "align_tokens keeps equal tokens");
  expect_true(has_token(at.tokens(), "=gamma"), "align_tokens keeps equal suffix");

  pluss::Note l1 = pluss::content("a\nb\nc");
  pluss::Note l2 = pluss::content("a\nx\nc");
  auto dl = l1.diff_lines(l2);
  expect_true(has_token(dl.tokens(), "-b"), "diff_lines marks removed line");
  expect_true(has_token(dl.tokens(), "+x"), "diff_lines marks inserted line");
}

void test_pairwise_set_ops_and_fast_paths() {
  std::cout << "[RUN] coden:pairwise_setops_fast\n";
  pluss::Note a = pluss::content("Alpha beta beta delta");
  pluss::Note b = pluss::content("beta theta DELTA");

  auto inter = a.intersect_tokens(b);
  expect_true(has_token(inter.tokens(), "beta"), "intersect_tokens keeps shared token");
  expect_true(has_token(inter.tokens(), "delta"), "intersect_tokens lower-normalizes shared token");
  expect_true(inter.tokens().numel() == 2, "intersect_tokens is unique-set intersection");

  auto jac = a.jaccard_tokens(b);
  const double jac_v = as_score(jac);
  expect_true(jac_v > 0.49 && jac_v < 0.51, "jaccard_tokens ~= 0.5");

  auto ov = a.overlap_tokens(b);
  const double ov_v = as_score(ov);
  expect_true(ov_v > 0.66 && ov_v < 0.67, "overlap_tokens ~= 2/3");

  pluss::Note e = pluss::content("");
  expect_true(as_score(e.jaccard_tokens(e)) == 1.0, "jaccard_tokens empty-empty is 1");
  expect_true(as_score(e.overlap_tokens(e)) == 1.0, "overlap_tokens empty-empty is 1");
  expect_true(as_score(e.overlap_tokens(a)) == 0.0, "overlap_tokens empty-vs-nonempty is 0");

  pluss::Note t1 = pluss::content("The quantum method is robust and scalable.");
  pluss::Note t2 = pluss::content("A scalable quantum pipeline improves method quality.");
  auto inter_terms = t1.intersect_terms(t2);
  expect_true(has_token(inter_terms.tokens(), "quantum"), "intersect_terms captures shared term");
  expect_true(has_token(inter_terms.tokens(), "scalable"), "intersect_terms captures second shared term");

  const std::array<std::int64_t, 1> sz{2};
  pluss::Note docs = pluss::content(
      std::vector<std::string>{"Alpha beta gamma", "One, two; three!!"},
      std::span<const std::int64_t>(sz.data(), sz.size()));
  pluss::Note scalar = pluss::content("beta two");

  auto inter_b = docs.intersect_tokens(scalar);
  expect_true(inter_b.offsets().size() == 3, "intersect_tokens broadcast preserves row offsets");
  expect_true(inter_b.offsets()[1] >= 1, "intersect_tokens broadcast row0 has intersection");
  expect_true(inter_b.offsets()[2] >= inter_b.offsets()[1], "intersect_tokens broadcast row1 offset monotonic");

  auto punct = docs.split_on_punct();
  expect_true(has_token(punct.tokens(), "One"), "split_on_punct extracts first token");
  expect_true(has_token(punct.tokens(), "three"), "split_on_punct extracts trailing token");

  pluss::Note words = pluss::content("foo_bar don't C3PO");
  auto w = words.tokenize_words();
  expect_true(has_token(w.tokens(), "foo_bar"), "tokenize_words keeps underscore");
  expect_true(has_token(w.tokens(), "don't"), "tokenize_words keeps apostrophe in body");
  expect_true(has_token(w.tokens(), "C3PO"), "tokenize_words keeps alnum word");

  pluss::Note sent = pluss::content("Hello!!! Next one? version2.0 works.");
  auto sents = sent.split_sentences();
  expect_true(has_token(sents.tokens(), "Hello!!!"), "split_sentences keeps punctuation runs");
  expect_true(has_token(sents.tokens(), "Next one?"), "split_sentences finds second sentence");
  expect_true(has_token(sents.tokens(), "version2.0 works."), "split_sentences does not split decimal");

  pluss::Note p1 = pluss::content("NASA GPT and [12] use x+y with \\alpha.");
  pluss::Note p2 = pluss::content("GPT NASA cite [12] and y/x with \\alpha and +.");
  auto ia = p1.intersect_acronyms(p2);
  expect_true(has_token(ia.tokens(), "nasa"), "intersect_acronyms captures shared acronym");
  expect_true(has_token(ia.tokens(), "gpt"), "intersect_acronyms captures second acronym");
  expect_true(as_score(p1.jaccard_acronyms(p2)) > 0.99, "jaccard_acronyms ~ 1");
  expect_true(as_score(p1.overlap_acronyms(p2)) > 0.99, "overlap_acronyms ~ 1");

  auto ic = p1.intersect_citations(p2);
  expect_true(has_token(ic.tokens(), "[12]"), "intersect_citations captures citation");
  expect_true(as_score(p1.jaccard_citations(p2)) > 0.99, "jaccard_citations ~ 1");

  auto im = p1.intersect_math_symbols(p2);
  expect_true(has_token(im.tokens(), "\\alpha"), "intersect_math_symbols captures macro");
  expect_true(has_token(im.tokens(), "+"), "intersect_math_symbols captures operator");
  expect_true(as_score(p1.overlap_math_symbols(p2)) > 0.4, "overlap_math_symbols positive");
}

void test_encoding_and_shape_ops() {
  std::cout << "[RUN] coden:encoding_shape\n";
  pluss::Note s = pluss::content("Hi!");
  expect_true(s.base64_encode().at(0) == "SGkh", "base64_encode encodes bytes");
  expect_true(s.base64_encode().base64_decode().at(0) == "Hi!", "base64_decode reverses encode");
  expect_true(s.hex_encode().at(0) == "486921", "hex_encode encodes bytes");
  expect_true(s.hex_encode().hex_decode().at(0) == "Hi!", "hex_decode reverses encode");

  pluss::Note u = pluss::content("a b+c");
  expect_true(u.url_encode().at(0) == "a%20b%2Bc", "url_encode encodes reserved chars");
  expect_true(u.url_encode().url_decode().at(0) == "a b+c", "url_decode reverses encode");

  pluss::Note h = pluss::content("<a&\"'>");
  expect_true(h.html_escape().at(0) == "&lt;a&amp;&quot;&#39;&gt;", "html_escape encodes entities");
  expect_true(h.html_escape().html_unescape().at(0) == "<a&\"'>", "html_unescape decodes entities");

  pluss::Note j = pluss::content("a\n\"b\"");
  expect_true(j.json_escape().at(0) == "a\\n\\\"b\\\"", "json_escape emits escaped control chars");

  const std::array<std::int64_t, 1> v_sizes{3};
  pluss::Note v = pluss::content(
      std::vector<std::string>{"b", "a", "b"},
      std::span<const std::int64_t>(v_sizes.data(), v_sizes.size()));
  auto sorted = v.sort();
  expect_true(sorted.at(0) == "a" && sorted.at(1) == "b" && sorted.at(2) == "b", "sort orders lexicographically");
  auto sorted_dim0 = v.sort(0);
  expect_true(sorted_dim0.at(0) == "a" && sorted_dim0.at(2) == "b", "sort(0) sorts along dim 0");
  auto uniq_sorted = v.unique();
  expect_true(uniq_sorted.numel() == 2 && uniq_sorted.at(0) == "a" && uniq_sorted.at(1) == "b", "unique() sorted distinct");
  auto dedup = v.deduplicate();
  expect_true(dedup.numel() == 2 && dedup.at(0) == "b" && dedup.at(1) == "a", "deduplicate() preserves first-seen");

  const std::array<std::int64_t, 2> m_sizes{2, 2};
  pluss::Note m = pluss::content(
      std::vector<std::string>{"x", "y", "z", "w"},
      std::span<const std::int64_t>(m_sizes.data(), m_sizes.size()));
  auto flat = m.flatten();
  expect_true(flat.dim() == 1 && flat.size(0) == 4, "flatten converts to 1D");
  auto ex = m.expand_dims(1);
  expect_true(ex.dim() == 3 && ex.size(0) == 2 && ex.size(1) == 1 && ex.size(2) == 2, "expand_dims inserts singleton axis");
  auto sq = ex.squeeze(1);
  expect_true(sq.dim() == 2 && sq.size(0) == 2 && sq.size(1) == 2, "squeeze removes singleton axis");

  const std::array<std::int64_t, 1> reps2{2};
  auto rep = v.repeat(std::span<const std::int64_t>(reps2.data(), reps2.size()));
  expect_true(rep.dim() == 1 && rep.size(0) == 6, "repeat expands 1D length");
  expect_true(rep.at(0) == "b" && rep.at(3) == "b", "repeat preserves chunk ordering");
  auto ri = v.repeat_interleave(2, 0);
  expect_true(ri.size(0) == 6 && ri.at(0) == "b" && ri.at(1) == "b" && ri.at(2) == "a", "repeat_interleave duplicates along dim");
  auto til = v.tile(std::span<const std::int64_t>(reps2.data(), reps2.size()));
  expect_true(til.size(0) == 6 && til.at(4) == "a", "tile aliases repeat semantics");
}

void test_high_yield_nlp_and_printing() {
  std::cout << "[RUN] coden:high_yield_nlp_print\n";
  pluss::Note n = pluss::content("I can't believe this GREAT product costs 123 dollars in 2026.");

  const std::string expanded = std::string(n.nlp_expand_contractions().at(0));
  expect_true(expanded.find("can not") != std::string::npos, "nlp_expand_contractions expands can't");

  const std::string normn = std::string(n.nlp_normalize_numbers().at(0));
  expect_true(normn.find("0 dollars in 0") != std::string::npos, "nlp_normalize_numbers masks numeric spans");

  const double sent = as_score(n.nlp_sentiment());
  expect_true(sent > 0.5, "nlp_sentiment positive for positive lexicon");
  const double tox = as_score(n.nlp_toxicity());
  expect_true(tox < 0.4, "nlp_toxicity stays low for neutral sentence");
  expect_true(!std::string(n.nlp_readability_level().at(0)).empty(), "nlp_readability_level emits label");
  expect_true(!std::string(n.nlp_script().at(0)).empty(), "nlp_script emits script");

  pluss::Note sw = pluss::content("the robust method is in the pipeline");
  const std::string swr = std::string(sw.nlp_remove_stopwords().at(0));
  expect_true(swr.find("the") == std::string::npos, "nlp_remove_stopwords removes english stopwords");

  const std::string tr = std::string(pluss::content("São").transliterate_ascii().at(0));
  expect_true(tr == "Sao", "transliterate_ascii strips accent");
  expect_true(!std::string(pluss::content("Robert").phonetic_soundex().at(0)).empty(), "phonetic_soundex emits code");

  std::string latin1 = "caf\xE9";
  auto decoded = pluss::bytes_content(latin1).from_latin1();
  expect_true(decoded.at(0) == "café", "from_latin1 decodes bytes to UTF-8");
  auto reenc = decoded.to_latin1_lossy();
  expect_true(static_cast<const pluss::coden::NoteBase&>(reenc).is_bytes(),
              "to_latin1_lossy switches encoding to bytes");
  expect_true(reenc.at(0) == latin1, "to_latin1_lossy round-trips basic latin1 text");

  pluss::NotePrintOptions opt;
  opt.max_elements = 6;
  opt.edge_items = 1;
  const std::array<std::int64_t, 2> sizes{2, 6};
  pluss::Note m = pluss::content(
      std::vector<std::string>{
          "alpha", "beta", "gamma", "delta", "epsilon", "zeta",
          "line1\nline2", "emoji 🙂", "quoted \"x\"", "kappa", "lambda", "mu"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const std::string printed = pluss::print_note(m, opt);
  expect_true(printed.find("Note(") != std::string::npos, "print_note includes metadata");
  expect_true(printed.find("shape=[2, 6]") != std::string::npos, "print_note includes shape");
  expect_true(printed.find("...") != std::string::npos, "print_note summarizes large notes");

  pluss::NotePrintOptions hex_opt;
  hex_opt.bytes_mode = pluss::coden::NotePrintBytesMode::Hex;
  const std::string bytes_print = pluss::print_note(pluss::bytes_content("Hi"), hex_opt);
  expect_true(bytes_print.find("hex[48 69]") != std::string::npos, "print_note hex mode prints byte hex");

  pluss::Note tex = pluss::content(
      "\\section{Intro}\r\n\r\n"
      "\\textbf{Bold} and \\emph{emph}.\\n"
      "Math: $x+y$ and \\(z\\).\\n\\n\\n"
      "{\\bfseries font} line.");
  const std::string tex_norm = std::string(tex.tex_normalize_newlines().at(0));
  expect_true(tex_norm.find('\r') == std::string::npos, "tex_normalize_newlines removes CR");

  const std::string tex_dedup = std::string(tex.tex_dedup_blank_lines().at(0));
  expect_true(tex_dedup.find("\n\n\n") == std::string::npos, "tex_dedup_blank_lines collapses blank runs");

  const std::string escaped = std::string(pluss::content("a_b#%").tex_escape().at(0));
  expect_true(escaped.find("\\_") != std::string::npos, "tex_escape escapes underscore");
  expect_true(std::string(pluss::content(escaped).tex_unescape().at(0)) == "a_b#%", "tex_unescape reverses escapes");

  const std::string no_delims = std::string(tex.tex_remove_math_delimiters().at(0));
  expect_true(no_delims.find("$") == std::string::npos, "tex_remove_math_delimiters strips dollar delimiters");
  expect_true(std::string(pluss::content("x+y").tex_add_math_delimiters().at(0)) == "$x+y$",
              "tex_add_math_delimiters wraps plain expression");

  const std::string math_only = std::string(tex.tex_math_only().at(0));
  expect_true(math_only.find("x+y") != std::string::npos, "tex_math_only extracts math payload");
  const std::string text_only = std::string(tex.tex_text_only().at(0));
  expect_true(text_only.find("Math:") != std::string::npos, "tex_text_only keeps prose payload");

  const std::string no_fmt = std::string(tex.tex_remove_formatting().at(0));
  expect_true(no_fmt.find("Bold") != std::string::npos, "tex_remove_formatting keeps formatted content");
  expect_true(no_fmt.find("\\textbf") == std::string::npos, "tex_remove_formatting drops formatting commands");

  const std::string md = std::string(tex.tex_to_markdown().at(0));
  expect_true(md.find("# Intro") != std::string::npos, "tex_to_markdown converts section to markdown heading");
  expect_true(md.find("**Bold**") != std::string::npos, "tex_to_markdown converts bold command");

  const std::string html = std::string(tex.tex_to_html().at(0));
  expect_true(html.find("<h1>Intro</h1>") != std::string::npos, "tex_to_html converts section heading");
  expect_true(html.find("<strong>Bold</strong>") != std::string::npos, "tex_to_html converts bold command");
}

void test_tex_canonicalize_and_macros() {
  std::cout << "[RUN] coden:tex_structural\n";
  pluss::Note t = pluss::content(
      "\\newcommand{\\RR}{R}\n"
      "Let $x \\in \\RR$ and \\dfrac{a}{b} + \\sqrt{c} + \\pm 1.\n"
      "Vector: \\mathbf{v}. Matrix: \\begin{bmatrix}1\\end{bmatrix}.\n"
      "Limits: \\sumlimits_{i=1}^n x_i and x^2 and x'' and \\ldots\n"
      "Accents: \\'{e}.");

  const std::string expanded = std::string(t.tex_expand_macros().at(0));
  expect_true(expanded.find("x \\in R") != std::string::npos, "tex_expand_macros expands \\RR");
  expect_true(expanded.find("\\RR") == std::string::npos, "tex_expand_macros removes macro usage");
  expect_true(expanded.find("\\newcommand") == std::string::npos, "tex_expand_macros strips macro decls");

  const std::string canon = std::string(t.tex_canonicalize_macros().at(0));
  expect_true(canon.find("\\frac") != std::string::npos, "tex_canonicalize_macros normalizes \\dfrac");

  const std::string frac = std::string(t.tex_simplify_fractions().at(0));
  expect_true(frac.find("\\frac{a}{b}") != std::string::npos, "tex_simplify_fractions normalizes frac");

  const std::string sqrtp = std::string(t.tex_convert_sqrt_to_pow().at(0));
  expect_true(sqrtp.find("^(1/2)") != std::string::npos, "tex_convert_sqrt_to_pow converts sqrt");

  const std::string pm = std::string(t.tex_convert_pm_to_plusminus().at(0));
  expect_true(pm.find("+/-") != std::string::npos, "tex_convert_pm_to_plusminus converts \\pm");

  const std::string vec = std::string(t.tex_standardize_vectors().at(0));
  expect_true(vec.find("\\vec{v}") != std::string::npos, "tex_standardize_vectors normalizes vector markup");

  const std::string mat = std::string(t.tex_standardize_matrices().at(0));
  expect_true(mat.find("\\begin{matrix}") != std::string::npos, "tex_standardize_matrices normalizes matrix env");

  const std::string sums = std::string(t.tex_standardize_summations().at(0));
  expect_true(sums.find("\\sumlimits") == std::string::npos, "tex_standardize_summations removes sumlimits");

  const std::string lims = std::string(t.tex_standardize_limits().at(0));
  expect_true(lims.find("\\limits") == std::string::npos, "tex_standardize_limits strips \\limits");

  const std::string subs = std::string(t.tex_standardize_subscripts().at(0));
  expect_true(subs.find("x_{i}") != std::string::npos, "tex_standardize_subscripts braces subscripts");

  const std::string sups = std::string(t.tex_standardize_superscripts().at(0));
  expect_true(sups.find("x^{2}") != std::string::npos, "tex_standardize_superscripts braces superscripts");

  const std::string primes = std::string(t.tex_standardize_primes().at(0));
  expect_true(primes.find("\\prime") != std::string::npos, "tex_standardize_primes expands primes");

  const std::string dots = std::string(t.tex_standardize_dots().at(0));
  expect_true(dots.find("\\dots") != std::string::npos, "tex_standardize_dots normalizes dots");

  const std::string acc = std::string(t.tex_remove_accents().at(0));
  expect_true(acc.find("e") != std::string::npos, "tex_remove_accents strips accent macros");

  const std::string setm = std::string(t.tex_set_macro("Foo", "Bar").at(0));
  expect_true(setm.find("\\newcommand{\\Foo}{Bar}") != std::string::npos, "tex_set_macro inserts macro decl");

  const std::string remm = std::string(t.tex_remove_macro("RR").at(0));
  expect_true(remm.find("\\newcommand") == std::string::npos, "tex_remove_macro removes macro decl");

  const std::string wrapped = std::string(pluss::content("x+y").tex_wrap_inline_math("$").at(0));
  expect_true(wrapped == "$x+y$", "tex_wrap_inline_math wraps inline math");

  const std::string display = std::string(pluss::content("x+y").tex_wrap_display_math("$$").at(0));
  expect_true(display.find("$$") != std::string::npos, "tex_wrap_display_math wraps display math");

  const std::string bal_br = std::string(pluss::content("{a{b}").tex_balance_braces().at(0));
  expect_true(bal_br.find("}}") != std::string::npos, "tex_balance_braces closes missing braces");

  const std::string bal_math = std::string(pluss::content("$x+y").tex_balance_math_delims().at(0));
  expect_true(!bal_math.empty() && bal_math.back() == '$', "tex_balance_math_delims closes missing $");

  const std::string inc = std::string(pluss::content("\\input{sec1}").tex_expand_inputs().at(0));
  expect_true(inc.find("[include:sec1]") != std::string::npos, "tex_expand_inputs rewrites inputs");

  const std::string gfx = std::string(pluss::content("\\includegraphics{fig}").tex_resolve_graphics().at(0));
  expect_true(gfx.find("[graphic:fig]") != std::string::npos, "tex_resolve_graphics rewrites graphics");
}

void test_note_factory_paths() {
  std::cout << "[RUN] coden:note_factories\n";

  pluss::Note s0 = pluss::content("hi");
  expect_true(s0.dim() == 0, "content(\"hi\") produces scalar note");

  pluss::Note s1 = pluss::content({"a", "b", "c"});
  expect_true(s1.dim() == 1 && s1.size(0) == 3, "content({\"a\",\"b\",\"c\"}) produces 1D note");

  pluss::Note s2 = pluss::content({{"a", "b"}, {"c", "d"}});
  expect_true(s2.dim() == 2 && s2.sizes() == std::vector<std::int64_t>({2, 2}), "content 2D nested list");

  pluss::Note s3 = pluss::content({{{"a"}, {"b"}}, {{"c"}, {"d"}}});
  expect_true(s3.dim() == 3 && s3.sizes() == std::vector<std::int64_t>({2, 2, 1}), "content 3D nested list");

  pluss::Note m2 = pluss::make_note({{"x", "y"}, {"z", "w"}});
  expect_true(m2.dim() == 2 && m2.at(0) == "x" && m2.at(3) == "w", "make_note 2D nested list");

  const std::array<std::int64_t, 2> sz{2, 2};
  std::vector<std::string> flat = {"p", "q", "r", "s"};
  pluss::Note mf = pluss::make_note(flat, std::span<const std::int64_t>(sz.data(), sz.size()));
  expect_true(mf.dim() == 2 && mf.at(2) == "r", "make_note(flat, sizes) constructs 2D");

  pluss::Note lines = pluss::note_from_lines("a\nb\nc");
  expect_true(lines.dim() == 1 && lines.size(0) == 3, "note_from_lines splits into 1D note");

  pluss::Note words = pluss::note_from_words("alpha beta");
  expect_true(words.dim() == 1 && words.size(0) == 2, "note_from_words splits words");

  pluss::Note full = pluss::full_note({2, 2, 1}, "x");
  expect_true(full.dim() == 3 && full.numel() == 4 && full.at(0) == "x", "full_note fills shape");

  pluss::Note empty = pluss::empty_note({2, 1, 2});
  expect_true(empty.dim() == 3 && empty.numel() == 4, "empty_note creates shape");

  pluss::Note like = pluss::full_note_like(s2, "y");
  expect_true(like.sizes() == s2.sizes(), "full_note_like copies shape");

  pluss::Note b0 = pluss::bytes_content("hi");
  expect_true(static_cast<const pluss::coden::NoteBase&>(b0).is_bytes(), "bytes_content uses bytes mode");

  pluss::Note b2 = pluss::bytes_content({{"\x01", "\x02"}, {"\x03", "\x04"}});
  expect_true(b2.dim() == 2 && b2.at(3).size() == 1, "bytes_content supports nested list");

  pluss::Note bfull = pluss::full_bytes_note_like(b2, "\xFF");
  expect_true(static_cast<const pluss::coden::NoteBase&>(bfull).is_bytes(), "full_bytes_note_like preserves bytes");

  pluss::Note lower = s3.lower();
  expect_true(lower.dim() == 3 && lower.sizes() == s3.sizes(), "methods preserve 3D shape");

  pluss::Note escaped = s3.tex_escape();
  expect_true(escaped.dim() == 3 && escaped.sizes() == s3.sizes(), "tex methods preserve 3D shape");
}

} // namespace

int main() {
  test_basic_extracts();
  test_nlp_extracts_and_filters();
  test_diff_align_family();
  test_pairwise_set_ops_and_fast_paths();
  test_encoding_and_shape_ops();
  test_high_yield_nlp_and_printing();
  test_tex_canonicalize_and_macros();
  test_note_factory_paths();

  if (g_failures == 0) {
    std::cout << "[OK] CodeN no-catch smoke tests passed\n";
    return 0;
  }
  std::cerr << "[FAIL] CodeN no-catch smoke tests failed: " << g_failures << "\n";
  return 1;
}
