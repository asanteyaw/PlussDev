// Arithmetic can be written with operators or with methods. They are the same operations: use whichever reads better, and mix them freely.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto y = pluss::make_series({10.0, 20.0, 30.0, 40.0});

  // 1. Each operator has a method with the same result. allclose says the two Series agree.
  std::cout << "x + y   == x.add(y):   " << (x + y).allclose(x.add(y)) << "\n";
  std::cout << "x - y   == x.sub(y):   " << (x - y).allclose(x.sub(y)) << "\n";
  std::cout << "x * y   == x.mul(y):   " << (x * y).allclose(x.mul(y)) << "\n";
  std::cout << "y / x   == y.div(x):   " << (y / x).allclose(y.div(x)) << "\n";
  std::cout << "-x      == x.neg():    " << (-x).allclose(x.neg()) << "\n";

  // 2. A number may be on either side, and an expression reads the way it is written on paper.
  std::cout << "2 * x + 1   : " << pluss::series::print(2.0 * x + 1.0) << "\n";
  std::cout << "1 / x       : " << pluss::series::print(1.0 / x) << "\n";
  std::cout << "(y - x) / x : " << pluss::series::print((y - x) / x) << "\n";

  // 3. Comparisons give Boolean Series, with operators or methods.
  std::cout << "x > 2       : " << pluss::series::print(x > 2.0) << "\n";
  std::cout << "x.gt(2)     : " << pluss::series::print(x.gt(2.0)) << "\n";

  // 4. In place: += changes the Series it is applied to. The method with a trailing underscore does the same.
  auto a = x.clone();
  a += y;
  auto b = x.clone();
  b.add_(y);
  std::cout << "a += y      : " << pluss::series::print(a) << "\n";
  std::cout << "b.add_(y)   : " << pluss::series::print(b) << "\n";

  // 5. Methods take what operators cannot say: a power, a clamp, a fused multiply-add.
  std::cout << "x.pow(2)           : " << pluss::series::print(x.pow(2.0)) << "\n";
  std::cout << "y.clamp_max(25)    : " << pluss::series::print(y.clamp_max(25.0)) << "\n";
  std::cout << "x.addcmul(x, y, 0.1): " << pluss::series::print(x.addcmul(x, y, 0.1)) << "\n";
}
