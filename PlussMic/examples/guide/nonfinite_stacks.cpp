// The same family on every type. One column of six cells holding a value, a NaN, +inf, -inf, a missing cell and a value is built in each type's
// own way, and the same three drops keep the same number of cells everywhere: the types differ in shape, not in what the kinds mean.
#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>

using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::scribble::Axel;
using pluss::scribble::Registry;
using pluss::scribble::Slot;
using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

namespace {
void row(const char* type, std::int64_t dropnan, std::int64_t dropinf, std::int64_t dropinvalid) {
  std::cout << std::left << std::setw(10) << type << std::right << std::setw(8) << dropnan << std::setw(8) << dropinf << std::setw(12)
            << dropinvalid << "\n";
}
}  // namespace

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
  const auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});

  // One dimension: the shelf holds the rule, a stream wraps a shelf, and a slot holds cells (an empty cell is NULL).
  Shelf shelf(values, present);
  Stream stream(values, present);
  Slot slot({pluss::make_series({1.0}), pluss::make_series({nan}), pluss::make_series({inf}), pluss::make_series({-inf}), std::monostate{},
             pluss::make_series({5.0})});

  // Two dimensions delegate to the one-dimensional rule, and the stacks delegate once more.
  DataFrame frame({{"x", shelf}});
  Catalog catalog(std::vector<std::pair<std::string, Stream>>{{"x", stream}});
  Registry registry({{"x", slot}});
  Atlas atlas(std::vector<DataFrame>{frame, frame});
  Archive archive(std::vector<Catalog>{catalog, catalog});
  Axel axel(std::vector<Registry>{registry, registry});

  std::cout << std::left << std::setw(10) << "cells kept" << std::right << std::setw(8) << "dropnan" << std::setw(8) << "dropinf" << std::setw(12)
            << "dropinvalid" << "\n";
  row("Shelf", shelf.dropnan().size(), shelf.dropinf().size(), shelf.dropinvalid().size());
  row("Stream", stream.dropnan().size(), stream.dropinf().size(), stream.dropinvalid().size());
  row("Slot", slot.dropnan().size(), slot.dropinf().size(), slot.dropinvalid().size());
  row("DataFrame", frame.dropnan().ncases(), frame.dropinf().ncases(), frame.dropinvalid().ncases());
  row("Catalog", catalog.dropnan().keys("x").size(), catalog.dropinf().keys("x").size(), catalog.dropinvalid().keys("x").size());
  row("Registry", registry.dropnan().fields("x").size(), registry.dropinf().fields("x").size(), registry.dropinvalid().fields("x").size());
  row("Atlas", atlas.dropnan().frame(1).ncases(), atlas.dropinf().frame(1).ncases(), atlas.dropinvalid().frame(1).ncases());
  row("Archive", archive.dropnan().catalog(1).keys("x").size(), archive.dropinf().catalog(1).keys("x").size(),
      archive.dropinvalid().catalog(1).keys("x").size());
  row("Axel", axel.dropnan().registry(1).fields("x").size(), axel.dropinf().registry(1).fields("x").size(),
      axel.dropinvalid().registry(1).fields("x").size());
}
