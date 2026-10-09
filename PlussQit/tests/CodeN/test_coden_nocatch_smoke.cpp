#include <pluss/pluss.h>
#include <pluss/tils.h>
#include "CodeN/native/cpu/TextKernels.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <sstream>
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
  // Numeric-score methods (nlp_sentiment, jaccard_*, overlap_*, ...) return
  // a native Float64 Note (pass 8 cleanup), not a formatted Text Note.
  return n.reals()[static_cast<std::size_t>(i)];
}

bool notes_equal_with_diag(const pluss::Note& a, const pluss::Note& b, std::string& diag) {
  if (a.defined() != b.defined()) {
    diag = "defined() mismatch";
    return false;
  }
  if (!a.defined()) return true;
  if (a.dim() != b.dim()) {
    diag = "dim mismatch";
    return false;
  }
  if (a.sizes() != b.sizes()) {
    diag = "sizes mismatch";
    return false;
  }
  if (a.numel() != b.numel()) {
    diag = "numel mismatch";
    return false;
  }
  for (std::int64_t i = 0; i < a.numel(); ++i) {
    if (a.at(i) != b.at(i)) {
      std::ostringstream oss;
      oss << "first mismatch at i=" << i
          << " lhs=\"" << std::string(a.at(i)) << "\""
          << " rhs=\"" << std::string(b.at(i)) << "\"";
      diag = oss.str();
      return false;
    }
  }
  diag.clear();
  return true;
}

bool token_batches_equal_with_diag(const pluss::TokenBatch& a,
                                   const pluss::TokenBatch& b,
                                   std::string& diag) {
  const auto a_off = a.offsets().integers();
  const auto b_off = b.offsets().integers();
  if (!std::equal(a_off.begin(), a_off.end(), b_off.begin(), b_off.end())) {
    diag = "offset mismatch";
    return false;
  }
  return notes_equal_with_diag(a.tokens(), b.tokens(), diag);
}

void set_coden_encode_fastpath_mode(const char* mode) {
#if defined(_WIN32)
  _putenv_s("PLUSS_CODEN_ENCODE_FASTPATH", (mode == nullptr) ? "" : mode);
#else
  if (mode == nullptr || mode[0] == '\0') {
    unsetenv("PLUSS_CODEN_ENCODE_FASTPATH");
  } else {
    setenv("PLUSS_CODEN_ENCODE_FASTPATH", mode, 1);
  }
#endif
}

void set_env_mode(const char* key, const char* mode) {
#if defined(_WIN32)
  _putenv_s(key, (mode == nullptr) ? "" : mode);
#else
  if (mode == nullptr || mode[0] == '\0') {
    unsetenv(key);
  } else {
    setenv(key, mode, 1);
  }
#endif
}

void test_basic_extracts() {
  std::cout << "[RUN] coden:tex_extract\n";
  pluss::Note tex = pluss::make_note(
      "\\section{Intro}\n"
      "\\begin{equation}E=mc^2\\end{equation}\n"
      "See \\ref{eq:1}\n");
  auto refs = tex.tex().refs();
  expect_true(has_token(refs.tokens(), "eq:1"), "tex_refs extracts reference key");

  auto cmds = tex.tex().commands();
  expect_true(has_token(cmds.tokens(), "\\section"), "tex_commands extracts command tokens");
}

void test_text_kernels_compare_hash() {
  std::cout << "[RUN] coden:text_kernels_compare_hash\n";
  using op = pluss::coden::native::cpu::LexCompareOp;
  namespace tk = pluss::coden::native::cpu;

  expect_true(tk::compare_lex_bytes("alpha", "alpha", op::Eq), "compare_lex_bytes Eq true");
  expect_true(!tk::compare_lex_bytes("alpha", "beta", op::Eq), "compare_lex_bytes Eq false");
  expect_true(tk::compare_lex_bytes("naïve", "東京", op::Lt), "compare_lex_bytes Lt utf8 lexical");
  expect_true(tk::compare_lex_bytes("ßeta", "ßeta", op::Ge), "compare_lex_bytes Ge equality");
  expect_true(tk::compare_lex_bytes("東京", "naïve", op::Gt), "compare_lex_bytes Gt utf8 lexical");

  const std::array<std::string_view, 4> lhs{"a", "beta", "東京", "ßeta"};
  const std::array<std::string_view, 4> rhs{"a", "alpha", "東京", "zeta"};
  std::array<std::uint8_t, 4> out{};
  tk::compare_lex_bytes_batch(
      std::span<const std::string_view>(lhs.data(), lhs.size()),
      std::span<const std::string_view>(rhs.data(), rhs.size()),
      op::Gt,
      std::span<std::uint8_t>(out.data(), out.size()));
  expect_true(out[0] == 0 && out[1] == 1 && out[2] == 0 && out[3] == 1,
              "compare_lex_bytes_batch applies lexical op element-wise");

  const auto h0 = tk::hash_bytes_fnv1a("alpha");
  const auto h1 = tk::hash_bytes_fnv1a("alpha");
  const auto h2 = tk::hash_bytes_fnv1a("beta");
  expect_true(h0 == h1, "hash_bytes_fnv1a deterministic for equal inputs");
  expect_true(h0 != h2, "hash_bytes_fnv1a differentiates common unequal inputs");

  std::array<std::uint64_t, 4> hb{};
  tk::hash_bytes_fnv1a_batch(
      std::span<const std::string_view>(lhs.data(), lhs.size()),
      std::span<std::uint64_t>(hb.data(), hb.size()));
  expect_true(hb[0] == tk::hash_bytes_fnv1a(lhs[0]) &&
                  hb[1] == tk::hash_bytes_fnv1a(lhs[1]) &&
                  hb[2] == tk::hash_bytes_fnv1a(lhs[2]) &&
                  hb[3] == tk::hash_bytes_fnv1a(lhs[3]),
              "hash_bytes_fnv1a_batch matches scalar hash kernel");
}

void test_encoded_fastpath_parity() {
  std::cout << "[RUN] coden:encoded_fastpath_parity\n";
  const std::array<std::int64_t, 1> shape{12};
  const std::vector<std::string> values{
      "alpha", "beta",  "alpha", "東京", "beta", "東京",
      "naïve", "alpha", "ßeta",  "beta", "ßeta", "alpha"};
  const auto note = pluss::make_note(values, std::span<const std::int64_t>(shape.data(), shape.size()));

  set_coden_encode_fastpath_mode("0");
  const auto uniq_off = note.unique();
  const auto dedup_off = note.deduplicate();

  set_coden_encode_fastpath_mode("1");
  const auto uniq_on = note.unique();
  const auto dedup_on = note.deduplicate();

  set_coden_encode_fastpath_mode(nullptr);

  expect_true(uniq_off.numel() == uniq_on.numel(), "encoded parity: unique numel match");
  for (std::int64_t i = 0; i < uniq_off.numel(); ++i) {
    expect_true(uniq_off.at(i) == uniq_on.at(i), "encoded parity: unique values match");
  }

  expect_true(dedup_off.numel() == dedup_on.numel(), "encoded parity: deduplicate numel match");
  for (std::int64_t i = 0; i < dedup_off.numel(); ++i) {
    expect_true(dedup_off.at(i) == dedup_on.at(i), "encoded parity: deduplicate values match");
  }
}

void test_nlp_extracts_and_filters() {
  std::cout << "[RUN] coden:nlp_extract\n";
  pluss::Note n = pluss::make_note("OpenAI GPT NASA shipped in 2025. Visit https://openai.com");

  auto acr = n.nlp().acronyms();
  expect_true(has_token(acr.tokens(), "GPT"), "nlp_extract_acronyms finds GPT");
  expect_true(has_token(acr.tokens(), "NASA"), "nlp_extract_acronyms finds NASA");

  auto urls = n.nlp().urls();
  expect_true(has_token(urls.tokens(), "https://openai.com"), "nlp_urls finds URL");

  auto filtered = n.nlp().min_length(4);
  expect_true(filtered.tokens().numel() > 0, "nlp_filter_tokens_len_ge returns tokens");
}

void test_nlp_keyword_frequency_order() {
  std::cout << "[RUN] coden:nlp_keyword_frequency\n";
  const std::array<std::int64_t, 1> shape{2};
  const auto note = pluss::make_note(
      std::vector<std::string>{
          "beta alpha beta gamma alpha alpha the and beta",
          "omega omega zeta zeta and the"},
      std::span<const std::int64_t>(shape.data(), shape.size()));

  const auto kw = note.nlp().keywords(3);
  const auto kw_offsets = kw.offsets().integers();
  expect_true(kw_offsets.size() == 3, "nlp_keywords returns row offsets");
  expect_true(kw_offsets[0] == 0 && kw_offsets[1] == 3 && kw_offsets[2] == 5,
              "nlp_keywords offsets match per-row top-k lengths");

  expect_true(kw.tokens().at(0) == "alpha" && kw.tokens().at(1) == "beta" && kw.tokens().at(2) == "gamma",
              "nlp_keywords row0 ranks by frequency then lexical tie-break");
  expect_true(kw.tokens().at(3) == "omega" && kw.tokens().at(4) == "zeta",
              "nlp_keywords row1 removes stopwords and ranks ties lexically");

  const auto stop_only = pluss::make_note("the and in to with");
  const auto kw_stop_only = stop_only.nlp().keywords(5);
  const auto kw_stop_only_offsets = kw_stop_only.offsets().integers();
  expect_true(kw_stop_only_offsets.size() == 2 && kw_stop_only_offsets[0] == 0 && kw_stop_only_offsets[1] == 0,
              "nlp_keywords stopword-only row produces empty token range");
  expect_true(kw_stop_only.tokens().numel() == 0, "nlp_keywords stopword-only row returns no keywords");
}

void test_parallel_lane_parity() {
  std::cout << "[RUN] coden:parallel_lane_parity\n";

  std::vector<std::string> rows;
  rows.reserve(32768);
  for (int i = 0; i < 32768; ++i) {
    rows.push_back((i % 2 == 0) ? "beta alpha beta gamma alpha alpha the and beta"
                                : "omega omega zeta zeta and the");
  }
  const std::array<std::int64_t, 2> shape{512, 64};
  const auto note2d = pluss::make_note(rows, std::span<const std::int64_t>(shape.data(), shape.size()));

  set_env_mode("PLUSS_CODEN_PARALLEL", "off");
  const auto sorted_off = note2d.sort();
  const auto kw_off = note2d.nlp().keywords(3);

  set_env_mode("PLUSS_CODEN_PARALLEL", "on");
  const auto sorted_on = note2d.sort();
  const auto kw_on = note2d.nlp().keywords(3);

  set_env_mode("PLUSS_CODEN_PARALLEL", nullptr);

  bool same_sort = (sorted_off.numel() == sorted_on.numel());
  for (std::int64_t i = 0; same_sort && i < sorted_off.numel(); ++i) {
    if (sorted_off.at(i) != sorted_on.at(i)) same_sort = false;
  }
  expect_true(same_sort, "parallel on/off sort parity is exact");

  const auto kw_off_offsets = kw_off.offsets().integers();
  const auto kw_on_offsets = kw_on.offsets().integers();
  bool same_kw = std::equal(kw_off_offsets.begin(), kw_off_offsets.end(),
                            kw_on_offsets.begin(), kw_on_offsets.end()) &&
                 (kw_off.tokens().numel() == kw_on.tokens().numel());
  for (std::int64_t i = 0; same_kw && i < kw_off.tokens().numel(); ++i) {
    if (kw_off.tokens().at(i) != kw_on.tokens().at(i)) same_kw = false;
  }
  expect_true(same_kw, "parallel on/off tokenbatch parity is exact");
}

void test_reference_mode_parity() {
  std::cout << "[RUN] coden:reference_mode_parity\n";

  std::vector<std::string> rows;
  rows.reserve(32768);
  for (int i = 0; i < 32768; ++i) {
    rows.push_back((i % 2 == 0) ? "beta alpha beta gamma alpha alpha the and beta"
                                : "omega omega zeta zeta and the");
  }
  const std::array<std::int64_t, 2> shape2d{512, 64};
  const auto note2d = pluss::make_note(rows, std::span<const std::int64_t>(shape2d.data(), shape2d.size()));
  const std::array<std::int64_t, 1> shape1d{32768};
  const auto note1d = pluss::make_note(rows, std::span<const std::int64_t>(shape1d.data(), shape1d.size()));
  const auto scalar = pluss::make_note("alpha omega");

  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", "reference");
  set_env_mode("PLUSS_CODEN_PARALLEL", "on");
  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", "force");
  set_env_mode("PLUSS_CODEN_COUNT_FASTPATH", "on");
  const auto sort_ref = note2d.sort();
  const auto unique_ref = note1d.unique();
  const auto dedup_ref = note1d.deduplicate();
  const auto kw_ref = note2d.nlp().keywords(3);
  const auto inter_ref = note2d.nlp().intersect_tokens(scalar);

  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", "optimized");
  const auto sort_opt = note2d.sort();
  const auto unique_opt = note1d.unique();
  const auto dedup_opt = note1d.deduplicate();
  const auto kw_opt = note2d.nlp().keywords(3);
  const auto inter_opt = note2d.nlp().intersect_tokens(scalar);

  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", nullptr);
  set_env_mode("PLUSS_CODEN_PARALLEL", nullptr);
  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", nullptr);
  set_env_mode("PLUSS_CODEN_COUNT_FASTPATH", nullptr);

  std::string diag;
  expect_true(notes_equal_with_diag(sort_ref, sort_opt, diag), "reference parity sort: " + diag);
  expect_true(notes_equal_with_diag(unique_ref, unique_opt, diag), "reference parity unique: " + diag);
  expect_true(notes_equal_with_diag(dedup_ref, dedup_opt, diag), "reference parity deduplicate: " + diag);
  expect_true(token_batches_equal_with_diag(kw_ref, kw_opt, diag), "reference parity nlp_keywords: " + diag);
  expect_true(token_batches_equal_with_diag(inter_ref, inter_opt, diag), "reference parity intersect_tokens: " + diag);
}

void test_diff_align_family() {
  std::cout << "[RUN] coden:diff_align\n";
  pluss::Note a = pluss::make_note("alpha beta gamma");
  pluss::Note b = pluss::make_note("alpha theta gamma delta");

  auto dw = a.nlp().diff_words(b);
  expect_true(has_token(dw.tokens(), "-beta"), "diff_words marks removed tokens");
  expect_true(has_token(dw.tokens(), "+theta"), "diff_words marks inserted tokens");
  expect_true(has_token(dw.tokens(), "+delta"), "diff_words captures tail insertions");

  auto at = a.nlp().align_tokens(b);
  expect_true(has_token(at.tokens(), "=alpha"), "align_tokens keeps equal tokens");
  expect_true(has_token(at.tokens(), "=gamma"), "align_tokens keeps equal suffix");

  pluss::Note l1 = pluss::make_note("a\nb\nc");
  pluss::Note l2 = pluss::make_note("a\nx\nc");
  auto dl = l1.diff_lines(l2);
  expect_true(has_token(dl.tokens(), "-b"), "diff_lines marks removed line");
  expect_true(has_token(dl.tokens(), "+x"), "diff_lines marks inserted line");
}

void test_pairwise_set_ops_and_fast_paths() {
  std::cout << "[RUN] coden:pairwise_setops_fast\n";
  pluss::Note a = pluss::make_note("Alpha beta beta delta");
  pluss::Note b = pluss::make_note("beta theta DELTA");

  auto inter = a.nlp().intersect_tokens(b);
  expect_true(has_token(inter.tokens(), "beta"), "intersect_tokens keeps shared token");
  expect_true(has_token(inter.tokens(), "delta"), "intersect_tokens lower-normalizes shared token");
  expect_true(inter.tokens().numel() == 2, "intersect_tokens is unique-set intersection");

  auto jac = a.nlp().jaccard_tokens(b);
  const double jac_v = as_score(jac);
  expect_true(jac_v > 0.49 && jac_v < 0.51, "jaccard_tokens ~= 0.5");

  auto ov = a.nlp().overlap_tokens(b);
  const double ov_v = as_score(ov);
  expect_true(ov_v > 0.66 && ov_v < 0.67, "overlap_tokens ~= 2/3");

  pluss::Note e = pluss::make_note("");
  expect_true(as_score(e.nlp().jaccard_tokens(e)) == 1.0, "jaccard_tokens empty-empty is 1");
  expect_true(as_score(e.nlp().overlap_tokens(e)) == 1.0, "overlap_tokens empty-empty is 1");
  expect_true(as_score(e.nlp().overlap_tokens(a)) == 0.0, "overlap_tokens empty-vs-nonempty is 0");

  pluss::Note t1 = pluss::make_note("The quantum method is robust and scalable.");
  pluss::Note t2 = pluss::make_note("A scalable quantum pipeline improves method quality.");
  auto inter_terms = t1.nlp().intersect_terms(t2);
  expect_true(has_token(inter_terms.tokens(), "quantum"), "intersect_terms captures shared term");
  expect_true(has_token(inter_terms.tokens(), "scalable"), "intersect_terms captures second shared term");

  const std::array<std::int64_t, 1> sz{2};
  pluss::Note docs = pluss::make_note(
      std::vector<std::string>{"Alpha beta gamma", "One, two; three!!"},
      std::span<const std::int64_t>(sz.data(), sz.size()));
  pluss::Note scalar = pluss::make_note("beta two");

  auto inter_b = docs.nlp().intersect_tokens(scalar);
  const auto inter_b_offsets = inter_b.offsets().integers();
  expect_true(inter_b_offsets.size() == 3, "intersect_tokens broadcast preserves row offsets");
  expect_true(inter_b_offsets[1] >= 1, "intersect_tokens broadcast row0 has intersection");
  expect_true(inter_b_offsets[2] >= inter_b_offsets[1], "intersect_tokens broadcast row1 offset monotonic");

  auto punct = docs.nlp().punct_tokens();
  expect_true(has_token(punct.tokens(), ","), "punct_tokens extracts comma");
  expect_true(has_token(punct.tokens(), "!"), "punct_tokens extracts exclamation marks");

  pluss::Note words = pluss::make_note("foo_bar don't C3PO");
  auto w = words.nlp().tokens();
  expect_true(has_token(w.tokens(), "foo_bar"), "tokenize_words keeps underscore");
  expect_true(has_token(w.tokens(), "don't"), "tokenize_words keeps apostrophe in body");
  expect_true(has_token(w.tokens(), "C3PO"), "tokenize_words keeps alnum word");

  pluss::Note sent = pluss::make_note("Hello!!! Next one? version2.0 works.");
  auto sents = sent.nlp().sentences();
  expect_true(has_token(sents.tokens(), "Hello!!!"), "split_sentences keeps punctuation runs");
  expect_true(has_token(sents.tokens(), "Next one?"), "split_sentences finds second sentence");
  expect_true(has_token(sents.tokens(), "version2.0 works."), "split_sentences does not split decimal");

  pluss::Note p1 = pluss::make_note("NASA GPT and [12] use x+y with \\alpha.");
  pluss::Note p2 = pluss::make_note("GPT NASA cite [12] and y/x with \\alpha and +.");
  auto ia = p1.nlp().intersect_acronyms(p2);
  expect_true(has_token(ia.tokens(), "nasa"), "intersect_acronyms captures shared acronym");
  expect_true(has_token(ia.tokens(), "gpt"), "intersect_acronyms captures second acronym");
  expect_true(as_score(p1.nlp().jaccard_acronyms(p2)) > 0.99, "jaccard_acronyms ~ 1");
  expect_true(as_score(p1.nlp().overlap_acronyms(p2)) > 0.99, "overlap_acronyms ~ 1");

  auto ic = p1.nlp().intersect_citations(p2);
  expect_true(has_token(ic.tokens(), "[12]"), "intersect_citations captures citation");
  expect_true(as_score(p1.nlp().jaccard_citations(p2)) > 0.99, "jaccard_citations ~ 1");

  auto im = p1.nlp().intersect_symbols(p2);
  expect_true(has_token(im.tokens(), "\\alpha"), "intersect_math_symbols captures macro");
  expect_true(has_token(im.tokens(), "+"), "intersect_math_symbols captures operator");
  expect_true(as_score(p1.nlp().overlap_symbols(p2)) > 0.4, "overlap_math_symbols positive");
}

void test_encoding_and_shape_ops() {
  std::cout << "[RUN] coden:encoding_shape\n";
  pluss::Note s = pluss::make_note("Hi!");
  expect_true(s.base64_encode().at(0) == "SGkh", "base64_encode encodes bytes");
  expect_true(s.base64_encode().base64_decode().at(0) == "Hi!", "base64_decode reverses encode");
  expect_true(s.hex_encode().at(0) == "486921", "hex_encode encodes bytes");
  expect_true(s.hex_encode().hex_decode().at(0) == "Hi!", "hex_decode reverses encode");

  pluss::Note u = pluss::make_note("a b+c");
  expect_true(u.url_encode().at(0) == "a%20b%2Bc", "url_encode encodes reserved chars");
  expect_true(u.url_encode().url_decode().at(0) == "a b+c", "url_decode reverses encode");

  pluss::Note h = pluss::make_note("<a&\"'>");
  expect_true(h.html_escape().at(0) == "&lt;a&amp;&quot;&#39;&gt;", "html_escape encodes entities");
  expect_true(h.html_escape().html_unescape().at(0) == "<a&\"'>", "html_unescape decodes entities");

  pluss::Note j = pluss::make_note("a\n\"b\"");
  expect_true(j.json_escape().at(0) == "a\\n\\\"b\\\"", "json_escape emits escaped control chars");

  const std::array<std::int64_t, 1> v_sizes{3};
  pluss::Note v = pluss::make_note(
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
  pluss::Note m = pluss::make_note(
      std::vector<std::string>{"x", "y", "z", "w"},
      std::span<const std::int64_t>(m_sizes.data(), m_sizes.size()));
  auto flat = m.flatten();
  expect_true(flat.dim() == 1 && flat.size(0) == 4, "flatten converts to 1D");
  auto ex = m.unsqueeze(1);
  expect_true(ex.dim() == 3 && ex.size(0) == 2 && ex.size(1) == 1 && ex.size(2) == 2, "expand_dims inserts singleton axis");
  auto sq = ex.squeeze(1);
  expect_true(sq.dim() == 2 && sq.size(0) == 2 && sq.size(1) == 2, "squeeze removes singleton axis");

  const std::array<std::int64_t, 1> reps2{2};
  auto rep = v.repeat(std::span<const std::int64_t>(reps2.data(), reps2.size()));
  expect_true(rep.dim() == 1 && rep.size(0) == 6, "repeat expands 1D length");
  expect_true(rep.at(0) == "b" && rep.at(3) == "b", "repeat preserves chunk ordering");
  auto ri = v.repeat_interleave(2, 0);
  expect_true(ri.size(0) == 6 && ri.at(0) == "b" && ri.at(1) == "b" && ri.at(2) == "a", "repeat_interleave duplicates along dim");

  const std::vector<std::int64_t> take_idx{2, 1, -1, 0};
  auto taken = v.take(std::span<const std::int64_t>(take_idx.data(), take_idx.size()));
  expect_true(taken.dim() == 1 && taken.size(0) == 4, "take emits 1D gathered Note");
  expect_true(taken.at(0) == "b" && taken.at(1) == "a" && taken.at(2) == "b" && taken.at(3) == "b",
              "take gathers with negative index support");

  bool threw_hi = false;
  try {
    const std::vector<std::int64_t> bad_idx{3};
    (void)v.take(std::span<const std::int64_t>(bad_idx.data(), bad_idx.size()));
  } catch (const std::exception&) {
    threw_hi = true;
  }
  expect_true(threw_hi, "take throws on out-of-range positive index");

  bool threw_lo = false;
  try {
    const std::vector<std::int64_t> bad_idx{-4};
    (void)v.take(std::span<const std::int64_t>(bad_idx.data(), bad_idx.size()));
  } catch (const std::exception&) {
    threw_lo = true;
  }
  expect_true(threw_lo, "take throws on out-of-range negative index");
}

void test_perf_regression_corpus_ops() {
  std::cout << "[RUN] coden:perf_regression_corpus\n";
  const std::array<std::int64_t, 1> shape{10};
  const std::vector<std::string> values{
      "",
      "beta",
      "東京",
      "beta",
      "naïve",
      "",
      "ßeta",
      "alpha",
      "東京",
      "alpha"};
  const auto note = pluss::make_note(values, std::span<const std::int64_t>(shape.data(), shape.size()));

  const auto sorted = note.sort();
  expect_true(sorted.numel() == note.numel(), "sort keeps same numel on regression corpus");
  for (std::int64_t i = 1; i < sorted.numel(); ++i) {
    expect_true(std::string(sorted.at(i - 1)) <= std::string(sorted.at(i)),
                "sort keeps non-decreasing lexical order on mixed UTF-8 corpus");
  }

  const auto uniq = note.unique();
  expect_true(uniq.numel() == 6, "unique returns 6 distinct values on regression corpus");
  for (std::int64_t i = 1; i < uniq.numel(); ++i) {
    expect_true(std::string(uniq.at(i - 1)) < std::string(uniq.at(i)),
                "unique returns strictly increasing sorted distinct values");
  }

  const auto dedup = note.deduplicate();
  expect_true(dedup.numel() == 6, "deduplicate keeps first-seen distinct values");
  expect_true(dedup.at(0) == "", "deduplicate first token");
  expect_true(dedup.at(1) == "beta", "deduplicate second token");
  expect_true(dedup.at(2) == "東京", "deduplicate third token");
  expect_true(dedup.at(3) == "naïve", "deduplicate fourth token");
  expect_true(dedup.at(4) == "ßeta", "deduplicate fifth token");
  expect_true(dedup.at(5) == "alpha", "deduplicate sixth token");

  const auto sorted_once = note.sort();
  const auto sorted_twice = note.sort();
  expect_true(sorted_once.numel() == sorted_twice.numel(), "sort deterministic numel on repeated run");
  bool same = true;
  for (std::int64_t i = 0; i < sorted_once.numel(); ++i) {
    if (sorted_once.at(i) != sorted_twice.at(i)) {
      same = false;
      break;
    }
  }
  expect_true(same, "sort deterministic ordering across repeated runs");

  const std::array<std::int64_t, 1> shape2{5};
  const std::vector<std::string> repeated_empty{"", "", "", "alpha", "alpha"};
  const auto t = pluss::make_note(repeated_empty, std::span<const std::int64_t>(shape2.data(), shape2.size()));
  const auto uniq2 = t.unique();
  expect_true(uniq2.numel() == 2, "unique retains empty token once on repeated-empty corpus");
  bool uniq2_has_empty = false;
  bool uniq2_has_alpha = false;
  for (std::int64_t i = 0; i < uniq2.numel(); ++i) {
    if (uniq2.at(i) == "") uniq2_has_empty = true;
    if (uniq2.at(i) == "alpha") uniq2_has_alpha = true;
  }
  expect_true(uniq2_has_empty && uniq2_has_alpha, "unique keeps both empty and alpha in repeated-empty corpus");

  const auto dedup2 = t.deduplicate();
  expect_true(dedup2.numel() == 2, "deduplicate retains two first-seen distinct values on repeated-empty corpus");
  expect_true(dedup2.at(0) == "" && dedup2.at(1) == "alpha", "deduplicate keeps first-seen order on repeated-empty corpus");

  const auto undef = pluss::Note::undefined();
  expect_true(!undef.unique().defined(), "unique(undefined) remains undefined");
  expect_true(!undef.deduplicate().defined(), "deduplicate(undefined) remains undefined");
}

void test_argsort_interop_ops() {
  // Note::argsort now returns a closed Int64 Note directly (pass 5 of the
  // Note redesign) -- no CodeM/Series interop required to read the
  // permutation back out, unlike before.
  std::cout << "[RUN] coden:argsort_interop\n";
  const std::array<std::int64_t, 1> shape{7};
  const std::vector<std::string> values{"b", "a", "b", "z", "", "b", "a"};
  const auto note = pluss::make_note(values, std::span<const std::int64_t>(shape.data(), shape.size()));

  const auto order_note = note.argsort(/*descending=*/false, /*stable=*/true);
  expect_true(order_note.is_int64(), "argsort returns an Int64 Note");
  const auto order = order_note.integers();
  expect_true(order.size() == values.size(), "argsort size matches input size");
  expect_true(order[0] == 4 && order[1] == 1 && order[2] == 6 && order[3] == 0 && order[4] == 2 && order[5] == 5 &&
                  order[6] == 3,
              "argsort ascending stable ordering matches expected permutation");

  auto sorted = note.take(std::span<const std::int64_t>(order.data(), order.size()));
  expect_true(sorted.at(0) == "" && sorted.at(1) == "a" && sorted.at(2) == "a" && sorted.at(3) == "b" &&
                  sorted.at(4) == "b" && sorted.at(5) == "b" && sorted.at(6) == "z",
              "take(argsort()) reproduces lexicographically sorted Note");

  const auto desc_note = note.argsort(/*descending=*/true, /*stable=*/true);
  const auto desc = desc_note.integers();
  expect_true(desc[0] == 3 && desc[1] == 0 && desc[2] == 2 && desc[3] == 5 && desc[4] == 1 && desc[5] == 6 &&
                  desc[6] == 4,
              "argsort descending stable ordering matches expected permutation");
}

void test_high_yield_nlp_and_printing() {
  std::cout << "[RUN] coden:high_yield_nlp_print\n";
  pluss::Note n = pluss::make_note("I can't believe this GREAT product costs 123 dollars in 2026.");

  const std::string expanded = std::string(n.nlp().expand_contractions().at(0));
  expect_true(expanded.find("can not") != std::string::npos, "nlp_expand_contractions expands can't");

  const std::string normn = std::string(n.nlp().normalize_numbers().at(0));
  expect_true(normn.find("0 dollars in 0") != std::string::npos, "nlp_normalize_numbers masks numeric spans");

  const double sent = as_score(n.nlp().sentiment());
  expect_true(sent > 0.5, "nlp_sentiment positive for positive lexicon");
  const double tox = as_score(n.nlp().toxicity());
  expect_true(tox < 0.4, "nlp_toxicity stays low for neutral sentence");
  expect_true(!std::string(n.nlp().readability_level().at(0)).empty(), "nlp_readability_level emits label");
  expect_true(!std::string(n.nlp().script().at(0)).empty(), "nlp_script emits script");

  pluss::Note sw = pluss::make_note("the robust method is in the pipeline");
  const std::string swr = std::string(sw.nlp().remove_stopwords().at(0));
  expect_true(swr.find("the") == std::string::npos, "nlp_remove_stopwords removes english stopwords");

  const std::string tr = std::string(pluss::make_note("São").nlp().to_ascii().at(0));
  expect_true(tr == "Sao", "transliterate_ascii strips accent");
  expect_true(!std::string(pluss::make_note("Robert").nlp().soundex().at(0)).empty(), "phonetic_soundex emits code");

  std::string latin1 = "caf\xE9";
  auto decoded = pluss::note::bytes(latin1).from_latin1();
  expect_true(decoded.at(0) == "café", "from_latin1 decodes bytes to UTF-8");
  auto reenc = decoded.to_latin1_lossy();
  expect_true(reenc.is_bytes(),
              "to_latin1_lossy switches encoding to bytes");
  expect_true(reenc.at(0) == latin1, "to_latin1_lossy round-trips basic latin1 text");

  pluss::NotePrintOptions opt;
  opt.max_elements = 6;
  opt.edge_items = 1;
  const std::array<std::int64_t, 2> sizes{2, 6};
  pluss::Note m = pluss::make_note(
      std::vector<std::string>{
          "alpha", "beta", "gamma", "delta", "epsilon", "zeta",
          "line1\nline2", "emoji 🙂", "quoted \"x\"", "kappa", "lambda", "mu"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const std::string printed = pluss::note::print(m, opt);
  expect_true(printed.find("Note(") != std::string::npos, "note::print includes metadata");
  expect_true(printed.find("shape=[2, 6]") != std::string::npos, "note::print includes shape");
  expect_true(printed.find("...") != std::string::npos, "note::print summarizes large notes");

  pluss::NotePrintOptions hex_opt;
  hex_opt.bytes_mode = pluss::coden::NotePrintBytesMode::Hex;
  const std::string bytes_print = pluss::note::print(pluss::note::bytes("Hi"), hex_opt);
  expect_true(bytes_print.find("hex[48 69]") != std::string::npos, "note::print hex mode prints byte hex");

  pluss::Note tex = pluss::make_note(
      "\\section{Intro}\r\n\r\n"
      "\\textbf{Bold} and \\emph{emph}.\\n"
      "Math: $x+y$ and \\(z\\).\\n\\n\\n"
      "{\\bfseries font} line.");
  const std::string tex_norm = std::string(tex.tex().line_norm().at(0));
  expect_true(tex_norm.find('\r') == std::string::npos, "tex_normalize_newlines removes CR");

  const std::string tex_dedup = std::string(tex.tex().dedup_blank_lines().at(0));
  expect_true(tex_dedup.find("\n\n\n") == std::string::npos, "tex_dedup_blank_lines collapses blank runs");

  const std::string escaped = std::string(pluss::make_note("a_b#%").tex().escape().at(0));
  expect_true(escaped.find("\\_") != std::string::npos, "tex_escape escapes underscore");
  expect_true(std::string(pluss::make_note(escaped).tex().unescape().at(0)) == "a_b#%", "tex_unescape reverses escapes");

  const std::string no_delims = std::string(tex.tex().strip_delims().at(0));
  expect_true(no_delims.find("$") == std::string::npos, "tex_remove_math_delimiters strips dollar delimiters");
  expect_true(std::string(pluss::make_note("x+y").tex().add_math_delimiters().at(0)) == "$x+y$",
              "tex_add_math_delimiters wraps plain expression");

  const std::string math_only = std::string(tex.tex().math_only().at(0));
  expect_true(math_only.find("x+y") != std::string::npos, "tex_math_only extracts math payload");
  const std::string text_only = std::string(tex.tex().text_only().at(0));
  expect_true(text_only.find("Math:") != std::string::npos, "tex_text_only keeps prose payload");

  const std::string no_fmt = std::string(tex.tex().strip_fmt().at(0));
  expect_true(no_fmt.find("Bold") != std::string::npos, "tex_remove_formatting keeps formatted content");
  expect_true(no_fmt.find("\\textbf") == std::string::npos, "tex_remove_formatting drops formatting commands");

  const std::string md = std::string(tex.tex().to_markdown().at(0));
  expect_true(md.find("# Intro") != std::string::npos, "tex_to_markdown converts section to markdown heading");
  expect_true(md.find("**Bold**") != std::string::npos, "tex_to_markdown converts bold command");

  const std::string html = std::string(tex.tex().to_html().at(0));
  expect_true(html.find("<h1>Intro</h1>") != std::string::npos, "tex_to_html converts section heading");
  expect_true(html.find("<strong>Bold</strong>") != std::string::npos, "tex_to_html converts bold command");
}

void test_tex_canonicalize_and_macros() {
  std::cout << "[RUN] coden:tex_structural\n";
  pluss::Note t = pluss::make_note(
      "\\newcommand{\\RR}{R}\n"
      "Let $x \\in \\RR$ and \\dfrac{a}{b} + \\sqrt{c} + \\pm 1.\n"
      "Vector: \\mathbf{v}. Matrix: \\begin{bmatrix}1\\end{bmatrix}.\n"
      "Limits: \\sumlimits_{i=1}^n x_i and x^2 and x'' and \\ldots\n"
      "Accents: \\'{e}.");

  const std::string expanded = std::string(t.tex().macro_subs().at(0));
  expect_true(expanded.find("x \\in R") != std::string::npos, "macro_subs expands \\RR");
  expect_true(expanded.find("\\RR") == std::string::npos, "macro_subs removes macro usage");
  expect_true(expanded.find("\\newcommand") == std::string::npos, "macro_subs strips macro decls");

  const std::string canon = std::string(t.tex().canonicalize_macros().at(0));
  expect_true(canon.find("\\frac") != std::string::npos, "tex_canonicalize_macros normalizes \\dfrac");

  const std::string frac = std::string(t.tex().simplify_fractions().at(0));
  expect_true(frac.find("\\frac{a}{b}") != std::string::npos, "tex_simplify_fractions normalizes frac");

  const std::string sqrtp = std::string(t.tex().sqrt_pow().at(0));
  expect_true(sqrtp.find("^(1/2)") != std::string::npos, "sqrt_pow converts sqrt");

  const std::string pm = std::string(t.tex().pm_signs().at(0));
  expect_true(pm.find("+/-") != std::string::npos, "pm_signs converts \\pm");

  const std::string vec = std::string(t.tex().vector_norm().at(0));
  expect_true(vec.find("\\vec{v}") != std::string::npos, "tex_standardize_vectors normalizes vector markup");

  const std::string mat = std::string(t.tex().matrix_norm().at(0));
  expect_true(mat.find("\\begin{matrix}") != std::string::npos, "tex_standardize_matrices normalizes matrix env");

  const std::string sums = std::string(t.tex().sum_norm().at(0));
  expect_true(sums.find("\\sumlimits") == std::string::npos, "tex_standardize_summations removes sumlimits");

  const std::string lims = std::string(t.tex().limit_norm().at(0));
  expect_true(lims.find("\\limits") == std::string::npos, "tex_standardize_limits strips \\limits");

  const std::string subs = std::string(t.tex().sub_norm().at(0));
  expect_true(subs.find("x_{i}") != std::string::npos, "tex_standardize_subscripts braces subscripts");

  const std::string sups = std::string(t.tex().sup_norm().at(0));
  expect_true(sups.find("x^{2}") != std::string::npos, "tex_standardize_superscripts braces superscripts");

  const std::string primes = std::string(t.tex().prime_norm().at(0));
  expect_true(primes.find("\\prime") != std::string::npos, "tex_standardize_primes expands primes");

  const std::string dots = std::string(t.tex().dots_norm().at(0));
  expect_true(dots.find("\\dots") != std::string::npos, "tex_standardize_dots normalizes dots");

  const std::string acc = std::string(t.tex().deaccent().at(0));
  expect_true(acc.find("e") != std::string::npos, "tex_remove_accents strips accent macros");

  const std::string setm = std::string(t.tex().set_macro("Foo", "Bar").at(0));
  expect_true(setm.find("\\newcommand{\\Foo}{Bar}") != std::string::npos, "tex_set_macro inserts macro decl");

  const std::string remm = std::string(t.tex().strip_macro("RR").at(0));
  expect_true(remm.find("\\newcommand") == std::string::npos, "tex_remove_macro removes macro decl");

  const std::string wrapped = std::string(pluss::make_note("x+y").tex().wrap_inline_math("$").at(0));
  expect_true(wrapped == "$x+y$", "tex_wrap_inline_math wraps inline math");

  const std::string display = std::string(pluss::make_note("x+y").tex().wrap_display_math("$$").at(0));
  expect_true(display.find("$$") != std::string::npos, "tex_wrap_display_math wraps display math");

  const std::string bal_br = std::string(pluss::make_note("{a{b}").tex().balance_braces().at(0));
  expect_true(bal_br.find("}}") != std::string::npos, "tex_balance_braces closes missing braces");

  const std::string bal_math = std::string(pluss::make_note("$x+y").tex().balance_math_delims().at(0));
  expect_true(!bal_math.empty() && bal_math.back() == '$', "tex_balance_math_delims closes missing $");

  const std::string inc = std::string(pluss::make_note("\\input{sec1}").tex().input_tags().at(0));
  expect_true(inc.find("[include:sec1]") != std::string::npos, "input_tags rewrites inputs");

  const std::string gfx = std::string(pluss::make_note("\\includegraphics{fig}").tex().graphics_tags().at(0));
  expect_true(gfx.find("[graphic:fig]") != std::string::npos, "tex_resolve_graphics rewrites graphics");
}

void test_note_factory_paths() {
  std::cout << "[RUN] coden:note_factories\n";

  pluss::Note s0 = pluss::make_note("hi");
  expect_true(s0.dim() == 0, "content(\"hi\") produces scalar note");

  pluss::Note s1 = pluss::make_note({"a", "b", "c"});
  expect_true(s1.dim() == 1 && s1.size(0) == 3, "content({\"a\",\"b\",\"c\"}) produces 1D note");

  pluss::Note s2 = pluss::make_note({{"a", "b"}, {"c", "d"}});
  expect_true(s2.dim() == 2 && s2.sizes() == std::vector<std::int64_t>({2, 2}), "content 2D nested list");

  pluss::Note s3 = pluss::make_note({{{"a"}, {"b"}}, {{"c"}, {"d"}}});
  expect_true(s3.dim() == 3 && s3.sizes() == std::vector<std::int64_t>({2, 2, 1}), "content 3D nested list");

  pluss::Note m2 = pluss::make_note({{"x", "y"}, {"z", "w"}});
  expect_true(m2.dim() == 2 && m2.at(0) == "x" && m2.at(3) == "w", "make_note 2D nested list");

  const std::array<std::int64_t, 2> sz{2, 2};
  std::vector<std::string> flat = {"p", "q", "r", "s"};
  pluss::Note mf = pluss::make_note(flat, std::span<const std::int64_t>(sz.data(), sz.size()));
  expect_true(mf.dim() == 2 && mf.at(2) == "r", "make_note(flat, sizes) constructs 2D");

  pluss::Note lines = pluss::note::from_lines("a\nb\nc");
  expect_true(lines.dim() == 1 && lines.size(0) == 3, "note_from_lines splits into 1D note");

  pluss::Note words = pluss::note::from_words("alpha beta");
  expect_true(words.dim() == 1 && words.size(0) == 2, "note_from_words splits words");

  pluss::Note full = pluss::note::full({2, 2, 1}, "x");
  expect_true(full.dim() == 3 && full.numel() == 4 && full.at(0) == "x", "full_note fills shape");

  pluss::Note empty = pluss::note::empty({2, 1, 2});
  expect_true(empty.dim() == 3 && empty.numel() == 4, "empty_note creates shape");

  pluss::Note like = pluss::note::full_like(s2, "y");
  expect_true(like.sizes() == s2.sizes(), "full_note_like copies shape");

  pluss::Note repeated = pluss::note::rep("Alex", {2, 2});
  expect_true(repeated.dim() == 2 && repeated.size(0) == 2 && repeated.at(3) == "Alex",
              "note::rep repeats arbitrary text");

  pluss::Note random_upper = pluss::note::rand({6}, true, 7);
  expect_true(random_upper.dim() == 1 && random_upper.size(0) == 6, "note::rand shape");
  for (std::int64_t i = 0; i < random_upper.numel(); ++i) {
    const auto token = random_upper.at(i);
    expect_true(token.size() == 1 && token[0] >= 'A' && token[0] <= 'Z',
                "note::rand emits uppercase letters");
  }

  pluss::Note random_between = pluss::note::rand_between("D", "H", {6}, 7);
  for (std::int64_t i = 0; i < random_between.numel(); ++i) {
    const auto token = random_between.at(i);
    expect_true(token.size() == 1 && token[0] >= 'D' && token[0] <= 'H',
                "note::rand_between honors inclusive bounds");
  }

  pluss::Note alpha_range = pluss::note::arange("A", "H", 2);
  expect_true(alpha_range.size(0) == 4 &&
                  alpha_range.at(0) == "A" &&
                  alpha_range.at(1) == "C" &&
                  alpha_range.at(2) == "E" &&
                  alpha_range.at(3) == "G",
              "note::arange walks characters");

  pluss::Note combos = pluss::note::perm("abc", 2);
  expect_true(combos.size(0) == 3 &&
                  combos.at(0) == "ab" &&
                  combos.at(1) == "ac" &&
                  combos.at(2) == "bc",
              "note::perm default combination mode");

  pluss::Note perms = pluss::note::perm(
      "aba", 2, pluss::note::WordSelectMode::Permutation, false, true);
  expect_true(perms.size(0) == 3 &&
                  perms.at(0) == "ab" &&
                  perms.at(1) == "aa" &&
                  perms.at(2) == "ba",
              "note::perm permutation mode deduplicates repeated letters");

  pluss::Note b0 = pluss::note::bytes("hi");
  expect_true(b0.is_bytes(), "bytes_content uses bytes mode");

  pluss::Note b2 = pluss::note::bytes({{"\x01", "\x02"}, {"\x03", "\x04"}});
  expect_true(b2.dim() == 2 && b2.at(3).size() == 1, "bytes_content supports nested list");

  pluss::Note bfull = pluss::note::full_bytes_like(b2, "\xFF");
  expect_true(bfull.is_bytes(), "full_bytes_note_like preserves bytes");

  pluss::Note lower = s3.lower();
  expect_true(lower.dim() == 3 && lower.sizes() == s3.sizes(), "methods preserve 3D shape");

  pluss::Note escaped = s3.tex().escape();
  expect_true(escaped.dim() == 3 && escaped.sizes() == s3.sizes(), "tex methods preserve 3D shape");
}

void test_aux_transmit_translate_and_queue_basics() {
  std::cout << "[RUN] aux:transmit_translate_queue\n";

  const auto mapped = pluss::tils::transmit([](int x) { return x * x; }, std::vector<int>{1, 2, 3});
  expect_true(mapped.size() == 3 && mapped[2] == 9, "transmit maps range values");

  pluss::Note n = pluss::make_note({{"aa", "bbb"}, {"c", "dddd"}});
  const auto per_row_chars = pluss::tils::map_axis(
      [](const pluss::Note& row) {
        std::int64_t total = 0;
        for (std::int64_t i = 0; i < row.numel(); ++i) total += static_cast<std::int64_t>(row.at(i).size());
        return total;
      },
      n,
      1);
  expect_true(per_row_chars.size() == 2, "map_axis(Note, axis=1) returns one value per row slice");
  expect_true(per_row_chars[0] == 5 && per_row_chars[1] == 5, "map_axis computes per-row aggregation");

  const auto per_col_chars = pluss::tils::map_axis(
      [](const pluss::Note& col) {
        std::int64_t total = 0;
        for (std::int64_t i = 0; i < col.numel(); ++i) total += static_cast<std::int64_t>(col.at(i).size());
        return total;
      },
      n,
      0);
  expect_true(per_col_chars.size() == 2, "map_axis(Note, axis=0) returns one value per column slice");
  expect_true(per_col_chars[0] == 3 && per_col_chars[1] == 7, "map_axis axis=0 follows column-wise semantics");

  pluss::Type qn = n;
  expect_true(qn.kind() == pluss::Type::Kind::Note, "Type stores Note");
  expect_true(qn.defined(), "Type reports defined for Note payload");
  expect_true(qn.as_note().at(0) == "aa", "Type.as_note() exposes Note surface");
  pluss::Type qn_lower = qn.invoke("lower");
  expect_true(qn_lower.as<pluss::Note>().at(0) == "aa", "Type.invoke dispatches Note methods");

  pluss::Type qs = pluss::make_series({-1.0, 2.0, -3.0});
  expect_true(qs.as_series().to_vec<double>()[1] == 2.0, "Type.as_series() exposes Series surface");
  pluss::Type qs_abs = qs.invoke("abs");
  auto abs_vals = qs_abs.as<pluss::Series>().to_vec<double>();
  expect_true(abs_vals.size() == 3 && abs_vals[0] == 1.0 && abs_vals[2] == 3.0,
              "Type.invoke dispatches Series methods");

  pluss::Type qsum = qs + pluss::Type(pluss::make_series({1.0, 1.0, 1.0}));
  auto sum_vals = qsum.as<pluss::Series>().to_vec<double>();
  expect_true(sum_vals.size() == 3 && sum_vals[0] == 0.0 && sum_vals[1] == 3.0 && sum_vals[2] == -2.0,
              "Type binary + delegates to Series");

  pluss::Type qcmp = qn == pluss::Type(pluss::make_note({{"aa", "zzz"}, {"c", "DDDD"}}));
  auto cmp_vals = qcmp.as<pluss::Series>().to_vec<bool>();
  expect_true(cmp_vals.size() == 4 && cmp_vals[0] && !cmp_vals[1] && cmp_vals[2] && !cmp_vals[3],
              "Type Note comparison delegates safely");

  pluss::Type qwhere = pluss::Type::where(
      pluss::Type(pluss::make_series({true, false, true})),
      pluss::Type(pluss::make_series({10.0, 20.0, 30.0})),
      pluss::Type(pluss::make_series({1.0, 2.0, 3.0})));
  auto where_vals = qwhere.as<pluss::Series>().to_vec<double>();
  expect_true(where_vals.size() == 3 && where_vals[0] == 10.0 && where_vals[1] == 2.0 && where_vals[2] == 30.0,
              "Type::where delegates ternary selection for Series");
}

} // namespace

int main() {
  test_basic_extracts();
  test_text_kernels_compare_hash();
  test_encoded_fastpath_parity();
  test_nlp_extracts_and_filters();
  test_nlp_keyword_frequency_order();
  test_parallel_lane_parity();
  test_reference_mode_parity();
  test_diff_align_family();
  test_pairwise_set_ops_and_fast_paths();
  test_encoding_and_shape_ops();
  test_perf_regression_corpus_ops();
  test_argsort_interop_ops();
  test_high_yield_nlp_and_printing();
  test_tex_canonicalize_and_macros();
  test_note_factory_paths();
  test_aux_transmit_translate_and_queue_basics();

  if (g_failures == 0) {
    std::cout << "[OK] CodeN no-catch smoke tests passed\n";
    return 0;
  }
  std::cerr << "[FAIL] CodeN no-catch smoke tests failed: " << g_failures << "\n";
  return 1;
}
