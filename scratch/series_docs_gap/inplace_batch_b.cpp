#include <pluss/pluss.h>

#include <iostream>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}
}  // namespace

int main() {
  heading("div_(Series)");
  { auto x = pluss::make_series({10.0, 20.0, 30.0}, {3}); auto y = pluss::make_series({2.0, 4.0, 5.0}, {3});
    auto& ref = x.div_(y); std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }
  heading("div_(double)");
  { auto x = pluss::make_series({10.0, 20.0, 30.0}, {3});
    x.div_(2.0); std::cout << "x=" << x; }

  heading("mul_(Series)");
  { auto x = pluss::make_series({2.0, 3.0, 4.0}, {3}); auto y = pluss::make_series({5.0, 5.0, 5.0}, {3});
    auto& ref = x.mul_(y); std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }
  heading("mul_(double)");
  { auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    x.mul_(10.0); std::cout << "x=" << x; }

  heading("sub_(Series)");
  { auto x = pluss::make_series({10.0, 20.0, 30.0}, {3}); auto y = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto& ref = x.sub_(y); std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("eq_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.eq_(y); std::cout << "x=" << x; }
  heading("ge_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({1.0, 3.0, 3.0}, {3});
    x.ge_(y); std::cout << "x=" << x; }
  heading("gt_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({0.0, 2.0, 5.0}, {3});
    x.gt_(y); std::cout << "x=" << x; }
  heading("le_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({1.0, 1.0, 5.0}, {3});
    x.le_(y); std::cout << "x=" << x; }
  heading("lt_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({2.0, 2.0, 1.0}, {3});
    x.lt_(y); std::cout << "x=" << x; }
  heading("ne_");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3}); auto y = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.ne_(y); std::cout << "x=" << x; }

  heading("fmod_(Series)");
  { auto x = pluss::make_series({10.0, 11.0, -7.0}, {3}); auto y = pluss::make_series({3.0, 3.0, 3.0}, {3});
    x.fmod_(y); std::cout << "x=" << x; }
  heading("fmod_(double)");
  { auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    x.fmod_(3.0); std::cout << "x=" << x; }

  heading("remainder_(Series)");
  { auto x = pluss::make_series({10.0, 11.0, -7.0}, {3}); auto y = pluss::make_series({3.0, 3.0, 3.0}, {3});
    x.remainder_(y); std::cout << "x=" << x; }
  heading("remainder_(double)");
  { auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    x.remainder_(3.0); std::cout << "x=" << x; }

  heading("floor_divide_(Series)");
  { auto x = pluss::make_series({7.0, -7.0, 8.0}, {3}); auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});
    x.floor_divide_(y); std::cout << "x=" << x; }
  heading("floor_divide_(double)");
  { auto x = pluss::make_series({7.0, -7.0, 8.0}, {3});
    x.floor_divide_(2.0); std::cout << "x=" << x; }

  heading("pow_(Series)");
  { auto x = pluss::make_series({2.0, 3.0, 4.0}, {3}); auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});
    x.pow_(y); std::cout << "x=" << x; }
  heading("pow_(double)");
  { auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    x.pow_(2.0); std::cout << "x=" << x; }

  heading("gcd_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{12, 18}, {2}); auto y = pluss::make_series(std::vector<std::int64_t>{8, 24}, {2});
    x.gcd_(y); std::cout << "x=" << x; }
  heading("lcm_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{4, 6}, {2}); auto y = pluss::make_series(std::vector<std::int64_t>{6, 9}, {2});
    x.lcm_(y); std::cout << "x=" << x; }
  heading("copysign_");
  { auto x = pluss::make_series({3.0, -3.0}, {2}); auto y = pluss::make_series({-1.0, 1.0}, {2});
    x.copysign_(y); std::cout << "x=" << x; }
  heading("hypot_");
  { auto x = pluss::make_series({3.0, 3.0}, {2}); auto y = pluss::make_series({4.0, 4.0}, {2});
    x.hypot_(y); std::cout << "x=" << x; }
  heading("nextafter_");
  { auto x = pluss::make_series({1.0}, {1}); auto y = pluss::make_series({2.0}, {1});
    x.nextafter_(y); std::cout << "x=" << x; }
  heading("xlogy_(Series)");
  { auto x = pluss::make_series({0.0, 1.0, 2.0}, {3}); auto y = pluss::make_series({1.0, 1.0, 1.0}, {3});
    x.xlogy_(y); std::cout << "x=" << x; }
  heading("xlogy_(double)");
  { auto x = pluss::make_series({0.0, 1.0, 2.0}, {3});
    x.xlogy_(1.0); std::cout << "x=" << x; }
  heading("atan2_");
  { auto x = pluss::make_series({1.0, 1.0}, {2}); auto y = pluss::make_series({1.0, -1.0}, {2});
    x.atan2_(y); std::cout << "x=" << x; }
  heading("heaviside_(Series)");
  { auto x = pluss::make_series({-1.0, 0.0, 1.0}, {3}); auto y = pluss::make_series({0.5, 0.5, 0.5}, {3});
    x.heaviside_(y); std::cout << "x=" << x; }
  heading("heaviside_(double)");
  { auto x = pluss::make_series({-1.0, 0.0, 1.0}, {3});
    x.heaviside_(0.5); std::cout << "x=" << x; }
  heading("maximum_(double)");
  { auto x = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.maximum_(4.0); std::cout << "x=" << x; }
  heading("minimum_(double)");
  { auto x = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.minimum_(4.0); std::cout << "x=" << x; }
  heading("logit_(double eps)");
  { auto x = pluss::make_series({0.0, 0.5, 1.0}, {3});
    x.logit_(1e-6); std::cout << "x=" << x; }
  heading("ldexp_(Series)");
  { auto x = pluss::make_series({1.0, 1.0}, {2}); auto y = pluss::make_series({2.0, 3.0}, {2});
    x.ldexp_(y); std::cout << "x=" << x; }

  return 0;
}
