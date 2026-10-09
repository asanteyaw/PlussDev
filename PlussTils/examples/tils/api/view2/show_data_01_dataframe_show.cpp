#include <pluss/pluss.h>

#include <iostream>

int main() {
  using namespace pluss::table;

  auto orders = DataFrame({
      {"id", Slate(pluss::make_series({1, 2, 3}))},
      {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
      {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
  });

  // The explicit-stream overload always prints plain text, regardless of
  // which window backend is available on this machine.
  display::DisplayOptions options;
  orders.show(std::cout, options);
  return 0;
}
