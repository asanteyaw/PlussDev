#include "kindle/catalog.h"

#include <iostream>
#include <ostream>

#include "kindle/display/Display.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"

namespace pluss::kindle {

pluss::table::DataFrame Catalog::to_table(const TabelizePolicy& policy, bool render) const {
  auto out = tabelize(policy);
  if (render) {
    pluss::table::display::print(std::cout, out, {});
  }
  return out;
}

void Catalog::display() const {
  show(std::cout);
}

void Catalog::info() const {
  display::print(std::cout, *this);
}

void Catalog::save_csv(std::string_view filename) const {
  tabelize().save_csv(filename);
}

std::string Catalog::to_string() const {
  return display::render(*this);
}

std::string Catalog::to_json() const {
  return display::render(*this);
}

void Catalog::show(std::ostream& os) const { display::print(os, *this); }

std::ostream& operator<<(std::ostream& os, const Catalog& catalog) {
  catalog.show(os);
  return os;
}

} // namespace pluss::kindle
