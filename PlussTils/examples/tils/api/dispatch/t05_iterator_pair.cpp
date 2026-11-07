// Transforming a part of a container. Two iterators mark the start and the end, so only the middle
// of a list is changed. The rest of the list is not visited at all.
#include <pluss/tils/auxiliary/transmit.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const std::vector<std::string> names{"ama", "kofi", "efua", "yaw", "akosua"};

  const auto upper = pluss::tils::transmit(
      [](const std::string& s) {
        std::string out = s;
        for (char& c : out) c = static_cast<char>(c - ('a' - 'A'));
        return out;
      },
      names.begin() + 1, names.begin() + 4);

  for (const auto& name : upper) std::cout << name << '\n';
  std::cout << "first name left alone: " << names.front() << '\n';
  return 0;
}
