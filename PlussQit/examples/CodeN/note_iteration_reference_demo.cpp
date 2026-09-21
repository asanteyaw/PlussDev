#include <pluss/pluss.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

template <class Range>
void show_quoted(const Range& values) {
  std::cout << '[';
  std::size_t index = 0;
  for (const auto value : values)
    std::cout << (index++ ? ", " : "") << '"' << value << '"';
  std::cout << ']';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

} // namespace

int main() {
  const auto words = pluss::note::make({"north", "east", "south", "west"});
  std::cout << "=== range iteration ===\n";
  show_quoted(words);
  std::cout << " count=" << std::distance(words.begin(), words.end()) << '\n';
  for (auto it = words.begin(); it != words.end(); ++it)
    std::cout << *it << (std::next(it) == words.end() ? '\n' : ' ');

  const auto grid = pluss::note::make(
      std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
      std::array<std::int64_t, 2>{2, 3});
  std::cout << "\n=== row-major matrix ===\n";
  show_quoted(grid);
  std::cout << '\n';
  for (const auto cell : grid) std::cout << cell << ' ';
  std::cout << '\n';

  const auto scalar = pluss::note::make("solo");
  const auto empty = pluss::note::make(std::vector<std::string>{});
  std::cout << "\n=== scalar and empty ===\n"
            << "scalar distance=" << std::distance(scalar.begin(), scalar.end())
            << " value=" << *scalar.begin() << '\n'
            << "empty distance=" << std::distance(empty.begin(), empty.end())
            << " equal=" << std::boolalpha << (empty.begin() == empty.end()) << '\n';

  auto kept = [] {
    const auto temporary = pluss::note::make({"kept", "alive"});
    return temporary.begin();
  }();
  std::cout << "\n=== retained iterator ===\n" << *kept;
  ++kept;
  std::cout << ' ' << *kept << '\n';

  std::vector<std::string_view> borrowed;
  words.copy_flat_to(std::back_inserter(borrowed));
  std::vector<std::string> owning;
  std::transform(words.begin(), words.end(), std::back_inserter(owning),
                 [](std::string_view value) { return std::string(value); });
  std::cout << "\n=== copy destinations ===\n"
            << "owning=";
  show_quoted(owning);
  std::cout << "\nborrowed=";
  show_quoted(borrowed);
  std::cout << '\n';
  std::vector<std::string_view> compass{"north", "east"};
  const auto southern = pluss::note::make({"south", "west"});
  southern.copy_flat_to(std::back_inserter(compass));
  for (const auto value : compass) std::cout << value << ' ';
  std::cout << '\n';

  const std::array<std::byte, 5> raw{
      std::byte{'A'}, std::byte{0}, std::byte{'B'}, std::byte{0x7f}, std::byte{0xff}};
  const auto bytes = pluss::note::bytes(raw);
  std::vector<std::string_view> byte_cells;
  bytes.copy_flat_to(std::back_inserter(byte_cells));
  std::cout << "\n=== embedded bytes ===\nsize=" << byte_cells[0].size() << " hex=";
  for (const unsigned char value : byte_cells[0])
    std::cout << std::hex << std::setfill('0') << std::setw(2)
              << static_cast<unsigned int>(value);
  std::cout << std::dec << '\n';

  std::size_t cells = 0;
  std::size_t bytes_total = 0;
  grid.for_each_flat([&](std::string_view cell) {
    ++cells;
    bytes_total += cell.size();
  });
  std::cout << "\n=== callback aggregation ===\n"
            << "cells=" << cells << " bytes=" << bytes_total << '\n';
  bool first = true;
  words.for_each_flat([&](std::string_view word) {
    std::cout << (first ? "" : " | ") << word;
    first = false;
  });
  std::cout << '\n';

  try {
    words.for_each_flat([](std::string_view cell) {
      if (cell == "south") throw std::runtime_error("stop at south");
    });
  } catch (const std::runtime_error& error) {
    std::cout << "callback error=" << error.what() << '\n';
  }
  std::size_t empty_calls = 0;
  empty.for_each_flat([&](std::string_view) { ++empty_calls; });
  std::cout << "empty calls=" << empty_calls << '\n';

  const auto undefined = pluss::Note::undefined();
  std::cout << "\n=== undefined range ===\n"
            << "equal=" << (undefined.begin() == undefined.end()) << '\n';
  show_rejection("undefined copy", [&] {
    undefined.copy_flat_to(std::back_inserter(borrowed));
  });
  show_rejection("undefined callback", [&] {
    undefined.for_each_flat([](std::string_view) {});
  });

  const auto integers = pluss::note::make(
      std::vector<std::int64_t>{3, 1, 4});
  std::cout << "\n=== typed traversal ===\n";
  show_rejection("Int64 begin", [&] { (void)integers.begin(); });
  show_rejection("Int64 copy", [&] {
    integers.copy_flat_to(std::back_inserter(borrowed));
  });
  std::cout << "integers=";
  for (const auto value : integers.integers()) std::cout << value << ' ';
  std::cout << '\n';
}
