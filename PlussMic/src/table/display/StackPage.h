#pragma once

// The window page of a stack of tables (an atlas's frames, an archive's catalogs, an axel's registries): one table
// painted at a time by the one table renderer, with a Level menu to move between the levels. The table of a level is
// whatever model its type paints (a frame's rectangular model, a catalog's or a registry's ragged grid), so a level has
// every feature that table has anywhere else: sorting, resizing, the filter, Visit, number format, themes.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "table/display/Display.h"

namespace pluss::table::display {

struct StackSource {
  std::string title;  // "Atlas", "Archive", "Axel"
  std::string unit;   // what a level is called: "frame", "catalog", "registry"
  std::int64_t depth{0};
  std::function<std::string(std::int64_t)> label;  // the label of a level; unset: its number
  std::function<std::shared_ptr<OptionsModel>(std::int64_t)> table;  // the table of a level, built when it is first shown
  std::function<std::vector<std::pair<std::string, std::string>>(std::int64_t)> cards;  // what a level says about itself
  bool ragged{false};  // the levels are ragged grids, so View > Align is offered
};

ModelPage stack_model_page(StackSource source);

}  // namespace pluss::table::display
