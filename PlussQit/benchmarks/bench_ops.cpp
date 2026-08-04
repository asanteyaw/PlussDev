#include <pluss/pluss.h>
#include <chrono>
#include <iostream>

static double ms_since(std::chrono::steady_clock::time_point t0) {
  using namespace std::chrono;
  return duration_cast<duration<double, std::milli>>(steady_clock::now() - t0).count();
}

int main() {
  const std::int64_t N = 10'000'000;
  auto a = pluss::series::full({N}, 1.25, pluss::ScalarType::Float32);
  auto b = pluss::series::full({N}, 2.00, pluss::ScalarType::Float32);

  // warmup
  auto c = a.add(b);
  c = c.mul(a);
  c = c.exp();

  {
    auto t0 = std::chrono::steady_clock::now();
    for (int it=0; it<50; ++it) c = a.add(b);
    std::cout << "add Float32: " << ms_since(t0) << " ms\n";
  }
  {
    auto t0 = std::chrono::steady_clock::now();
    for (int it=0; it<50; ++it) c = a.mul(b);
    std::cout << "mul Float32: " << ms_since(t0) << " ms\n";
  }
  {
    auto t0 = std::chrono::steady_clock::now();
    for (int it=0; it<20; ++it) c = a.exp();
    std::cout << "exp Float32: " << ms_since(t0) << " ms\n";
  }
  std::cout << "done\n";
}
