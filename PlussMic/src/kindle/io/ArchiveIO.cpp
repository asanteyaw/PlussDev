#include "kindle/Archive.h"

#include <iostream>
#include <ostream>

#include "kindle/display/Display.h"
#include "table/Atlas.h"

namespace pluss::kindle {

std::string Archive::to_string() const {
  return display::render(*this);
}

void Archive::show(std::ostream& os) const {
  display::print(os, *this);
}

std::ostream& operator<<(std::ostream& os, const Archive& archive) {
  archive.show(os);
  return os;
}

} // namespace pluss::kindle
