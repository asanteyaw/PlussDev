#include <pluss/tils/auxiliary/transmit.h>

#include <iostream>
#include <vector>

using namespace pluss::tils;

int main() {
  std::vector<int> out;
  auto end_it = transmit_into([](int x) { return x * x; }, std::vector<int>{1, 2, 3}, std::back_inserter(out));
  (void)end_it;  // a back_insert_iterator has no meaningful position to print; the call succeeded if out is filled
  std::cout << "squares:";
  for (int v : out) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
