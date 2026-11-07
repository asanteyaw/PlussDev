// Running a slow transformation across all cores. The parallel result must match the sequential one
// exactly, so the program checks that before trusting the speedup. The sum uses integers, so the
// comparison is exact.
#include <pluss/tils/auxiliary/transmit.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  std::vector<std::int64_t> inputs(200000);
  for (std::size_t i = 0; i < inputs.size(); ++i) inputs[i] = static_cast<std::int64_t>(i);

  const auto square_mod = [](std::int64_t x) { return (x * x) % 1000003; };

  const auto sequential = pluss::tils::transmit(square_mod, inputs);
  const auto parallel = pluss::tils::transmit(
      square_mod, inputs, pluss::tils::transmit_options{.exec = pluss::tils::transmit_exec::par,
                                                        .grain_size = 4096, .workers = 4});

  std::cout << "results: " << parallel.size() << '\n';
  std::cout << "parallel equals sequential: " << (parallel == sequential) << '\n';
  return 0;
}
