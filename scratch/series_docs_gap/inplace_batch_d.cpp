#include <pluss/pluss.h>

#include <iostream>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}
}  // namespace

int main() {
  heading("addcdiv_");
  { auto x = pluss::make_series({1.0, 1.0, 1.0}, {3});
    auto t1 = pluss::make_series({2.0, 3.0, 4.0}, {3});
    auto t2 = pluss::make_series({2.0, 3.0, 4.0}, {3});
    x.addcdiv_(t1, t2, 1.0); std::cout << x; }

  heading("addcmul_");
  { auto x = pluss::make_series({1.0, 1.0, 1.0}, {3});
    auto t1 = pluss::make_series({2.0, 3.0, 4.0}, {3});
    auto t2 = pluss::make_series({1.0, 1.0, 1.0}, {3});
    x.addcmul_(t1, t2, 1.0); std::cout << x; }

  heading("addmm_");
  { auto x = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
    auto mat1 = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
    auto mat2 = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
    x.addmm_(mat1, mat2, 1.0, 1.0); std::cout << x; }

  heading("addmv_");
  { auto x = pluss::make_series({0.0, 0.0}, {2});
    auto mat = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
    auto vec = pluss::make_series({1.0, 1.0}, {2});
    x.addmv_(mat, vec, 1.0, 1.0); std::cout << x; }

  heading("addr_");
  { auto x = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
    auto vec1 = pluss::make_series({1.0, 2.0}, {2});
    auto vec2 = pluss::make_series({3.0, 4.0}, {2});
    x.addr_(vec1, vec2, 1.0, 1.0); std::cout << x; }

  heading("baddbmm_");
  { auto x = pluss::make_series({{{0.0, 0.0}, {0.0, 0.0}}});
    auto batch1 = pluss::make_series({{{1.0, 0.0}, {0.0, 1.0}}});
    auto batch2 = pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}}});
    x.baddbmm_(batch1, batch2, 1.0, 1.0); std::cout << x; }

  heading("lerp_(Series end, double weight)");
  { auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    auto end = pluss::make_series({10.0, 10.0, 10.0}, {3});
    x.lerp_(end, 0.5); std::cout << x; }

  heading("lerp_(Series end, Series weight)");
  { auto x = pluss::make_series({0.0, 0.0}, {2});
    auto end = pluss::make_series({10.0, 20.0}, {2});
    auto weight = pluss::make_series({0.5, 0.25}, {2});
    x.lerp_(end, weight); std::cout << x; }

  return 0;
}
