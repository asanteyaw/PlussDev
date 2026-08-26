#include "pluss/codem/codem.h"
#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

struct Foo {
  int grok(std::string name, int age, int height) {
    std::cout << "name=" << name << ", age=" << age << ", height=" << height << "\n";
    return age + height;
  }
};

int main() {
  Foo foo;

  auto r = pluss::dispense(foo, &Foo::grok, std::make_tuple("John", 29, 190));
  std::cout << "return: " << r << "\n";

  auto r2 = pluss::dispense(foo, &Foo::grok, std::make_tuple(std::string("Jane"), 31, 165));
  std::cout << "return (tuple): " << r2 << "\n";

  auto s = pluss::make_series({-4.0, 5.0, -6.0});
  auto s_numel = pluss::dispense(s, &pluss::Series::numel, std::make_tuple());
  std::cout << "Series numel: " << s_numel << "\n";

  auto n = pluss::make_note({"apple", "banana", "apricot"});
  auto starts_with_a = pluss::dispense(n, &pluss::Note::starts_with, std::make_tuple("a"));
  std::cout << "Note starts_with(\"a\"): ["
            << static_cast<int>(starts_with_a[0]) << ", "
            << static_cast<int>(starts_with_a[1]) << ", "
            << static_cast<int>(starts_with_a[2]) << "]\n";

  auto t = pluss::timeline::make("2024-01-15T10:11:12Z");
  auto t_floor = pluss::dispense(t, &pluss::codeh::Timeline::floor, std::make_tuple("D"));
  std::cout << "Timeline floor(\"D\") hour: "
            << t_floor.hour().to_vec<std::int32_t>(0) << "\n";

  auto t_year = pluss::dispense(t, &pluss::codeh::Timeline::year, std::make_tuple());
  std::cout << "Timeline year: " << t_year.to_vec<std::int32_t>(0) << "\n";

  auto s1 = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto s2 = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3});
  auto s3 = pluss::make_series(std::vector<double>{100.0, 200.0, 300.0}, {3});
  auto piled = pluss::dispense(
      s1,
      &pluss::Series::pile,
      std::make_tuple(std::vector<pluss::Series>{s2, s3}, std::int64_t{1}));
  const auto piled_vals = piled.to_vec<double>();
  std::cout << "Series pile({s2,s3}, dim=1) dim=" << piled.dim()
            << ", shape=[" << piled.sizes()[0] << ", " << piled.sizes()[1] << "]"
            << ", first row=[" << piled_vals[0] << ", " << piled_vals[1] << ", " << piled_vals[2] << "]\n";
  std::cout << pluss::series::print(piled) << "\n";

  return 0;
}
