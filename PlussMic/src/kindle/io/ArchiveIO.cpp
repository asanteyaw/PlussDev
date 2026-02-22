#include "kindle/Archive.h"

#include <ostream>

#include "kindle/display/Display.h"
#include "table/Atlas.h"

namespace pluss::kindle {

pluss::table::Atlas Archive::to_table(const TabelizePolicy& policy, bool render) const {
  auto out = tabelize(policy);
  if (render) {
    out.show();
  }
  return out;
}

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
