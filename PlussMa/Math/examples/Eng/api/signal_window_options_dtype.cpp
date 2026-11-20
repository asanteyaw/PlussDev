#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));

  const pluss::Series wide = sig::hamming(8);
  const pluss::Series narrow = sig::hamming(8, {.dtype = pluss::ScalarType::Float32});
  out.writeln("float64 dtype =", wide.scalar_type(), " float32 dtype =", narrow.scalar_type());
  const auto a = wide.items<double>();
  const auto b = narrow.items<float>();
  double worst = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) worst = std::max(worst, std::abs(a[i] - static_cast<double>(b[i])));
  out.writeln("largest float32 rounding difference =", worst);
}
