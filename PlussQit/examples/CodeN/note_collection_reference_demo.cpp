#include <pluss/pluss.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

void show(std::string_view label, const pluss::Note& note) {
  std::cout << label << '\n' << pluss::note::print(note) << '\n';
}

pluss::Note matrix(std::initializer_list<const char*> values,
                   std::int64_t rows, std::int64_t columns) {
  std::vector<std::string> owned;
  owned.reserve(values.size());
  for (const auto* value : values) owned.emplace_back(value);
  return pluss::note::make(
      owned, std::array<std::int64_t, 2>{rows, columns});
}

} // namespace

int main() {
  const auto words = pluss::note::make({"red", "green", "blue"});
  std::cout << "=== join returns Note ===\n";
  show("custom separator", words.join(" / "));
  show("empty separator", words.join());
  std::cout << "matrix="
            << matrix({"a", "b", "c", "d"}, 2, 2).join("/").scalar() << '\n';
  show("empty cells", pluss::note::make({"", "x", ""}).join("|"));
  const auto no_cells = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  show("zero cells", no_cells.join(","));
  std::cout << "undefined join defined=" << std::boolalpha
            << pluss::Note::undefined().join(",").defined() << '\n';

  const auto raw = pluss::note::bytes(
      std::vector<std::string>{std::string("A\0", 2), "B"},
      std::array<std::int64_t, 1>{2});
  rejected("join bytes", [&] { (void)raw.join("|"); });

  std::cout << "\n=== join_flat returns string ===\n"
            << "default=" << words.join_flat() << '\n'
            << "custom=" << matrix({"a", "b", "c", "d"}, 2, 2).join_flat("/") << '\n'
            << "zero cells bytes=" << no_cells.join_flat(",").size() << '\n';
  std::string retained;
  {
    retained = pluss::note::make({"left", "right"}).join_flat("+");
  }
  std::cout << "lifetime=" << retained << '\n';
  const auto flat_raw = raw.join_flat("|");
  std::cout << "raw size=" << flat_raw.size() << " values="
            << static_cast<int>(static_cast<unsigned char>(flat_raw[0])) << ','
            << static_cast<int>(static_cast<unsigned char>(flat_raw[1])) << ','
            << static_cast<int>(static_cast<unsigned char>(flat_raw[2])) << ','
            << static_cast<int>(static_cast<unsigned char>(flat_raw[3])) << '\n';
  rejected("join_flat undefined", [&] {
    (void)pluss::Note::undefined().join_flat(",");
  });

  const auto sortable = matrix({"b", "2", "a", "3", "b", "1"}, 3, 2);
  std::cout << "\n=== sort lanes ===\n";
  show("default last axis", sortable.sort());
  show("axis zero", sortable.sort(0));
  std::cout << "explicit -1 matches default="
            << std::boolalpha
            << (sortable.sort(-1).vector() == sortable.sort().vector()) << '\n';
  show("scalar", pluss::note::make("x").sort());
  show("empty matrix", no_cells.sort());
  std::cout << "undefined sort defined="
            << pluss::Note::undefined().sort().defined() << '\n';
  rejected("sort axis 2", [&] { (void)sortable.sort(2); });

  const auto raw_values = pluss::note::bytes(
      std::vector<std::string>{"b", "a", "b"},
      std::array<std::int64_t, 1>{3});
  show("raw bytes", raw_values.sort());

  const auto repeated_rows = matrix({"b", "2", "a", "1", "b", "2"}, 3, 2);
  const auto repeated_columns = matrix({"b", "a", "b", "2", "1", "2"}, 2, 3);
  std::cout << "\n=== unique sorted distinct ===\n";
  show("flattened", repeated_rows.unique());
  show("whole rows", repeated_rows.unique(0));
  show("whole columns", repeated_columns.unique(-1));
  const auto empty_slices = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 3, 0});
  show("three equal empty slices", empty_slices.unique(1));
  const auto empty_axis = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{0, 2});
  show("empty selected axis", empty_axis.unique(0));
  show("scalar flattened", pluss::note::make("x").unique());
  show("raw bytes", raw_values.unique());
  rejected("unique scalar axis", [&] { (void)pluss::note::make("x").unique(0); });
  std::cout << "undefined unique defined="
            << pluss::Note::undefined().unique().defined() << '\n';

  std::cout << "\n=== deduplicate first seen ===\n";
  show("flattened", repeated_rows.deduplicate());
  show("whole rows", repeated_rows.deduplicate(0));
  show("whole columns", repeated_columns.deduplicate(-1));
  show("three equal empty slices", empty_slices.deduplicate(1));
  show("raw bytes", raw_values.deduplicate());
  std::cout << "undefined deduplicate defined="
            << pluss::Note::undefined().deduplicate().defined() << '\n';
  rejected("deduplicate axis 2", [&] { (void)repeated_rows.deduplicate(2); });
}
