#include "kindle/display/GridModel.h"

#include <utility>

#include "kindle/Stream.h"

namespace pluss::kindle::display {

namespace {

std::vector<pluss::table::display::RaggedColumn> catalog_columns(const Catalog& catalog) {
  std::vector<pluss::table::display::RaggedColumn> columns;
  for (const auto& name : catalog.keys()) {
    columns.push_back(pluss::table::display::ragged_column_from_shelf(name, catalog.keys(name).to_shelf()));
  }
  return columns;
}

}  // namespace

CatalogGridModel::CatalogGridModel(const Catalog& catalog) : RaggedGridModel("entry", catalog_columns(catalog)) {}

}  // namespace pluss::kindle::display
