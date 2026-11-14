#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::psk(4);
  const pluss::Series labels = pluss::series::make(std::vector<std::int64_t>{0, 1, 2, 3, 3, 0});
  const pluss::Series symbols = cm::symbols(c, labels);
  out.writeln("real", flat(symbols.real()));
  out.writeln("imag", flat(symbols.imag()));
  // a matrix of labels keeps its shape
  const pluss::Series grid = pluss::series::make(std::vector<std::int64_t>{0, 1, 2, 3}).reshape({2, 2});
  out.writeln("symbol shape", cm::symbols(c, grid).sizes());
}
