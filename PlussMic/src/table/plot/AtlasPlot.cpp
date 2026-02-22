#include "table/Atlas.h"
#include "table/plot/DataFramePlot.h"

namespace pluss::table {

DataFramePlotAccessor Atlas::plot(std::int64_t frame_index) const {
  return frame(frame_index).plot();
}

} // namespace pluss::table
