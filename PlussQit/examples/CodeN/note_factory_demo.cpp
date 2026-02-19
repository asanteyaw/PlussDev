#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
  pluss::Note scalar = pluss::content("hello");
  pluss::Note one_d = pluss::content({"a", "b", "c"});
  pluss::Note two_d = pluss::content({{"a", "b"}, {"c", "d"}});
  pluss::Note three_d = pluss::content({{{"a"}, {"b"}}, {{"c"}, {"d"}}});

  std::vector<std::string> flat = {"p", "q", "r", "s"};
  const std::array<std::int64_t, 2> sizes{2, 2};
  pluss::Note from_flat = pluss::make_note(flat, std::span<const std::int64_t>(sizes.data(), sizes.size()));

  pluss::Note from_lines = pluss::note_from_lines("line1\nline2\nline3");
  pluss::Note from_words = pluss::note_from_words("alpha beta gamma");

  pluss::Note filled = pluss::full_note({2, 1, 2}, "z");
  pluss::Note empty = pluss::empty_note({2, 2});
  pluss::Note like = pluss::full_note_like(two_d, "x");

  pluss::Note bytes = pluss::bytes_content({{"\x01", "\x02"}, {"\x03", "\x04"}});

  std::cout << "scalar:\n" << pluss::print_note(scalar) << "\n\n";
  std::cout << "one_d:\n" << pluss::print_note(one_d) << "\n\n";
  std::cout << "two_d:\n" << pluss::print_note(two_d) << "\n\n";
  std::cout << "three_d:\n" << pluss::print_note(three_d) << "\n\n";
  std::cout << "from_flat:\n" << pluss::print_note(from_flat) << "\n\n";
  std::cout << "from_lines:\n" << pluss::print_note(from_lines) << "\n\n";
  std::cout << "from_words:\n" << pluss::print_note(from_words) << "\n\n";
  std::cout << "filled:\n" << pluss::print_note(filled) << "\n\n";
  std::cout << "empty:\n" << pluss::print_note(empty) << "\n\n";
  std::cout << "like:\n" << pluss::print_note(like) << "\n\n";

  pluss::Note lower = two_d.lower();
  pluss::Note escaped = three_d.tex_escape();
  std::cout << "lower:\n" << pluss::print_note(lower) << "\n\n";
  std::cout << "escaped:\n" << pluss::print_note(escaped) << "\n\n";

  auto tokens = from_lines.split_lines().tokens();
  std::cout << "tokens from split_lines:\n" << pluss::print_note(tokens) << "\n\n";

  pluss::NotePrintOptions bytes_opt;
  bytes_opt.bytes_mode = pluss::coden::NotePrintBytesMode::Hex;
  std::cout << "bytes (hex):\n" << pluss::print_note(bytes, bytes_opt) << "\n\n";

  return 0;
}
