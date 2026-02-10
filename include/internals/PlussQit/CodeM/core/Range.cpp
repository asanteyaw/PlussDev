#include <PlussQit/core/Range.h>

#include <ostream>

namespace pluss {

std::ostream& operator<<(std::ostream& out, const Range& range) {
  out << "Range[" << range.begin << ", " << range.end << "]";
  return out;
}

}  // namespace pluss
