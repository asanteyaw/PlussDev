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
  out.print_defaults(PrintOptions{}.fixed(5));
  // a phase offset rotates every point; Complex64 gives single-precision points
  cm::ConstellationOptions options;
  options.phase = M_PI / 4;
  const cm::Constellation rotated = cm::psk(4, options);
  out.writeln("QPSK at 45 degrees, point of label 0", flat(rotated.points.real())[0], flat(rotated.points.imag())[0]);
  cm::ConstellationOptions single;
  single.dtype = pluss::ScalarType::Complex64;
  out.writeln("single precision points", cm::psk(4, single).points.scalar_type() == pluss::ScalarType::Complex64);
  // pi/2-BPSK style start: BPSK is two points on the real axis
  out.writeln("BPSK real parts", flat(cm::psk(2).points.real()));
}
