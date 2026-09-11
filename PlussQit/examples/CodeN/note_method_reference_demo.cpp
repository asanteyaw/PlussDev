// Permanent verifier for the Note method API Reference
// (docs/sphinx/api/note/*.md). Every deterministic example output on those
// pages is produced here. One section per documented method, A-Z; keep the
// labels stable so a page and its output stay easy to line up.

#include <pluss/pluss.h>

#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::note::print(value) << '\n';
}

// contains()/is_*() predicates and lengths_*()/count_*() measurements return
// a shape-preserving Bool/Int64 Note (passes 4-5 of the Note redesign), not
// a raw std::vector.
void show_mask(std::string_view label, const pluss::Note& mask) {
  std::cout << "\n=== " << label << " ===\n[";
  const auto lanes = mask.bools();
  for (std::size_t i = 0; i < lanes.size(); ++i) {
    std::cout << (i ? ", " : "") << static_cast<int>(lanes[i]);
  }
  std::cout << "]\n";
}

void show_flag(std::string_view label, bool value) {
  std::cout << "\n=== " << label << " ===\n"
            << std::boolalpha << value << '\n';
}

void show_lens(std::string_view label, const pluss::Note& lens) {
  std::cout << "\n=== " << label << " ===\n[";
  const auto lanes = lens.integers();
  for (std::size_t i = 0; i < lanes.size(); ++i) {
    std::cout << (i ? ", " : "") << lanes[i];
  }
  std::cout << "]\n";
}

// A TokenBatch is a flat token Note plus an offsets Note (Int64). Print
// both, then the per-input-cell slices the offsets describe.
void show_batch(std::string_view label, const pluss::coden::TokenBatch& batch) {
  std::cout << "\n=== " << label << " ===\n"
            << "tokens: " << pluss::note::print(batch.tokens()) << '\n'
            << "offsets: [";
  const auto off = batch.offsets().integers();
  for (std::size_t i = 0; i < off.size(); ++i) {
    std::cout << (i ? ", " : "") << off[i];
  }
  std::cout << "]\nrows:";
  for (std::size_t r = 0; r + 1 < off.size(); ++r) {
    std::cout << "\n  [" << r << "] ";
    for (std::int64_t t = off[r]; t < off[r + 1]; ++t) {
      std::cout << (t == off[r] ? "" : " | ") << '"' << batch.tokens().at(t) << '"';
    }
    if (off[r] == off[r + 1]) std::cout << "(empty)";
  }
  std::cout << '\n';
}

template <class Fn>
void show_reject(std::string_view label, Fn&& fn) {
  std::cout << "\n=== " << label << " ===\n";
  try {
    fn();
    std::cout << "(no error)\n";
  } catch (const std::exception& e) {
    const std::string m = e.what();
    std::cout << m.substr(0, m.find('\n')) << '\n';
  }
}

// Raw projection: emit the exact bytes of each cell, one per line, without the
// Note printer's own escaping. Used where the printer would double-escape the
// backslashes an operation just produced (e.g. json_escape).
void show_raw(std::string_view label, const pluss::Note& value) {
  std::cout << "\n=== " << label << " ===\n";
  for (std::int64_t i = 0; i < value.numel(); ++i) {
    std::cout << std::string(value.at(i)) << '\n';
  }
}

// ----------------------------------------------------------- diff / align -----
// Each compares self against another Note cell-by-cell and returns a
// TokenBatch of tag-prefixed tokens: '=' unchanged (align only), '-' removed
// from self, '+' added from other.
void section_diff_align() {
  // ---- align_sentences ----
  show_batch("align_sentences/basic",
             pluss::make_note({"One. Two. Three."})
                 .align_sentences(pluss::make_note({"One. Two changed. Three."})));
  show_batch("align_sentences/identical",
             pluss::make_note({"Stay put. Do not move."})
                 .align_sentences(pluss::make_note({"Stay put. Do not move."})));
  show_batch("align_sentences/appended",
             pluss::make_note({"First one. Second one."})
                 .align_sentences(pluss::make_note({"First one. Second one. Third one."})));

  // ---- align_tokens ----
  show_batch("align_tokens/basic",
             pluss::make_note({"the quick brown fox"})
                 .align_tokens(pluss::make_note({"the slow brown fox"})));
  show_batch("align_tokens/identical",
             pluss::make_note({"same words"}).align_tokens(pluss::make_note({"same words"})));
  show_batch("align_tokens/per-cell",
             pluss::make_note({"a b c", "x y"})
                 .align_tokens(pluss::make_note({"a c", "x z"})));

  // ---- diff_chars ----
  show_batch("diff_chars/edit-script",
             pluss::make_note({"kitten"}).diff_chars(pluss::make_note({"sitting"})));
  show_batch("diff_chars/pure-insertion",
             pluss::make_note({"abc"}).diff_chars(pluss::make_note({"abXYZc"})));
  show_batch("diff_chars/identical",
             pluss::make_note({"same"}).diff_chars(pluss::make_note({"same"})));

  // ---- diff_lines ----
  show_batch("diff_lines/basic",
             pluss::make_note({"alpha\nbeta\ngamma"})
                 .diff_lines(pluss::make_note({"alpha\nBETA\ngamma\ndelta"})));
  show_batch("diff_lines/identical",
             pluss::make_note({"one\ntwo"}).diff_lines(pluss::make_note({"one\ntwo"})));
  show_batch("diff_lines/inserted",
             pluss::make_note({"one\ntwo"}).diff_lines(pluss::make_note({"one\nEXTRA\ntwo"})));

  // ---- diff_words ----
  show_batch("diff_words/basic",
             pluss::make_note({"the quick brown fox", "hello world"})
                 .diff_words(pluss::make_note({"the slow brown fox", "hello there world"})));
  show_batch("diff_words/identical",
             pluss::make_note({"no change here"}).diff_words(pluss::make_note({"no change here"})));
  show_batch("diff_words/broadcast",
             pluss::make_note({"a b c", "a x c"}).diff_words(pluss::make_note({"a b c"})));
  show_batch("diff_words/all-inserted",
             pluss::make_note({""}).diff_words(pluss::make_note({"brand new text"})));
  show_batch("diff_words/ignores-punctuation",
             pluss::make_note({"stop here"}).diff_words(pluss::make_note({"stop, here!"})));
  show_reject("diff_words/reject-mismatch",
              [] { (void)pluss::make_note({"a", "b"}).diff_words(pluss::make_note({"a", "b", "c"})); });
  show_reject("diff_words/reject-bytes",
              [] { (void)pluss::note::bytes({"x"}).diff_words(pluss::make_note({"y"})); });
}

// ------------------------------------------------------- set similarity -------
// intersect_* -> TokenBatch of the shared token set (self's order, deduped,
// lowercased). jaccard_* -> |A n B| / |A u B|. overlap_* -> |A n B| / min(|A|,|B|).
// Five token-set kinds: tokens (all words), terms (content words, len >= 4,
// no stopwords), acronyms (ALL-CAPS runs), citations ([n] or (Author, year)),
// math_symbols (\command or one of + - * / = < > ^).
void section_set_similarity() {
  // intersect_*
  show_batch("intersect_tokens/basic",
             pluss::make_note({"the quick brown fox", "alpha beta gamma"})
                 .intersect_tokens(pluss::make_note({"a quick red fox", "beta gamma delta"})));
  show_batch("intersect_tokens/dedup-and-order",
             pluss::make_note({"fox fox dog cat fox"}).intersect_tokens(pluss::make_note({"cat FOX"})));
  show_batch("intersect_tokens/broadcast",
             pluss::make_note({"cat dog", "dog bird"}).intersect_tokens(pluss::make_note({"dog"})));
  show_batch("intersect_tokens/disjoint",
             pluss::make_note({"one two"}).intersect_tokens(pluss::make_note({"three four"})));
  show_batch("intersect_terms/content-words",
             pluss::make_note({"The cat sat on the warm mat"})
                 .intersect_terms(pluss::make_note({"a warm mat and a cat"})));
  show_batch("intersect_acronyms/basic",
             pluss::make_note({"NASA and ESA and NOAA"}).intersect_acronyms(pluss::make_note({"ESA, NASA, JAXA"})));
  show_batch("intersect_acronyms/no-caps",
             pluss::make_note({"nasa esa"}).intersect_acronyms(pluss::make_note({"nasa"})));
  show_batch("intersect_citations/basic",
             pluss::make_note({"see [1] and [2] and [10]"})
                 .intersect_citations(pluss::make_note({"refer to [2], [3], [10]"})));
  show_batch("intersect_math_symbols/basic",
             pluss::make_note({"a + b = c \\alpha \\beta"})
                 .intersect_math_symbols(pluss::make_note({"x = y - z \\beta"})));

  // jaccard_*
  show("jaccard_tokens/basic",
       pluss::make_note({"the quick brown fox", "alpha beta gamma"})
           .jaccard_tokens(pluss::make_note({"the quick red fox", "beta gamma delta"})));
  show("jaccard_tokens/identical",
       pluss::make_note({"same words here"}).jaccard_tokens(pluss::make_note({"same words here"})));
  show("jaccard_tokens/disjoint",
       pluss::make_note({"a b c"}).jaccard_tokens(pluss::make_note({"d e f"})));
  show("jaccard_tokens/both-empty",
       pluss::make_note({""}).jaccard_tokens(pluss::make_note({""})));
  show("jaccard_tokens/one-empty",
       pluss::make_note({"a b"}).jaccard_tokens(pluss::make_note({""})));
  show("jaccard_tokens/broadcast",
       pluss::make_note({"cat dog", "dog bird fish"}).jaccard_tokens(pluss::make_note({"dog"})));
  show("jaccard_tokens/2d-shape",
       pluss::make_note({{"cat dog", "sun moon"}, {"red blue", "up down"}})
           .jaccard_tokens(pluss::make_note({{"cat fish", "sun moon"}, {"red green", "left right"}})));
  show("jaccard_terms/content-only",
       pluss::make_note({"The system processes documents"})
           .jaccard_terms(pluss::make_note({"a system that processes files"})));
  show("jaccard_acronyms/basic",
       pluss::make_note({"NASA ESA NOAA"}).jaccard_acronyms(pluss::make_note({"NASA ESA JAXA"})));
  show("jaccard_citations/basic",
       pluss::make_note({"[1] [2] [3]"}).jaccard_citations(pluss::make_note({"[2] [3] [4]"})));
  show("jaccard_math_symbols/basic",
       pluss::make_note({"a + b = c"}).jaccard_math_symbols(pluss::make_note({"a - b = c"})));

  // overlap_*
  show("overlap_tokens/subset-is-one",
       pluss::make_note({"quick brown fox"})
           .overlap_tokens(pluss::make_note({"the quick brown fox jumps over"})));
  show("overlap_tokens/genuine-partial",
       pluss::make_note({"a b c d"}).overlap_tokens(pluss::make_note({"c d e"})));
  show("overlap_tokens/disjoint",
       pluss::make_note({"a b"}).overlap_tokens(pluss::make_note({"c d"})));
  show("overlap_tokens/one-empty",
       pluss::make_note({"a b"}).overlap_tokens(pluss::make_note({""})));
  show("overlap_terms/basic",
       pluss::make_note({"machine learning models"})
           .overlap_terms(pluss::make_note({"statistical learning models with data"})));
  show("overlap_acronyms/basic",
       pluss::make_note({"NASA ESA"}).overlap_acronyms(pluss::make_note({"NASA ESA NOAA JAXA"})));
  show("overlap_citations/basic",
       pluss::make_note({"[1] [2]"}).overlap_citations(pluss::make_note({"[1] [2] [3] [4]"})));
  show("overlap_math_symbols/basic",
       pluss::make_note({"\\sum + \\int"})
           .overlap_math_symbols(pluss::make_note({"\\sum + \\int - \\lim = 0"})));

  show_reject("jaccard_tokens/reject-bytes",
              [] { (void)pluss::note::bytes({"x"}).jaccard_tokens(pluss::make_note({"y"})); });
  show_reject("intersect_tokens/reject-mismatch",
              [] { (void)pluss::make_note({"a", "b"}).intersect_tokens(pluss::make_note({"a", "b", "c"})); });
}

// -------------------------------------------------- encoding / transcode ------
// Escape forms keep the Note's mode and byte structure, rewriting only markup
// or control characters. Transcoders convert between a bytes-mode payload and
// UTF-8 text (from_*) or the reverse (to_*_lossy), changing the encoding tag.
void section_encoding_transcode() {
  // ---- html_escape ----
  show("html_escape/specials",
       pluss::make_note({"a & b < c > d \" e ' f", "plain"}).html_escape());
  show("html_escape/non-ascii-passthrough", pluss::make_note({"café — €"}).html_escape());
  show("html_escape/2d", pluss::make_note({{"<x>", "y&z"}}).html_escape());
  show("html_escape/double-escapes-amp", pluss::make_note({"<a>"}).html_escape().html_escape());

  // ---- html_unescape ----
  show("html_unescape/named",
       pluss::make_note({"a &amp; b &lt;c&gt; &quot;q&quot; &#39;x&apos;"}).html_unescape());
  show("html_unescape/numeric",
       pluss::make_note({"&#65;&#x41; &#8364; &#x1F600;"}).html_unescape());
  show("html_unescape/unknown-left-verbatim",
       pluss::make_note({"&foo; &amp &notreal;"}).html_unescape());
  show("html_unescape/roundtrip",
       pluss::make_note({"a & b < c > d \" e ' f"}).html_escape().html_unescape());

  // ---- json_escape (raw projection: the printer would double-escape) ----
  show_raw("json_escape/quotes-backslash",
           pluss::make_note({std::string("say \"hi\" \\ end")}).json_escape());
  show_raw("json_escape/controls",
           pluss::make_note({std::string("tab\t nl\n cr\r bell") + '\x07' + " end"})
               .json_escape());
  show_raw("json_escape/non-ascii-passthrough", pluss::make_note({"café"}).json_escape());

  // ---- to_utf8_strict ----
  show("to_utf8_strict/valid-bytes-to-text",
       pluss::note::bytes({"café", "abc"}).to_utf8_strict());
  show_reject("to_utf8_strict/reject-invalid", [] {
    (void)pluss::note::bytes({std::string("bad\xFF\xFE") + "bytes"}).to_utf8_strict();
  });

  // ---- to_utf8_lossy ----
  show("to_utf8_lossy/replaces-invalid",
       pluss::note::bytes({std::string("ok-") + '\xFF' + "-" + '\x80' + "-end"}).to_utf8_lossy());
  show("to_utf8_lossy/valid-unchanged", pluss::note::bytes({"café"}).to_utf8_lossy());

  // ---- from_latin1 ----
  show("from_latin1/high-bytes",
       pluss::note::bytes({std::string("caf") + '\xE9' + " " + '\xA9'}).from_latin1());

  // ---- to_latin1_lossy (result is bytes mode) ----
  show("to_latin1_lossy/roundtrippable", pluss::make_note({std::string("caf") + "é"}).to_latin1_lossy());
  show("to_latin1_lossy/unmappable-to-question", pluss::make_note({"a€b中c"}).to_latin1_lossy());
  show("to_latin1_lossy/then-from_latin1",
       pluss::make_note({std::string("caf") + "é"}).to_latin1_lossy().from_latin1());

  // ---- from_windows1252 ----
  show("from_windows1252/smart-punctuation",
       pluss::note::bytes({std::string("\x93") + "quote" + "\x94" + " " + "\x97" + " bullet "
                           + "\x95" + " euro " + "\x80"}).from_windows1252());
  show("from_windows1252/unmapped-slot",
       pluss::note::bytes({std::string("x\x81y\x8d") + "z"}).from_windows1252());

  // ---- to_windows1252_lossy (result is bytes mode) ----
  show("to_windows1252_lossy/smart-quotes",
       pluss::make_note({std::string("“") + "q" + "”" + "—" + "€"}).to_windows1252_lossy());
  show("to_windows1252_lossy/unmappable", pluss::make_note({"a中b"}).to_windows1252_lossy());
}

// ---------------------------------------------------------- base64_encode -----
void section_base64_encode() {
  show("base64_encode/scalar", pluss::make_note("Hello, Note").base64_encode());
  show("base64_encode/1d-padding",
       pluss::make_note({"abc", "ab", "a", ""}).base64_encode());
  show("base64_encode/utf8-bytes", pluss::make_note({"café", "€"}).base64_encode());
  show("base64_encode/2d",
       pluss::make_note({{"one", "two"}, {"three", "four"}}).base64_encode());
  show("base64_encode/round-trip",
       pluss::make_note({"payload", "café"}).base64_encode().base64_decode());
}

// --------------------------------------------------------------- contains -----
void section_contains() {
  const auto n = pluss::make_note({"midterm_1", "final_exam", "quiz"});
  show_mask("contains/basic", n.contains("exam"));
  show_mask("contains/case-sensitive", n.contains("EXAM"));
  show_mask("contains/after-lower",
            pluss::make_note({"Exam 1", "FINAL EXAM", "Quiz"}).lower().contains("exam"));
  show_mask("contains/empty-needle", pluss::make_note({"abc", "", "x"}).contains(""));
  show_mask("contains/needle-longer-than-cell", pluss::make_note({"ab"}).contains("abcd"));
  show_mask("contains/2d-flattened",
            pluss::make_note({{"apple", "grape"}, {"plum", "pear"}}).contains("p"));
}

// ---------------------------------------------------------- lengths_bytes -----
void section_lengths_bytes() {
  show_lens("lengths_bytes/ascii", pluss::make_note({"", "a", "abc"}).lengths_bytes());
  show_lens("lengths_bytes/utf8",
            pluss::make_note({"café", "日本語", "🎯"}).lengths_bytes());
  show_lens("lengths_bytes/vs-strip",
            pluss::make_note({"  id-01  ", "\tAB\t"}).strip().lengths_bytes());
  show_lens("lengths_bytes/2d-flattened",
            pluss::make_note({{"a", "bb"}, {"ccc", "dddd"}}).lengths_bytes());
}

// ------------------------------------------------------------------- lower -----
void section_lower() {
  show("lower/1d",
       pluss::make_note({"  Ada LOVELACE  ", "\tGRACE Hopper\t", "Linus"}).lower());
  show("lower/scalar", pluss::make_note("HeLLo, WORLD").lower());
  show("lower/2d", pluss::make_note({{"AB", "cD"}, {"Ef", "GH"}}).lower());
  show("lower/unicode",
       pluss::make_note({"CAFÉ", "ΑΒΓ ДЕЁ", "İ", "ẞ"}).lower());
  show("lower/idempotent", pluss::make_note({"already lower"}).lower().lower());
  show("lower/then-strip",
       pluss::make_note({"  Ada LOVELACE  ", "\tGRACE Hopper\t"}).strip().lower());
}

// ------------------------------------------------------------ nlp_mask_pii -----
void section_nlp_mask_pii() {
  const auto n = pluss::make_note(
      {"email bob@x.io url https://a.b @alice #ship on 2026-01-03 at 09:30 for $12.50 id 42",
       "call jane 555-0100 tomorrow"});
  show("nlp_mask_pii/all", n.nlp_mask_pii());
  show("nlp_mask_pii/numbers-only", n.nlp_mask_pii(1));
  show("nlp_mask_pii/email+url", n.nlp_mask_pii(2 | 4));
  show("nlp_mask_pii/no-op-when-nothing-matches",
       pluss::make_note({"plain prose with no identifiers"}).nlp_mask_pii());
  show("nlp_mask_pii/2d",
       pluss::make_note({{"ping a@b.io", "code 7"}}).nlp_mask_pii());
}

// --------------------------------------------------------- nlp_remove_urls -----
void section_nlp_remove_urls() {
  show("nlp_remove_urls/http-and-www",
       pluss::make_note({"see https://pluss.dev/docs and www.example.com now", "no link here"})
           .nlp_remove_urls());
  show("nlp_remove_urls/scalar",
       pluss::make_note("visit http://a.b/c?q=1 today").nlp_remove_urls());
  show("nlp_remove_urls/no-url-unchanged",
       pluss::make_note({"emails a@b.io are kept"}).nlp_remove_urls());
}

// ------------------------------------------------------------------- quote -----
void section_quote() {
  show("quote/1d", pluss::make_note({"payload", "two words", ""}).quote());
  show("quote/scalar", pluss::make_note("solo").quote());
  show("quote/2d", pluss::make_note({{"a", "b"}, {"c", "d"}}).quote());
  show("quote/interior-quote-not-escaped", pluss::make_note({"a\"b"}).quote());
  show("quote/round-trip-with-unquote",
       pluss::make_note({"payload", "  spaced  "}).quote().unquote());
}

// ----------------------------------------------------------- regex_findall -----
void section_regex_findall() {
  const auto n = pluss::make_note({"Order 17 ships 2026-08-02", "Ticket 204 at 18:30", "none"});
  show_batch("regex_findall/digits", n.regex_findall(R"(\d+)"));
  show_batch("regex_findall/capture-group",
             pluss::make_note({"k=1; v=2", "x=9"}).regex_findall(R"(\w+=(\d))"));
  show_batch("regex_findall/case-insensitive",
             pluss::make_note({"AbCdE fF"}).regex_findall(R"([a-z]+)", 1));
  show_batch("regex_findall/no-flags-baseline",
             pluss::make_note({"AbCdE fF"}).regex_findall(R"([a-z]+)"));
  show_batch("regex_findall/no-match-empty-row",
             pluss::make_note({"abc", "123"}).regex_findall(R"(\d+)"));
  show_reject("regex_findall/reject-bad-pattern",
              [] { (void)pluss::make_note({"x"}).regex_findall("("); });
}

// -------------------------------------------------------------- remove_all -----
void section_remove_all() {
  show("remove_all/basic",
       pluss::make_note({"alpha beta beta", "beta only", "none"}).remove_all("beta"));
  show("remove_all/non-overlapping",
       pluss::make_note({"aaaaa"}).remove_all("aa"));
  show("remove_all/needle-absent-unchanged",
       pluss::make_note({"keep me"}).remove_all("zzz"));
  show("remove_all/2d", pluss::make_note({{"a-b-c", "d-e"}}).remove_all("-"));
}

// ----------------------------------------------------------------- replace -----
void section_replace() {
  show("replace/all-occurrences",
       pluss::make_note({"score: 75 score: x", "pending"}).replace("score", "mark"));
  show("replace/underscores-to-spaces",
       pluss::make_note({"Ada_Lovelace", "Grace_Hopper"}).replace("_", " "));
  show("replace/non-overlapping",
       pluss::make_note({"aaaa"}).replace("aa", "b"));
  show("replace/empty-from-is-a-no-op",
       pluss::make_note({"unchanged"}).replace("", "X"));
  show("replace/to-empty-equals-remove-all",
       pluss::make_note({"a.b.c"}).replace(".", ""));
  show("replace/2d",
       pluss::make_note({{"x1", "x2"}, {"x3", "x4"}}).replace("x", "col"));
}

// ------------------------------------------------------------------- split -----
void section_split() {
  show_batch("split/comma", pluss::make_note({"a,b,c", "x,y", "solo"}).split(","));
  show_batch("split/keeps-empty-fields",
             pluss::make_note({"a,,b", ",lead", "trail,"}).split(","));
  show_batch("split/multi-char-delim",
             pluss::make_note({"left -> right -> end"}).split(" -> "));
  show_batch("split/delim-absent",
             pluss::make_note({"no delimiter here"}).split(";"));
  show_batch("split/limit",
             pluss::make_note({"a,b,c,d,e"}).split(",", 2));
  show_reject("split/reject-empty-delim",
              [] { (void)pluss::make_note({"x"}).split(""); });
}

// -------------------------------------------------------------- split_lines -----
void section_split_lines() {
  show_batch("split_lines/basic",
             pluss::make_note({"line1\nline2\nline3", "solo"}).split_lines());
  show_batch("split_lines/trailing-newline",
             pluss::make_note({"a\nb\n"}).split_lines());
  show_batch("split_lines/blank-line-kept",
             pluss::make_note({"a\n\nc"}).split_lines());
  show_batch("split_lines/crlf",
             pluss::make_note({"a\r\nb"}).split_lines());
  show_batch("split_lines/keep-ends",
             pluss::make_note({"a\nb\n"}).split_lines(true));
  show_batch("split_lines/empty-input",
             pluss::make_note({""}).split_lines());
}

// --------------------------------------------------------- split_whitespace -----
void section_split_whitespace() {
  show_batch("split_whitespace/runs-collapse",
             pluss::make_note({"alpha   beta\tgamma", "delta epsilon"}).split_whitespace());
  show_batch("split_whitespace/leading-trailing-trimmed",
             pluss::make_note({"  Ada Lovelace  ", "\tGrace Hopper\t"}).split_whitespace());
  show_batch("split_whitespace/limit",
             pluss::make_note({"one two three four"}).split_whitespace(1));
  show_batch("split_whitespace/all-whitespace-empty",
             pluss::make_note({"   ", ""}).split_whitespace());
}

// ------------------------------------------------------------------- strip -----
void section_strip() {
  show("strip/whitespace-both-ends",
       pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"}).strip());
  show("strip/internal-whitespace-kept",
       pluss::make_note({"a\tb  c"}).strip());
  show("strip/chars-set",
       pluss::make_note({"##alpha##", "#beta", "gamma##", "#a#b#"}).strip("#"));
  show("strip/chars-multiple",
       pluss::make_note({"xy-payload-yx"}).strip("xy-"));
  show("strip/all-strippable-becomes-empty",
       pluss::make_note({"   ", "###"}).strip().strip("#"));
  show("strip/2d", pluss::make_note({{"  a  ", "  b  "}}).strip());
}

// -------------------------------------------------------------- tex_escape -----
void section_tex_escape() {
  show("tex_escape/all-specials",
       pluss::make_note({"a_b $x$ 50% cost #1 & more {y} ~ ^ \\"}).tex_escape());
  show("tex_escape/plain-text-unchanged",
       pluss::make_note({"nothing special here"}).tex_escape());
  show("tex_escape/scalar", pluss::make_note("C_1 & C_2").tex_escape());
  show("tex_escape/round-trips-through-tex_to_text",
       pluss::make_note({"rate 50% of x_1"}).tex_escape().tex_to_text());
}

// ------------------------------------------------------------- tex_to_text -----
void section_tex_to_text() {
  show("tex_to_text/commands-and-math",
       pluss::make_note({"Text \\textbf{bold} and $x + y$.", "$only math$"}).tex_to_text());
  show("tex_to_text/comments-and-preamble",
       pluss::make_note({"\\documentclass{article}\n% note\n\\begin{document}\nBody \\emph{here}\n\\end{document}"})
           .tex_to_text());
  show("tex_to_text/mode-1-casefolds",
       pluss::make_note({"\\textbf{Hello} World"}).tex_to_text(1));
  show("tex_to_text/mode-0-default",
       pluss::make_note({"\\textbf{Hello} World"}).tex_to_text(0));
  show("tex_to_text/scalar",
       pluss::make_note("\\alpha kept? \\textit{no}").tex_to_text());
}

// ------------------------------------------------ transliteration / phonetics --
// transliterate_* rewrite one script into ASCII (lossy). phonetic_* reduce each
// word to a pronunciation key, tokenizing and space-joining per cell.
void section_transliteration_phonetics() {
  // ---- transliterate_ascii ----
  show("transliterate_ascii/latin1-accents",
       pluss::make_note({"café", "señor", "Ærø", "naïve"}).transliterate_ascii());
  show("transliterate_ascii/beyond-latin1",
       pluss::make_note({"ĥ", "Œuvre", "π ≈ 3"}).transliterate_ascii());
  show("transliterate_ascii/nfd-widens",
       pluss::make_note({"ā", "ǔ", "Ω"}).to_nfd().transliterate_ascii());
  show("transliterate_ascii/ascii-unchanged",
       pluss::make_note({"plain text 123"}).transliterate_ascii());
  show("transliterate_ascii/2d", pluss::make_note({{"àb", "c"}}).transliterate_ascii());

  // ---- transliterate_greek ----
  show("transliterate_greek/accented-words",
       pluss::make_note({"Ελλάς", "θεωρία", "Χάος", "ΩΜΕΓΑ"}).transliterate_greek());
  show("transliterate_greek/mixed-script", pluss::make_note({"alpha=Α beta=β"}).transliterate_greek());
  show("transliterate_greek/final-sigma", pluss::make_note({"λόγος"}).transliterate_greek());
  show("transliterate_greek/polytonic-via-nfd",
       pluss::make_note({"ἄνθρωπος"}).to_nfd().transliterate_greek());

  // ---- transliterate_cyrillic ----
  show("transliterate_cyrillic/words",
       pluss::make_note({"Москва", "Пушкин", "щи", "Ёлка"}).transliterate_cyrillic());
  show("transliterate_cyrillic/soft-hard-sign",
       pluss::make_note({"подъезд", "день"}).transliterate_cyrillic());
  show("transliterate_cyrillic/mixed-script", pluss::make_note({"тест test"}).transliterate_cyrillic());

  // ---- phonetic_soundex ----
  show("phonetic_soundex/names",
       pluss::make_note({"Robert", "Rupert", "Rubin", "Ashcraft", "Tymczak"}).phonetic_soundex());
  show("phonetic_soundex/hw-transparent",
       pluss::make_note({"Ashcroft", "Pfister"}).phonetic_soundex());
  show("phonetic_soundex/multi-word",
       pluss::make_note({"John Smith", "the quick brown fox"}).phonetic_soundex());
  show("phonetic_soundex/short-and-empty", pluss::make_note({"Lee", "A", ""}).phonetic_soundex());
  show("phonetic_soundex/punctuation-stripped", pluss::make_note({"O'Brien, Jr."}).phonetic_soundex());

  // ---- phonetic_metaphone ----
  show("phonetic_metaphone/names",
       pluss::make_note({"Thompson", "knight", "programming", "Pfister"}).phonetic_metaphone());
  show("phonetic_metaphone/multi-word", pluss::make_note({"quick brown fox"}).phonetic_metaphone());

  show_reject("transliterate_greek/reject-bytes",
              [] { (void)pluss::note::bytes({"x"}).transliterate_greek(); });
  show_reject("phonetic_soundex/reject-bytes",
              [] { (void)pluss::note::bytes({"x"}).phonetic_soundex(); });
}

// ------------------------------------------------------------- shape / layout --
// Metadata-only axis rearrangers: the cell strings are untouched, only the
// shape (and, for permute/transpose, the row-major order of the spans) changes.
void section_shape_layout() {
  const auto v6 = pluss::make_note({"a", "b", "c", "d", "e", "f"});
  const auto m23 = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
  const auto s = pluss::make_note("solo");
  const auto iv = [](std::initializer_list<std::int64_t> xs) {
    return std::vector<std::int64_t>(xs);
  };

  // ---- contiguous ----
  show("contiguous/is-noop", m23.transpose(0, 1).contiguous());

  // ---- flatten ----
  show("flatten/2d-to-1d", m23.flatten());
  show("flatten/1d-unchanged", v6.flatten());
  show("flatten/scalar-to-1d", s.flatten());

  // ---- permute ----
  show("permute/2d-swap", m23.permute(iv({1, 0})));
  show("permute/2d-identity", m23.permute(iv({0, 1})));
  show("permute/3d-rotate", v6.reshape(iv({1, 2, 3})).permute(iv({2, 0, 1})));
  show_reject("permute/reject-not-a-permutation",
              [] { (void)pluss::make_note({{"a", "b"}, {"c", "d"}}).permute(std::vector<std::int64_t>{0, 0}); });
  show_reject("permute/reject-wrong-length",
              [] { (void)pluss::make_note({{"a", "b"}, {"c", "d"}}).permute(std::vector<std::int64_t>{0}); });

  // ---- reshape ----
  show("reshape/1d-to-2d", v6.reshape(iv({2, 3})));
  show("reshape/1d-to-3d", v6.reshape(iv({1, 2, 3})));
  show("reshape/2d-to-2d", m23.reshape(iv({3, 2})));
  show("reshape/infer-rows", v6.reshape(iv({-1, 3})));
  show("reshape/infer-cols", v6.reshape(iv({2, -1})));
  show("reshape/to-scalar", s.reshape(iv({1})).reshape(std::vector<std::int64_t>{}));
  show_reject("reshape/reject-mismatch",
              [] { (void)pluss::make_note({"a", "b", "c"}).reshape(std::vector<std::int64_t>{2, 2}); });
  show_reject("reshape/reject-two-inferred",
              [] { (void)pluss::make_note({"a", "b", "c", "d"}).reshape(std::vector<std::int64_t>{-1, -1}); });
  show_reject("reshape/reject-indivisible",
              [] { (void)pluss::make_note({"a", "b", "c", "d", "e"}).reshape(std::vector<std::int64_t>{-1, 2}); });
  show_reject("reshape/reject-4d",
              [] { (void)pluss::make_note({"a", "b"}).reshape(std::vector<std::int64_t>{1, 1, 1, 2}); });

  // ---- squeeze ----
  show("squeeze/drop-leading-1", pluss::make_note({{"x", "y", "z"}}).squeeze(0));
  show("squeeze/drop-trailing-1", v6.reshape(iv({6, 1})).squeeze(1));
  show("squeeze/negative-dim", v6.reshape(iv({1, 6})).squeeze(-2));
  show("squeeze/not-size-1-is-noop", m23.squeeze(0));
  show("squeeze/scalar-is-noop", s.squeeze(0));

  // ---- transpose ----
  show("transpose/2d", m23.transpose(0, 1));
  show("transpose/3d-outer-inner", v6.reshape(iv({1, 2, 3})).transpose(0, 2));
  show("transpose/same-dim-is-noop", m23.transpose(1, 1));
  show("transpose/1d-is-noop", v6.transpose(0, 0));
  show_reject("transpose/reject-out-of-range",
              [] { (void)pluss::make_note({"a", "b"}).transpose(0, 1); });

  // ---- unsqueeze / expand_dims ----
  show("unsqueeze/front", v6.unsqueeze(0));
  show("unsqueeze/back", v6.unsqueeze(1));
  show("unsqueeze/negative-dim", v6.unsqueeze(-1));
  show("unsqueeze/scalar-to-1d", s.unsqueeze(0));
  show("expand_dims/is-the-same", v6.expand_dims(0));
  show_reject("unsqueeze/reject-exceeds-rank-3",
              [] { (void)pluss::make_note({{"a", "b"}}).reshape(std::vector<std::int64_t>{1, 1, 2}).unsqueeze(0); });
}

// -------------------------------------------------------- select / narrow / take
// Pick a subset of cells by position: select drops one axis at an index,
// narrow keeps a contiguous slice of one axis, take gathers by an index list.
void section_select_narrow_take() {
  const auto v6 = pluss::make_note({"a", "b", "c", "d", "e", "f"});
  const auto m23 = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
  const auto t123 = v6.reshape(std::vector<std::int64_t>{1, 2, 3});
  const auto s = pluss::make_note("solo");
  const auto iv = [](std::initializer_list<std::int64_t> xs) {
    return std::vector<std::int64_t>(xs);
  };

  // ---- select ----
  show("select/1d-to-scalar", v6.select(0, 2));
  show("select/2d-row", m23.select(0, 1));
  show("select/2d-column", m23.select(1, 0));
  show("select/negative-dim", m23.select(-1, 2));
  show("select/negative-index", v6.select(0, -1));
  show("select/3d-drops-middle", t123.select(1, 0));
  show_reject("select/reject-index-out-of-range",
              [] { (void)pluss::make_note({"a", "b"}).select(0, 5); });
  show_reject("select/reject-on-scalar",
              [] { (void)pluss::make_note("x").select(0, 0); });

  // ---- narrow ----
  show("narrow/1d-slice", v6.narrow(0, 1, 3));
  show("narrow/2d-rows", m23.narrow(0, 1, 1));
  show("narrow/2d-columns", m23.narrow(1, 0, 2));
  show("narrow/negative-dim", m23.narrow(-1, 1, 2));
  show("narrow/zero-length", v6.narrow(0, 2, 0));
  show("narrow/full-range-unchanged", v6.narrow(0, 0, 6));
  show_reject("narrow/reject-out-of-bounds",
              [] { (void)pluss::make_note({"a", "b", "c"}).narrow(0, 2, 2); });
  show_reject("narrow/reject-negative-start",
              [] { (void)pluss::make_note({"a", "b", "c"}).narrow(0, -1, 1); });

  // ---- take ----
  show("take/gather", v6.take(iv({0, 2, 4})));
  show("take/negative-and-repeat", v6.take(iv({-1, -1, 0})));
  show("take/reverse", v6.take(iv({5, 4, 3, 2, 1, 0})));
  show("take/flattens-2d", m23.take(iv({0, 3, 5})));
  show("take/flattens-3d", t123.take(iv({1, 4})));
  show("take/from-scalar", s.take(iv({0, 0})));
  show("take/empty-index-list", v6.take(std::vector<std::int64_t>{}));
  show_reject("take/reject-out-of-range",
              [] { (void)pluss::make_note({"a", "b"}).take(std::vector<std::int64_t>{5}); });
}

// ---------------------------------------------------------- tokenize_words -----
void section_tokenize_words() {
  show_batch("tokenize_words/drops-punctuation",
             pluss::make_note({"One.\nTwo!", "price=12.50, paid!"}).tokenize_words());
  show_batch("tokenize_words/lowercase-flag",
             pluss::make_note({"Ada LOVELACE", "Grace Hopper"}).tokenize_words(1));
  show_batch("tokenize_words/no-flag-baseline",
             pluss::make_note({"Ada LOVELACE"}).tokenize_words());
  show_batch("tokenize_words/2d-rows",
             pluss::make_note({{"a b", "c d e"}}).tokenize_words());
}

// -------------------------------------------------------------- url_encode -----
void section_url_encode() {
  show("url_encode/reserved-and-space",
       pluss::make_note({"a b&c=d", "path/to/x?y#z"}).url_encode());
  show("url_encode/unreserved-kept",
       pluss::make_note({"A-Za-z0-9-_.~"}).url_encode());
  show("url_encode/utf8-per-byte",
       pluss::make_note({"café", "€"}).url_encode());
  show("url_encode/round-trip",
       pluss::make_note({"a b/c?d", "café"}).url_encode().url_decode());
}

// ---------------------------------------------------------- base64_decode -----
void section_base64_decode() {
  show("base64_decode/basic",
       pluss::make_note({"SGVsbG8=", "Y2Fmw6k=", ""}).base64_decode());
  show("base64_decode/round-trip",
       pluss::make_note({"payload", "café"}).base64_encode().base64_decode());
  show("base64_decode/lenient-on-garbage",
       pluss::make_note({"not!!base64"}).base64_decode());
}

// ------------------------------------------------------------- capitalize -----
void section_capitalize() {
  show("capitalize/1d",
       pluss::make_note({"heLLo wORLd", "ALL CAPS", "3rd place", "café MODE"}).capitalize());
  show("capitalize/scalar", pluss::make_note("mIxEd").capitalize());
  show("capitalize/leading-space-or-digit",
       pluss::make_note({"  spaced", "42 answers"}).capitalize());
  show("capitalize/2d", pluss::make_note({{"aB", "cD"}}).capitalize());
}

// --------------------------------------------------------------- casefold -----
void section_casefold() {
  show("casefold/1d",
       pluss::make_note({"HeLLo", "ΑΒΓ", "STRASSE"}).casefold());
  show("casefold/vs-lower-sharp-s",
       pluss::make_note({"straße"}).casefold());
  show("casefold/locale",
       pluss::make_note({"STRASSE ẞ"}).casefold("de"));
  show("casefold/scalar", pluss::make_note("MiXeD").casefold());
}

// -------------------------------------------------------- count_substring -----
void section_count_substring() {
  show_lens("count_substring/basic",
            pluss::make_note({"banana", "aaaa", "xyz", ""}).count_substring("a"));
  show_lens("count_substring/non-overlapping",
            pluss::make_note({"aaaa"}).count_substring("aa"));
  show_lens("count_substring/empty-needle-is-zero",
            pluss::make_note({"abc"}).count_substring(""));
  show_lens("count_substring/multi-char",
            pluss::make_note({"a-b-c-d", "no dashes"}).count_substring("-"));
}

// --------------------------------------------------------------- ends_with -----
void section_ends_with() {
  show_mask("ends_with/basic",
            pluss::make_note({"report.md", "notes.MD", "data.csv"}).ends_with(".md"));
  show_mask("ends_with/empty-suffix",
            pluss::make_note({"x", ""}).ends_with(""));
  show_mask("ends_with/suffix-longer-than-cell",
            pluss::make_note({"ab"}).ends_with("abcd"));
  show_mask("ends_with/2d-flattened",
            pluss::make_note({{"a.py", "b.py"}, {"c.js", "d.py"}}).ends_with(".py"));
}

// ------------------------------------------------------------- hex_decode -----
void section_hex_decode() {
  show("hex_decode/basic", pluss::make_note({"486921", "636166c3a9"}).hex_decode());
  show("hex_decode/round-trip",
       pluss::make_note({"Hi!", "café"}).hex_encode().hex_decode());
}

// ------------------------------------------------------------- hex_encode -----
void section_hex_encode() {
  show("hex_encode/1d",
       pluss::make_note({"Hi!", "café", ""}).hex_encode());
  show("hex_encode/scalar", pluss::make_note("\n\t").hex_encode());
  show("hex_encode/round-trip",
       pluss::make_note({"payload", "€"}).hex_encode().hex_decode());
}

// ----------------------------------------------------------- lengths_chars -----
void section_lengths_chars() {
  show_lens("lengths_chars/ascii", pluss::make_note({"", "a", "abc"}).lengths_chars());
  show_lens("lengths_chars/utf8",
            pluss::make_note({"café", "日本語", "🎯"}).lengths_chars());
  show_lens("lengths_chars/vs-bytes",
            pluss::make_note({"café"}).lengths_bytes());
  show_lens("lengths_chars/2d",
            pluss::make_note({{"a", "bb"}, {"日本", "x"}}).lengths_chars());
}

// ------------------------------------------------------ lengths_graphemes -----
void section_lengths_graphemes() {
  // "e" + U+0301 combining acute: one grapheme, two code points.
  show_lens("lengths_graphemes/combining-mark",
            pluss::make_note({std::string("e\xCC\x81"), "abc"}).lengths_graphemes());
  show_lens("lengths_graphemes/vs-chars",
            pluss::make_note({std::string("e\xCC\x81")}).lengths_chars());
  show_lens("lengths_graphemes/precomposed",
            pluss::make_note({"café", "日本語"}).lengths_graphemes());
}

// ------------------------------------------------------------- starts_with -----
void section_starts_with() {
  show_mask("starts_with/basic",
            pluss::make_note({"the end", "The end", "in the end"}).starts_with("the"));
  show_mask("starts_with/empty-prefix",
            pluss::make_note({"x", ""}).starts_with(""));
  show_mask("starts_with/prefix-longer-than-cell",
            pluss::make_note({"ab"}).starts_with("abcd"));
  show_mask("starts_with/2d-flattened",
            pluss::make_note({{"img_1", "img_2"}, {"vid_1", "img_3"}}).starts_with("img_"));
}

// --------------------------------------------------------------- swapcase -----
void section_swapcase() {
  show("swapcase/1d",
       pluss::make_note({"Hello World", "ALL caps", "café MODE"}).swapcase());
  show("swapcase/involution",
       pluss::make_note({"MiXeD CaSe"}).swapcase().swapcase());
  show("swapcase/scalar", pluss::make_note("aBcD").swapcase());
}

// ------------------------------------------------------------------ title -----
void section_title() {
  show("title/1d",
       pluss::make_note({"the quick brown fox", "ALL CAPS HERE", "café au lait"}).title());
  show("title/digit-and-underscore-boundaries",
       pluss::make_note({"3rd place", "snake_case_name"}).title());
  show("title/scalar", pluss::make_note("hello world").title());
}

// ----------------------------------------------------------------- unquote -----
void section_unquote() {
  show("unquote/double-and-single",
       pluss::make_note({"\"double\"", "'single'", "bare"}).unquote());
  show("unquote/only-matching-pair-removed",
       pluss::make_note({"\"unbalanced", "trailing\"", "\"\"", "\"a\"b\""}).unquote());
  show("unquote/round-trip-with-quote",
       pluss::make_note({"payload", "  spaced  "}).quote().unquote());
  show("unquote/scalar", pluss::make_note("\"solo\"").unquote());
}

// ------------------------------------------------------------------- upper -----
void section_upper() {
  show("upper/1d",
       pluss::make_note({"  Ada lovelace  ", "grace HOPPER", "café"}).upper());
  show("upper/scalar", pluss::make_note("HeLLo, world").upper());
  show("upper/2d", pluss::make_note({{"ab", "cD"}, {"Ef", "gh"}}).upper());
  show("upper/unicode", pluss::make_note({"café", "αβγ", "ß"}).upper());
  show("upper/idempotent", pluss::make_note({"ALREADY"}).upper().upper());
}

// ------------------------------------------------------------- url_decode -----
void section_url_decode() {
  show("url_decode/percent-escapes",
       pluss::make_note({"a%20b%26c%3Dd", "caf%C3%A9"}).url_decode());
  show("url_decode/plus-becomes-space",
       pluss::make_note({"a+b+c"}).url_decode());
  show("url_decode/round-trip",
       pluss::make_note({"a b/c?d", "café"}).url_encode().url_decode());
  show("url_decode/unescaped-passes-through",
       pluss::make_note({"already-clean_text.~"}).url_decode());
}

// ---------------------------------------------------------- ensure_prefix -----
void section_ensure_prefix() {
  show("ensure_prefix/basic",
       pluss::make_note({"path", "/path", "p/ath"}).ensure_prefix("/"));
  show("ensure_prefix/whole-prefix-must-be-present",
       pluss::make_note({"htt", "http://x"}).ensure_prefix("http://"));
  show("ensure_prefix/scalar", pluss::make_note("users").ensure_prefix("/api/"));
}

// ---------------------------------------------------------- ensure_suffix -----
void section_ensure_suffix() {
  show("ensure_suffix/basic",
       pluss::make_note({"dir", "dir/", "d"}).ensure_suffix("/"));
  show("ensure_suffix/2d",
       pluss::make_note({{"a", "b.txt"}}).ensure_suffix(".txt"));
}

// ---------------------------------------------------------- regex_replace -----
void section_regex_replace() {
  show("regex_replace/all-matches",
       pluss::make_note({"a1b22c333", "none"}).regex_replace(R"(\d+)", "#"));
  show("regex_replace/capture-back-references",
       pluss::make_note({"2026-08-02"}).regex_replace(
           R"((\d{4})-(\d{2})-(\d{2}))", "$3/$2/$1"));
  show("regex_replace/case-insensitive-flag",
       pluss::make_note({"Hello HELLO hello"}).regex_replace("hello", "hi", 1));
  show("regex_replace/no-flag-baseline",
       pluss::make_note({"Hello HELLO hello"}).regex_replace("hello", "hi"));
  show_reject("regex_replace/reject-bad-pattern",
              [] { (void)pluss::make_note({"x"}).regex_replace("(", "y"); });
}

// ------------------------------------------------------------------ remove -----
void section_remove() {
  show("remove/first-occurrence-only",
       pluss::make_note({"alpha beta beta", "beta", "none"}).remove("beta"));
  show("remove/vs-remove_all",
       pluss::make_note({"a-b-c"}).remove("-"));
}

// ----------------------------------------------------------- remove_prefix -----
void section_remove_prefix() {
  show("remove_prefix/basic",
       pluss::make_note({"src/main.cpp", "docs/x", "src/src/y"}).remove_prefix("src/"));
  show("remove_prefix/absent-unchanged",
       pluss::make_note({"abc"}).remove_prefix("xyz"));
}

// ----------------------------------------------------------- remove_suffix -----
void section_remove_suffix() {
  show("remove_suffix/basic",
       pluss::make_note({"a.md", "a.md.md", "a.txt"}).remove_suffix(".md"));
  show("remove_suffix/absent-unchanged",
       pluss::make_note({"report"}).remove_suffix(".md"));
}

// ----------------------------------------------------------- replace_first -----
void section_replace_first() {
  const auto n = pluss::make_note({"alpha beta beta gamma", "beta", "no match"});
  show("replace_first/basic", n.replace_first("beta", "X"));
  show("replace_first/empty-from-is-a-no-op",
       pluss::make_note({"abc"}).replace_first("", "X"));
  show("replace_first/no-match-unchanged",
       pluss::make_note({"abc"}).replace_first("z", "X"));
}

// ------------------------------------------------------------ replace_last -----
void section_replace_last() {
  const auto n = pluss::make_note({"alpha beta beta gamma", "beta", "no match"});
  show("replace_last/basic", n.replace_last("beta", "X"));
}

// --------------------------------------------------------------- replace_n -----
void section_replace_n() {
  const auto n = pluss::make_note({"alpha beta beta beta", "beta"});
  show("replace_n/first-two", n.replace_n("beta", "X", 2));
  show("replace_n/zero-is-a-no-op", n.replace_n("beta", "X", 0));
  show("replace_n/count-above-available", n.replace_n("beta", "X", 99));
}

// ----------------------------------------------------------------- surround -----
void section_surround() {
  show("surround/brackets",
       pluss::make_note({"x", "", "a]"}).surround("[", "]"));
  show("surround/same-both-sides",
       pluss::make_note({"mid"}).surround("**", "**"));
  show("surround/2d", pluss::make_note({{"a", "b"}}).surround("<", ">"));
}

// ---------------------------------------------------- character predicates -----
// The is_* / contains_any_* / regex predicates return a flat
// std::vector<std::uint8_t> of 0/1, one per cell. Inputs match the per-page
// examples 1:1.
void section_char_predicates() {
  show_mask("is_alpha/1d",
            pluss::make_note({"abc", "ABC", "Abc", "abc123", "123", "", "a b", "café"}).is_alpha());
  show_mask("is_alnum/1d",
            pluss::make_note({"abc", "abc123", "123", "abc_123", "", "a b"}).is_alnum());
  show_mask("is_digit/1d",
            pluss::make_note({"123", "007", "12.5", "-3", "1 2", "", "12a"}).is_digit());
  show_mask("is_lower/1d",
            pluss::make_note({"abc", "ABC", "Abc", "abc123", "123", "", "a b", "café"}).is_lower());
  show_mask("is_upper/1d",
            pluss::make_note({"ABC", "abc", "Abc", "ABC123", "A B", "123", ""}).is_upper());
  show_mask("is_title/1d",
            pluss::make_note({"Two Words", "Abc", "TwoWords", "two words", "TWO", "123", ""}).is_title());
  show_mask("is_whitespace/1d",
            pluss::make_note({"  ", "\t\n", "", " a ", "abc"}).is_whitespace());
  show_mask("is_blank/1d",
            pluss::make_note({"", "  ", "\t", " x ", "x"}).is_blank());
  show_mask("is_ascii/1d",
            pluss::make_note({"plain", "", "café", "日本語", "a\tb"}).is_ascii());
  show_mask("is_utf8_valid/bytes-note",
            pluss::note::bytes({"abc", "café", "\xFF\xFE"}).is_utf8_valid());
  show_flag("is_bytes/utf8-note", pluss::make_note({"a", "b", "c"}).is_bytes());
  show_flag("is_bytes/bytes-note", pluss::note::bytes({"a", "b"}).is_bytes());
  const auto valid_raw = pluss::note::bytes({"plain ASCII", "caf\xC3\xA9"});
  std::cout << "\n=== is_bytes/valid-utf8-bytes ===\n"
            << std::boolalpha << "bytes: " << valid_raw.is_bytes() << '\n'
            << "valid: ";
  const auto valid_lanes = valid_raw.is_utf8_valid().bools();
  for (std::size_t i = 0; i < valid_lanes.size(); ++i) {
    std::cout << (i ? " " : "") << static_cast<int>(valid_lanes[i]);
  }
  std::cout << '\n';
  show_mask("contains_any_alpha/1d",
            pluss::make_note({"abc", "abc123", "123", "-42-", "", "a1"}).contains_any_alpha());
  show_mask("contains_any_digit/1d",
            pluss::make_note({"order 17", "abc", "v2", "", "3.14", "no numbers"}).contains_any_digit());
  show_mask("regex_match/anchored",
            pluss::make_note({"2026", "year 2026", "none"}).regex_match(R"(\d+)"));
  show_mask("regex_match/wildcards",
            pluss::make_note({"2026", "year 2026", "none"}).regex_match(R"(.*\d+.*)"));
  show_mask("regex_search/unanchored",
            pluss::make_note({"2026", "year 2026", "none"}).regex_search(R"(\d+)"));
  show_reject("regex_match/reject-bad-pattern",
              [] { (void)pluss::make_note({"x"}).regex_match("("); });
}

// ----------------------------------------------------- character-class filters -----
// Each keeps or drops one class of characters. Inputs match the per-page
// examples 1:1.
void section_char_filters() {
  show("remove_punctuation/1d",
       pluss::make_note({"Hello, World! 42", "a_b-c.d", "café #99"}).remove_punctuation());
  show("remove_punctuation/2d",
       pluss::make_note({{"a, b", "c.d"}}).remove_punctuation());
  show("remove_digits/1d",
       pluss::make_note({"v2.1 build 405", "no digits", "2026"}).remove_digits());
  show("remove_letters/1d",
       pluss::make_note({"Order 17!", "café", "42.0"}).remove_letters());
  show("remove_symbols/1d",
       pluss::make_note({"100% (sure)!", "a_b-c", "café #9"}).remove_symbols());
  show("remove_symbols/keeps-non-ascii",
       pluss::make_note({"café — résumé"}).remove_symbols());
  show("remove_emojis/1d",
       pluss::make_note({"ship it 🚀 today 🎉", "plain text", "👍👍👍"}).remove_emojis());
  show("keep_alnum/1d",
       pluss::make_note({"Hello, World! 42", "  x-y  ", "café"}).keep_alnum());
  show("keep_alpha/1d",
       pluss::make_note({"abc123", "A B C", "café", "42"}).keep_alpha());
  show("keep_digits/1d",
       pluss::make_note({"order 17", "v2.1.3", "none", ""}).keep_digits());
  show("keep_ascii/1d",
       pluss::make_note({"café résumé", "plain", "日本語 text"}).keep_ascii());
  show("keep_printable/controls",
       pluss::make_note({"line1\tcol2\nline2", "clean"}).keep_printable());
  show("keep_printable/non-ascii",
       pluss::make_note({"café — ok"}).keep_printable());
}

// --------------------------------------------------- whitespace / layout -----
void section_whitespace_layout() {
  show("collapse_whitespace/1d",
       pluss::make_note({"  hello   world  ", "a\t\tb\nc", "clean"}).collapse_whitespace());
  show("collapse_whitespace/2d",
       pluss::make_note({{"  a  b  ", " c "}}).collapse_whitespace());
  show("collapse_spaces/1d",
       pluss::make_note({"a   b     c", "keep\nthe\nnewlines"}).collapse_spaces());
  show("collapse_newlines/1d",
       pluss::make_note({"para1\n\n\n\npara2", "a\nb"}).collapse_newlines());
  show("collapse_repeats/1d",
       pluss::make_note({"woooow!!!", "aaabbbccc", "book"}).collapse_repeats());
  show("collapse_punct/1d",
       pluss::make_note({"wait...", "really?!?!", "a--b__c"}).collapse_punct());
  show("dedent/1d",
       pluss::make_note({"    line1\n    line2\n      line3", "no indent"}).dedent());
  show("indent/1d",
       pluss::make_note({"line1\nline2", "solo"}).indent());
  show("lstrip/1d",
       pluss::make_note({"  hello  ", "\tworld\t", "clean"}).lstrip());
  show("lstrip/chars",
       pluss::make_note({"xxhello", "--world"}).lstrip("x-"));
  show("rstrip/1d",
       pluss::make_note({"  hello  ", "\tworld\t", "clean"}).rstrip());
  {
    const std::string bell = std::string("alert") + '\x07' + "text\tkept\nkept café";
    show("remove_control_chars/1d",
         pluss::make_note({bell, "clean"}).remove_control_chars());
  }
  show("to_ascii/1d",
       pluss::make_note({"café résumé", "naïve", "日本語", "plain"}).to_ascii());
}

// ----------------------------------------------------- unicode normalization -----
// to_nfc / to_nfd change the byte encoding without changing the rendered text,
// so the examples pair print() with lengths_bytes() / lengths_chars() to expose
// the difference.
void section_unicode_normalization() {
  const std::string e_acute_decomposed = std::string("caf") + "e\xCC\x81";  // e + U+0301

  show("to_nfc/composes",
       pluss::make_note({e_acute_decomposed, "abc"}).to_nfc());
  show_lens("to_nfc/composes bytes",
            pluss::make_note({e_acute_decomposed, "abc"}).to_nfc().lengths_bytes());
  show_lens("to_nfc/input bytes",
            pluss::make_note({e_acute_decomposed, "abc"}).lengths_bytes());

  show("to_nfd/decomposes",
       pluss::make_note({"café", "abc"}).to_nfd());
  show_lens("to_nfd/decomposes bytes",
            pluss::make_note({"café", "abc"}).to_nfd().lengths_bytes());
  show_lens("to_nfd/decomposes chars",
            pluss::make_note({"café", "abc"}).to_nfd().lengths_chars());

  const std::string compat = std::string("\xEF\xAC\x81") + "le " + "\xC2\xB2";  // ﬁ + " le " + ²
  show("to_nfkc/compat-fold", pluss::make_note({compat, "plain"}).to_nfkc());
  show("to_nfkd/compat-fold", pluss::make_note({compat, "plain"}).to_nfkd());

  show("strip_accents/1d",
       pluss::make_note({"café résumé", "naïve", "señor"}).strip_accents());
  show("strip_accents/latin-extended-limit",
       pluss::make_note({"ĥăçķ ō"}).strip_accents());
  show("strip_accents/decompose-first",
       pluss::make_note({"ĥăçķ ō"}).to_nfd().strip_accents());

  show("strip_zero_width/1d",
       pluss::make_note({std::string("a") + "\xE2\x80\x8B" + "b" + "\xEF\xBB\xBF" + "c", "clean"})
           .strip_zero_width());
  show("strip_bom/1d",
       pluss::make_note({std::string("\xEF\xBB\xBF") + "hello", "plain",
                         std::string("mid") + "\xEF\xBB\xBF" + "bom"})
           .strip_bom());

  show("normalize_quotes/1d",
       pluss::make_note({std::string("\xE2\x80\x9C") + "hi" + "\xE2\x80\x9D" + " it" +
                             "\xE2\x80\x99" + "s",
                         "plain"})
           .normalize_quotes());
  show("normalize_dashes/1d",
       pluss::make_note({std::string("a ") + "\xE2\x80\x93" + " b " + "\xE2\x80\x94" + " c",
                         "plain"})
           .normalize_dashes());
  show("normalize_ellipsis/1d",
       pluss::make_note({std::string("wait") + "\xE2\x80\xA6" + " more", "plain"})
           .normalize_ellipsis());
}

}  // namespace

int main() {
  section_diff_align();
  section_set_similarity();
  section_encoding_transcode();
  section_base64_decode();
  section_base64_encode();
  section_capitalize();
  section_casefold();
  section_char_filters();
  section_whitespace_layout();
  section_unicode_normalization();
  section_char_predicates();
  section_contains();
  section_count_substring();
  section_ends_with();
  section_ensure_prefix();
  section_ensure_suffix();
  section_hex_decode();
  section_hex_encode();
  section_lengths_bytes();
  section_lengths_chars();
  section_lengths_graphemes();
  section_lower();
  section_nlp_mask_pii();
  section_nlp_remove_urls();
  section_quote();
  section_regex_findall();
  section_regex_replace();
  section_remove();
  section_remove_all();
  section_remove_prefix();
  section_remove_suffix();
  section_replace();
  section_replace_first();
  section_replace_last();
  section_replace_n();
  section_split();
  section_split_lines();
  section_split_whitespace();
  section_starts_with();
  section_strip();
  section_surround();
  section_swapcase();
  section_tex_escape();
  section_tex_to_text();
  section_title();
  section_tokenize_words();
  section_transliteration_phonetics();
  section_shape_layout();
  section_select_narrow_take();
  section_unquote();
  section_upper();
  section_url_decode();
  section_url_encode();
  return 0;
}
