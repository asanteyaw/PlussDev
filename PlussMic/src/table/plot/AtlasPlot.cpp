#include "table/Atlas.h"
#include "table/plot/AtlasPlot.h"

namespace pluss::table {

AtlasPlotAccessor Atlas::plot() const {
  return AtlasPlotAccessor(frames(), axis0_labels());
}

AtlasPlotAccessor Atlas::plot(const AtlasPlotOptions& options) const {
  return AtlasPlotAccessor(frames(), axis0_labels(), options);
}

} // namespace pluss::table
