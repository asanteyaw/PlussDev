#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  let x = pluss::value(std::vector<double>{-3, -2, -1, 0, 1, 2, 3}, {7});
  std::cout << "x:\n" << x << "\n\n";

  let xdct = x.dct();
  std::cout << "dct(x):\n" << xdct << "\n\n";
  std::cout << "idct(dct(x)):\n" << xdct.idct() << "\n\n";

  let xdst = x.dst();
  std::cout << "dst(x):\n" << xdst << "\n\n";
  std::cout << "idst(dst(x)):\n" << xdst.idst() << "\n\n";

  let h = x.hilbert();
  std::cout << "hilbert(x):\n" << h << "\n\n";
  std::cout << "real(h):\n" << h.real() << "\n\n";
  std::cout << "imag(h):\n" << h.imag() << "\n\n";
  std::cout << "angle(h):\n" << h.angle() << "\n\n";
  std::cout << "abs2(h):\n" << h.abs2() << "\n\n";

  let m = pluss::value(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9}, {3, 3});
  std::cout << "fftshift(m):\n" << m.fftshift() << "\n\n";
  std::cout << "ifftshift(fftshift(m)):\n" << m.fftshift().ifftshift() << "\n\n";
  std::cout << "unwrap(x):\n" << x.unwrap() << "\n";

  return 0;
}
