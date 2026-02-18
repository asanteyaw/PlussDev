#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::ScalarType;

  let a = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  let b = pluss::make_series(std::vector<double>{7, 8, 9, 10, 11, 12}, {3, 2});
  let i3 = pluss::eye(3, 3, ScalarType::Float64);

  std::cout << "A:\n" << a << "\n\n";
  std::cout << "B:\n" << b << "\n\n";
  std::cout << "A @ B:\n" << a.matmul(b) << "\n\n";

  let gram = a.transpose(0, 1).matmul(a);
  std::cout << "Gram = A^T A:\n" << gram << "\n\n";
  std::cout << "det(Gram + 1e-3 I):\n" << gram.add(i3.mul(1e-3)).det() << "\n\n";
  std::cout << "inverse(Gram + I):\n" << gram.add(i3).inverse() << "\n\n";
  std::cout << "matrix_power(Gram + I, 2):\n" << gram.add(i3).matrix_power(2) << "\n\n";
  std::cout << "cholesky(Gram + I):\n" << gram.add(i3).cholesky() << "\n\n";

  let x = pluss::make_series(std::vector<double>{1, 2, 3}, {3});
  let y = pluss::make_series(std::vector<double>{3, 2, 1}, {3});
  std::cout << "vdot(x, y):\n" << x.vdot(y) << "\n\n";

  let p = pluss::make_series(std::vector<double>{0, 0, 1, 0, 2, 0}, {3, 2});
  let q = pluss::make_series(std::vector<double>{0, 1, 2, 1, 3, 1}, {3, 2});
  std::cout << "cdist(p, q):\n" << p.cdist(q) << "\n";

  return 0;
}
