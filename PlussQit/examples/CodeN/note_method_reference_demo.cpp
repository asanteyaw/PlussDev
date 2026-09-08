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

void show_mask(std::string_view label, const std::vector<std::uint8_t>& mask) {
  std::cout << "\n=== " << label << " ===\n[";
  for (std::size_t i = 0; i < mask.size(); ++i) {
    std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
  }
  std::cout << "]\n";
}

void show_lens(std::string_view label, const std::vector<std::int64_t>& lens) {
  std::cout << "\n=== " << label << " ===\n[";
  for (std::size_t i = 0; i < lens.size(); ++i) {
    std::cout << (i ? ", " : "") << lens[i];
  }
  std::cout << "]\n";
}

// A TokenBatch is a flat token Note plus an offsets vector. Print both, then
// the per-input-cell slices the offsets describe.
void show_batch(std::string_view label, const pluss::coden::TokenBatch& batch) {
  std::cout << "\n=== " << label << " ===\n"
            << "tokens: " << pluss::note::print(batch.tokens()) << '\n'
            << "offsets: [";
  const auto& off = batch.offsets();
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

}  // namespace

int main() {
  section_base64_decode();
  section_base64_encode();
  section_capitalize();
  section_casefold();
  section_contains();
  section_count_substring();
  section_ends_with();
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
  section_remove_all();
  section_replace();
  section_split();
  section_split_lines();
  section_split_whitespace();
  section_starts_with();
  section_strip();
  section_swapcase();
  section_tex_escape();
  section_tex_to_text();
  section_title();
  section_tokenize_words();
  section_unquote();
  section_upper();
  section_url_decode();
  section_url_encode();
  return 0;
}
