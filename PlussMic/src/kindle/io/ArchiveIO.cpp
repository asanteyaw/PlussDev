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

// An archive in a notebook: one row per level (its label, how many keys, how long they run, and their names). A level is
// drilled into with catalog(i), which then shows as a grid.
std::string render_notebook_html(const pluss::kindle::Archive& archive) {
  if (!archive.defined()) {
    return "<div class=\"pluss-table\">Archive &lt;undefined&gt;</div>";
  }
  std::optional<pluss::table::Shelf> labels;
  if (archive.label().has()) {
    labels = archive.label().values().to_shelf().cast().note();
  }
  SummaryView view;
  view.columns = {"level", "label", "keys", "min_cs", "max_cs", "key names"};
  view.size = archive.depth();
  view.row = [&](const std::int64_t i) {
    const auto catalog = archive.catalog(i);
    std::int64_t shortest = 0;
    std::int64_t longest = 0;
    if (catalog.nkeys() > 0) {
      shortest = std::numeric_limits<std::int64_t>::max();
      for (const auto& key : catalog.keys()) {
        const auto size = catalog.keys(key).size();
        shortest = std::min(shortest, size);
        longest = std::max(longest, size);
      }
    }
    const auto& names = catalog.keys();
    return std::vector<std::string>{
        std::to_string(i), labels.has_value() ? std::string(labels->cast().note().as_note().at(i)) : std::string{},
        std::to_string(catalog.nkeys()), std::to_string(shortest), std::to_string(longest),
        preview_text(static_cast<std::int64_t>(names.size()), [&](const std::int64_t k) { return names[static_cast<std::size_t>(k)]; }, 60)};
  };
  return render_notebook_summary(view, pluss::tils::OutputEngine::format("depth {}", archive.depth()));
}

}  // namespace pluss::table::display
