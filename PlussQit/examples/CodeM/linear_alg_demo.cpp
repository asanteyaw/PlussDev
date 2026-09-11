#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::ScalarType;

  auto a = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  auto b = pluss::make_series(std::vector<double>{7, 8, 9, 10, 11, 12}, {3, 2});
  auto i3 = pluss::series::eye(3, 3, ScalarType::Float64);

  std::cout << "A:\n" << a << "\n\n";
  std::cout << "B:\n" << b << "\n\n";
  std::cout << "A @ B:\n" << a.matmul(b) << "\n\n";
  std::cout << "A.linearalg(B).matmul():\n" << a.linearalg(b).matmul() << "\n\n";

  auto gram = a.transpose(0, 1).matmul(a);
  std::cout << "Gram = A^T A:\n" << gram << "\n\n";
  auto gram_i = gram.add(i3);
  std::cout << "subsp::linalg::det(Gram + I):\n" << pluss::subsp::math::linalg::det(gram_i) << "\n\n";
  std::cout << "(Gram + I).linearalg.det():\n" << gram_i.linearalg.det() << "\n\n";
  std::cout << "subsp::linalg::inverse(Gram + I):\n" << pluss::subsp::math::linalg::inverse(gram_i) << "\n\n";
  std::cout << "subsp::linalg::matrix_power(Gram + I, 2):\n"
            << pluss::subsp::math::linalg::matrix_power(gram_i, 2) << "\n\n";
  std::cout << "subsp::linalg::matrix_exp(Gram + I):\n"
            << pluss::subsp::math::linalg::matrix_exp(gram_i) << "\n\n";
  std::cout << "subsp::linalg::cholesky(Gram + I):\n" << pluss::subsp::math::linalg::cholesky(gram_i) << "\n\n";

  auto qr_res = pluss::subsp::math::linalg::qr(a);
  std::cout << "subsp::linalg::qr(A).Q:\n" << qr_res.Q << "\n\n";
  std::cout << "subsp::linalg::qr(A).R:\n" << qr_res.R << "\n\n";

  auto svd_res = pluss::subsp::math::linalg::svd(a);
  std::cout << "subsp::linalg::svd(A).U:\n" << svd_res.U << "\n\n";
  std::cout << "subsp::linalg::svd(A).S:\n" << svd_res.S << "\n\n";
  std::cout << "subsp::linalg::svd(A).Vh:\n" << svd_res.Vh << "\n\n";

  auto x = pluss::make_series(std::vector<double>{1, 2, 3}, {3});
  auto y = pluss::make_series(std::vector<double>{3, 2, 1}, {3});
  std::cout << "vdot(x, y):\n" << x.vdot(y) << "\n\n";

  auto p = pluss::make_series(std::vector<double>{0, 0, 1, 0, 2, 0}, {3, 2});
  auto q = pluss::make_series(std::vector<double>{0, 1, 2, 1, 3, 1}, {3, 2});
  std::cout << "cdist(p, q):\n" << p.cdist(q) << "\n";

  pluss::Series s1{pluss::make_series({3.4, 2.4})}, s2{pluss::make_series({3.4, 2.5})};
  pluss::Series s3 = s1 - s2;
  pluss::Series s4 = s1 + s2;

  std::cout << "s3:\n" << s3 << "\n\n";
  std::cout << "s4:\n" << s4 << "\n\n";
  std::cout << "s1 == s2:\n" << (s1 == s2) << "\n\n";
  std::cout << "s1 >= s2:\n" << (s1 >= s2) << "\n\n";
  std::cout << "s1 != s2:\n" << (s1 != s2) << "\n\n";
  std::cout << "ternary chain (s1 == s2 == s3):\n" << (s1.tri() == s2 == s3) << "\n\n";
  std::cout << "ternary mixed (s1 >= s2 < s3):\n" << (s1.tri() >= s2 < s3) << "\n\n";

  return 0;
}
