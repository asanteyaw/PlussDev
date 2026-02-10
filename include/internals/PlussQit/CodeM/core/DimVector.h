#pragma once
#include <MSharp/util/DimVector.h>

namespace pluss {

// Re-declaring 'DimVector' type and size inside 'at' namespace.
// This is done to avoid modifying every use into their 'c10'
// equivalent.

using MSharp::kDimVectorStaticSize;
using MSharp::DimVector;

} // namespace pluss
