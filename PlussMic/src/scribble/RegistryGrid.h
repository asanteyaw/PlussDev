#pragma once

// A registry laid out for the viewer as a ragged grid: its fields side by side, each as long as its own slot. It is the
// shared ragged grid model with a registry's fields as its columns.

#include "scribble/registry.h"
#include "table/display/RaggedGrid.h"

namespace pluss::scribble::display {

class RegistryGridModel final : public pluss::table::display::RaggedGridModel {
 public:
  explicit RegistryGridModel(const Registry& registry);

  std::int64_t fields() const noexcept { return columns(); }
};

}  // namespace pluss::scribble::display
