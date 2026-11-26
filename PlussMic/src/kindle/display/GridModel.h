#pragma once

// A catalog laid out for the viewer as a ragged grid: its keys side by side, each column as long as its own stream.
// Nothing is padded to make the columns equal. Past the end of a stream a cell is absent (nothing is drawn); a
// real NULL inside a stream is still a NULL. It is a model for the one table renderer, so scrolling, resizing,
// themes, the filter and Visit work as they do for any table. All of that lives in the shared ragged grid model; a
// catalog only supplies its streams as the columns.

#include "kindle/catalog.h"
#include "table/display/RaggedGrid.h"

namespace pluss::kindle {

class Catalog;

// The page Catalog::show() opens: the ragged grid, its cards and the view tabs.
pluss::table::display::ModelPage catalog_page(const Catalog& catalog);

}  // namespace pluss::kindle

namespace pluss::kindle::display {

class CatalogGridModel final : public pluss::table::display::RaggedGridModel {
 public:
  explicit CatalogGridModel(const Catalog& catalog);

  std::int64_t keys() const noexcept { return columns(); }
};

}  // namespace pluss::kindle::display
