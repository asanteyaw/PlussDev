// The same value shown three ways: as a plain value, as a debug representation, and as a table
// cell. Presets are named starting points, and builder calls adjust them in a chain.
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::vector<int> ids{7, 42, 99};
  const std::optional<std::string> name{"Ama"};

  out.writeln("plain:", ids, name, PrintOptions::plain());
  out.writeln("repr:", ids, name, PrintOptions::repr());
  out.writelnf("cell: [{}]", "checkout", PrintOptions::table_cell(10));
  out.writeln("built with a chain:", 1234567.5, PrintOptions{}.fixed(1).grouped(',').with_sep(" "));
  out.writeln("pretty json:", ids, PrintOptions::pretty_json());
  return 0;
}
