#include "table/Thread.h"

#include <iostream>
#include <ostream>

#include "table/display/Display.h"

namespace pluss::table {

std::string Thread::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

void Thread::show(const display::DisplayOptions& options) const {
  display::show(*this, options);
}

void Thread::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const Thread& thread) {
  thread.show(os);
  return os;
}

} // namespace pluss::table
