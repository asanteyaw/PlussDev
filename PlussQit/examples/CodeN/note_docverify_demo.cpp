#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

namespace {

void print_marker(::std::string_view label) {
  std::cout << "== " << label << " ==\n";
}

// contains()/is_*() predicates and lengths_*()/count_*() measurements return
// a shape-preserving Bool/Int64 Note (passes 4-5 of the Note redesign), not
// a raw std::vector.
void print_mask(::std::string_view label, const pluss::Note& mask) {
  print_marker(label);
  std::cout << label << ":";
  for (std::uint8_t v : mask.bools()) {
    std::cout << " " << static_cast<int>(v);
  }
  std::cout << "\n";
}

void print_lengths(::std::string_view label, const pluss::Note& lengths) {
  print_marker(label);
  std::cout << label << ":";
  for (std::int64_t v : lengths.integers()) {
    std::cout << " " << v;
  }
  std::cout << "\n";
}

void print_lines(::std::string_view label, const pluss::Note& note) {
  print_marker(label);
  std::cout << label << ":\n";
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    std::cout << "  [" << i << "] " << note.at(i) << "\n";
  }
}

void print_batch(::std::string_view label, const pluss::coden::TokenBatch& batch) {
  print_marker(label);
  std::cout << label << " tokens:";
  for (std::int64_t i = 0; i < batch.tokens().numel(); ++i) {
    std::cout << " [" << batch.tokens().at(i) << "]";
  }
  std::cout << "\n";
  std::cout << label << " offsets:";
  for (std::int64_t off : batch.offsets().integers()) {
    std::cout << " " << off;
  }
  std::cout << "\n";
}

} // namespace

int main() {
  {
    auto note = pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"});
    print_lines("lower", note.lower());
    print_lines("lower_strip", note.strip().lower());
    print_lines("strip", note.strip());
    print_lines("strip_hash_space", pluss::make_note({"##alpha##", "#beta", "gamma##"}).strip("#"));
    print_lines("strip_boundary_only", pluss::make_note({"#alpha#beta#", "##gamma##"}).strip("#"));
  }

  {
    auto note = pluss::make_note({"midterm_1", "final_exam", "quiz"});
    print_mask("contains_exam", note.contains("exam"));
    print_mask("contains_exam_lower", pluss::make_note({"Exam 1", "final exam", "Quiz"}).lower().contains("exam"));
    print_lengths("lengths_bytes", note.lengths_bytes());
    print_lengths("lengths_bytes_strip", pluss::make_note({"  id-001  ", "\tAB\t"}).strip().lengths_bytes());
  }

  {
    auto note = pluss::make_note({"score: 75", "score: 88", "pending"});
    print_lines("replace", note.replace("score", "mark"));
    print_lines("replace_underscore", pluss::make_note({"Ada_Lovelace", "Grace_Hopper", "Linus"}).replace("_", " "));
    auto renamed = pluss::make_note({"midterm_1", "midterm_2", "final_exam"}).replace("midterm", "exam");
    print_lines("replace_then_contains", renamed);
    print_mask("replace_then_contains_mask", renamed.contains("midterm"));
    print_lines("remove_all", pluss::make_note({"alpha beta beta", "beta only"}).remove_all("beta"));
    print_lines("remove_all_strip", pluss::make_note({"##alpha##", "##beta"}).remove_all("#").strip());
    print_lines("remove_all_marker", pluss::make_note({"id-001-id", "id-204"}).remove_all("id-"));
    print_lines("quote", pluss::make_note({"alpha beta beta", "already"}).quote());
    print_lines("quote_strip", pluss::make_note({"  spaced  ", ""}).strip().quote());
    print_lines("quote_display", pluss::make_note({"Ada_Lovelace", "Grace_Hopper"}).replace("_", " ").quote());
  }

  {
    auto note = pluss::make_note({"alpha   beta\tgamma", "delta epsilon zeta"});
    print_batch("split_whitespace", note.split_whitespace());
    print_batch("split_whitespace_limit1", pluss::make_note({"alpha beta gamma", "delta epsilon"}).split_whitespace(1));
    print_batch("split_whitespace_strip", pluss::make_note({"  Ada Lovelace  ", "\tGrace Hopper\t"}).strip().split_whitespace());
  }

  {
    auto note = pluss::make_note({"line1\nline2\n", "solo"});
    print_batch("split_lines", note.split_lines());
    print_batch("split_lines_keep", note.split_lines(true));
    print_batch("split_comma_limit1", pluss::make_note({"A,b;c", "x,y,z"}).split(",", 1));
    print_batch("split_colon", pluss::make_note({"k:v:w", "id:42"}).split(":"));
    print_batch("split_arrow", pluss::make_note({"left -> right", "keep -> watch"}).split(" -> "));
  }

  {
    auto note = pluss::make_note({"Order 17 ships on 2026-08-02", "Ticket 204 closes at 18:30"});
    print_batch("regex_findall_digits", note.regex_findall(R"(\d+)"));
    print_batch("regex_findall_caps", pluss::make_note({"AX-17", "BY_204"}).regex_findall(R"([A-Z]+)"));
    print_batch("regex_findall_tags", pluss::make_note({"#alpha update", "ship #beta now"}).regex_findall(R"(#\w+)"));
    print_batch("tokenize_words", pluss::make_note({"One.\nTwo!", "A,b;c"}).tokenize_words());
    print_batch("tokenize_words_lower", pluss::make_note({"Ada LOVELACE", "Grace Hopper"}).lower().tokenize_words());
    print_batch("tokenize_words_punct", pluss::make_note({"price=12.50, paid!", "ref:A-17"}).tokenize_words());
  }

  {
    auto dirty = pluss::make_note({
        "Visit https://pluss.dev and email team@pluss.dev",
        "Mirror http://docs.pluss.dev for release notes",
    });
    print_lines("nlp_remove_urls", dirty.nlp_remove_urls());
    print_mask("nlp_remove_urls_contains_http", dirty.nlp_remove_urls().contains("http"));
  }

  {
    auto tex = pluss::make_note({"Price is 50% of $x+y$", R"(Use \alpha and \beta.)"});
    print_lines("tex_escape", tex.tex_escape());
    print_lines("tex_escape_plain", pluss::make_note({"chapter title", "plain text"}).tex_escape());
    print_lines(
        "tex_to_text",
        pluss::make_note(
            "\\documentclass{article}\n"
            "% comment\n"
            "\\begin{document}\n"
            "Text \\textbf{bold} and $x+y$.\n"
            "\\label{l1}\\ref{l1}\\cite{k}\\href{https://x.y}{link}\n"
            "\\end{document}\n")
            .tex_to_text());
    print_lines("tex_to_text_inline", pluss::make_note("\\textit{Hello}").tex_to_text());
  }

  {
    auto pii = pluss::make_note({
        "Email bob@example.com at 9:30am on 2025-01-03, pay $12.50 to @alice #ops",
        "Backup contact: jane@pluss.dev",
    });
    print_lines("nlp_mask_pii", pii.nlp_mask_pii());
    print_mask("nlp_mask_pii_contains_email", pii.nlp_mask_pii().contains("[EMAIL]"));
  }

  {
    auto codec = pluss::make_note("Hi!");
    print_lines("base64_encode", codec.base64_encode());
    print_lines("base64_roundtrip", codec.base64_encode().base64_decode());
    print_lines("base64_encode_batch", pluss::make_note({"Hi!", "Pluss"}).base64_encode());
    print_lines("url_encode", pluss::make_note("a b+c").url_encode());
    print_lines("url_roundtrip", pluss::make_note("a b+c").url_encode().url_decode());
    print_lines("url_encode_batch", pluss::make_note({"a b+c", "x/y"}).url_encode());
  }

  print_marker("end");
  return 0;
}
