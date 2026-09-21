#include <pluss/coden.h>
#include <pluss/coden/note_factory.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
  pluss::Note scalar = pluss::make_note("hello");
  pluss::Note one_d = pluss::make_note({"a", "b", "c"});
  pluss::Note two_d = pluss::make_note({{"a", "b"}, {"c", "d"}});
  pluss::Note three_d = pluss::make_note({{{"a"}, {"b"}}, {{"c"}, {"d"}}});

  std::vector<std::string> flat = {"p", "q", "r", "s"};
  const std::array<std::int64_t, 2> sizes{2, 2};
  pluss::Note from_flat = pluss::make_note(flat, std::span<const std::int64_t>(sizes.data(), sizes.size()));

  pluss::Note from_lines = pluss::note::from_lines("line1\nline2\nline3");
  pluss::Note from_words = pluss::note::from_words("alpha beta gamma");

  pluss::Note filled = pluss::note::full({2, 1, 2}, "z");
  pluss::Note empty = pluss::note::empty({2, 2});
  pluss::Note like = pluss::note::full_like(two_d, "x");
  pluss::Note repeated = pluss::note::rep("Alex", {2, 2});
  pluss::Note random_upper = pluss::note::rand({8}, true, 7);
  pluss::Note random_between = pluss::note::rand_between("D", "H", {6}, 7);
  pluss::Note alpha_range = pluss::note::arange("A", "H", 2);
  pluss::Note combos = pluss::note::perm("hypocrite", 2);
  pluss::Note perms = pluss::note::perm(
      "aba", 2, pluss::note::WordSelectMode::Permutation, false, true);
  int weight = 175;
  double height = 6.45;
  pluss::Note formatted = pluss::note::format(
      "This guy has weight {weight} and height {height}, thus his BMI is {weight * height / 2} right now",
      {
          {"weight", weight},
          {"height", height},
      });

  pluss::Note bytes = pluss::note::bytes({{"\x01", "\x02"}, {"\x03", "\x04"}});

  std::cout << "scalar:\n" << pluss::note::print(scalar) << "\n\n";
  std::cout << "one_d:\n" << pluss::note::print(one_d) << "\n\n";
  std::cout << "two_d:\n" << pluss::note::print(two_d) << "\n\n";
  std::cout << "three_d:\n" << pluss::note::print(three_d) << "\n\n";
  std::cout << "from_flat:\n" << pluss::note::print(from_flat) << "\n\n";
  std::cout << "from_lines:\n" << pluss::note::print(from_lines) << "\n\n";
  std::cout << "from_words:\n" << pluss::note::print(from_words) << "\n\n";
  std::cout << "filled:\n" << pluss::note::print(filled) << "\n\n";
  std::cout << "empty:\n" << pluss::note::print(empty) << "\n\n";
  std::cout << "like:\n" << pluss::note::print(like) << "\n\n";
  std::cout << "repeated:\n" << pluss::note::print(repeated) << "\n\n";
  std::cout << "random_upper:\n" << pluss::note::print(random_upper) << "\n\n";
  std::cout << "random_between:\n" << pluss::note::print(random_between) << "\n\n";
  std::cout << "alpha_range:\n" << pluss::note::print(alpha_range) << "\n\n";
  std::cout << "combos:\n" << pluss::note::print(combos) << "\n\n";
  std::cout << "perms:\n" << pluss::note::print(perms) << "\n\n";
  std::cout << "formatted:\n" << pluss::note::print(formatted) << "\n\n";

  pluss::Note lower = two_d.lower();
  pluss::Note escaped = three_d.tex().escape();
  std::cout << "lower:\n" << pluss::note::print(lower) << "\n\n";
  std::cout << "escaped:\n" << pluss::note::print(escaped) << "\n\n";

  auto tokens = from_lines.split_lines().tokens();
  std::cout << "tokens from split_lines:\n" << pluss::note::print(tokens) << "\n\n";

  pluss::NotePrintOptions bytes_opt;
  bytes_opt.bytes_mode = pluss::coden::NotePrintBytesMode::Hex;
  std::cout << "bytes (hex):\n" << pluss::note::print(bytes, bytes_opt) << "\n\n";

  return 0;
}
