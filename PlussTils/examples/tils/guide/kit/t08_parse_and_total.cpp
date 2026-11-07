// A small pipeline for a batch of order lines. Each line is parsed into a quantity and a unit price,
// the lines are priced in one pass, and the totals are summed. Each stage is one transmit call.
#include <pluss/tils/kit/transmit.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct Line {
  int quantity;
  double unit_price;
};

int main() {
  const std::vector<std::string> raw{"3 x 4.50", "1 x 19.99", "12 x 0.75"};

  const auto parsed = pluss::tils::transmit(
      [](const std::string& text) {
        std::istringstream in(text);
        Line line{};
        std::string separator;
        in >> line.quantity >> separator >> line.unit_price;
        return line;
      },
      raw);

  const auto totals = pluss::tils::transmit(
      [](const Line& line) { return line.quantity * line.unit_price; }, parsed);

  double grand_total = 0.0;
  for (double t : totals) grand_total += t;

  std::cout << std::fixed << std::setprecision(2);
  for (std::size_t i = 0; i < totals.size(); ++i) {
    std::cout << raw[i] << " -> " << totals[i] << '\n';
  }
  std::cout << "grand total: " << grand_total << '\n';
  return 0;
}
