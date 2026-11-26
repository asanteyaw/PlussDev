#include "kindle/Archive.h"
#include "CodeM/core/Errors.h"

#include <iostream>
#include <ostream>

#include "kindle/display/Display.h"
#include "table/Atlas.h"
#include "kindle/display/GridModel.h"
#include "table/display/Display.h"
#include "table/display/StackPage.h"
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

// The archive in the PlussView window: one catalog at a time as the ragged grid a catalog opens as (see StackPage.h).
void Archive::show() const {
  if (pluss::table::display::show_in_notebook(pluss::table::display::render_notebook_html(*this))) {
    return;
  }
  PLUSS_CHECK(defined(), "Archive::show: archive undefined");
  std::optional<pluss::table::Shelf> labels;
  if (label().has()) {
    labels = label().values().to_shelf().cast().note();
  }
  const Archive held = *this;
  pluss::table::display::StackSource source;
  source.title = "Archive";
  source.unit = "catalog";
  source.depth = depth();
  source.ragged = true;
  source.label = [labels](const std::int64_t i) {
    return labels.has_value() ? std::string(labels->cast().note().as_note().at(i)) : std::string{};
  };
  source.table = [held](const std::int64_t i) -> std::shared_ptr<pluss::table::display::OptionsModel> {
    return std::make_shared<display::CatalogGridModel>(held.catalog(i));
  };
  source.cards = [held](const std::int64_t i) {
    const display::CatalogGridModel model(held.catalog(i));
    return std::vector<std::pair<std::string, std::string>>{{"Keys", std::to_string(model.keys())},
                                                              {"Longest", std::to_string(model.longest())},
                                                              {"Shortest", std::to_string(model.shortest())}};
  };
  pluss::table::display::DisplayOptions options;
  if (pluss::table::display::default_theme() == pluss::table::display::DisplayTheme::Default) {
    options.theme = pluss::table::display::DisplayTheme::Ember;  // a ragged type opens in Ember
  }
  if (!pluss::table::display::show_model(pluss::table::display::stack_model_page(std::move(source)), options)) {
    show(std::cout);
  }
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
