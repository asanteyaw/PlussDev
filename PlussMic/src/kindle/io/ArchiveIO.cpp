#include "kindle/Archive.h"

#include <iostream>
#include <ostream>

#include "kindle/display/Display.h"
#include "table/Atlas.h"
#include "table/display/Display.h"
#include <pluss/tils/kit/output.h>
#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <vector>

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

namespace pluss::table::display {

// An archive in a notebook: the same short list of facts the terminal shows.
std::string render_notebook_html(const pluss::kindle::Archive& archive) {
  if (!archive.defined()) {
    return "<div class=\"pluss-table\">Archive &lt;undefined&gt;</div>";
  }
  return render_notebook_stack_facts(pluss::kindle::display::archive_facts(archive));
}

}  // namespace pluss::table::display
