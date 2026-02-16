#pragma once
#include <iosfwd>
#include <string>
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem {

struct PrintOptions {
  std::int64_t edge_items = 3;     // items to show at each edge per dimension
  std::int64_t max_elements = 200; // beyond this, summarize
  bool show_dtype = true;
  bool show_shape = true;
};

std::string to_string(const Series& s, const PrintOptions& opt = {});
std::ostream& operator<<(std::ostream& os, const Series& s);

} // namespace pluss::codem
