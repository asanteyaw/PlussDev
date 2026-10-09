#include <pluss/mic.h>
#include <pluss/queue.h>
#include <pluss/tils/let.h>

#include <iostream>

namespace tils = pluss::tils;

int main() {
  // Queue is a payload too -- a dynamic holder over Series/Note/Timeline/
  // Duration, for code that doesn't know which of the four it has until
  // runtime. Binding it works the same as any other payload.
  tils::let boxed = pluss::Queue(pluss::make_series({1.0, 2.0, 3.0}));
  std::cout << "kind: " << boxed.get().kind_name() << '\n';
  std::cout << "numel: " << boxed.get().numel() << '\n';
  return 0;
}
