#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// Renders a studio that holds one board built by the caller, and returns the render.
template <class Build>
bi::StudioRender render_with(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  build(studio);
  return studio.render();
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  std::cout << std::boolalpha;
  // Tile::from_view wraps a view with its key and layout. Board does this when you add a view.
  const bi::View view = bi::table("rows").query(bi::from("sales")).place(0, 1, 1, 2);
  const bi::Tile tile = bi::Tile::from_view(view.key(), view.layout(), view);
  std::cout << "key: " << tile.key() << '\n';
  std::cout << "holds a view: " << tile.view().has_value() << '\n';
  std::cout << "layout: row " << tile.layout().row << ", col " << tile.layout().col
            << ", span " << tile.layout().row_span << "x" << tile.layout().col_span << '\n';
  return 0;
}
