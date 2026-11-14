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
  // a real signal receives real noise of the full variance
  const pluss::Series signal = pluss::series::zeros({40000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const pluss::Series noisy = cm::noise(signal, 0.5, 6);
  out.writeln("dtype stays real", !noisy.dtype().is_complex());
  out.writeln("variance", noisy.mul(noisy).mean().item<double>(), " requested 0.5");
  out.writeln("mean", noisy.mean().item<double>());
}
