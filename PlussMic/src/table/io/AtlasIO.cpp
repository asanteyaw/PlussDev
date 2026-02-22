#include "table/Atlas.h"

#include <iostream>
#include <ostream>

#include "table/display/Display.h"

namespace pluss::table {

std::string Atlas::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

void Atlas::show(const display::DisplayOptions& options) const {
  display::print(std::cout, *this, options);
}

void Atlas::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const Atlas& atlas) {
  atlas.show(os);
  return os;
}

} // namespace pluss::table
