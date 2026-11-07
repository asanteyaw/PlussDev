// Numbering the entries of a checklist. When the callable takes a second argument, transmit passes the
// position of each element, so the label can depend on where the entry sits in the list.
#include <pluss/tils/kit/transmit.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const std::vector<std::string> steps{"pull", "build", "test", "deploy"};

  const auto labels = pluss::tils::transmit(
      [](const std::string& step, std::size_t position) {
        return std::to_string(position + 1) + ". " + step;
      },
      steps);

  for (const auto& label : labels) std::cout << label << '\n';
  return 0;
}
