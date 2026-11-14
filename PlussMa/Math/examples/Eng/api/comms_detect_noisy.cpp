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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series sent = pluss::series::make(std::vector<std::int64_t>{0, 5, 10, 15, 3, 12});
  const pluss::Series noisy = cm::noise(cm::symbols(c, sent), 0.005, 3);
  out.writeln("sent    ", labels_of(sent));
  out.writeln("detected", labels_of(cm::detect(c, noisy)));
  // a received symbol exactly between two points goes to the closer one; far outside the constellation to the nearest corner
  const pluss::Series far = pluss::series::complex(vec({9.0}), vec({9.0}), pluss::ScalarType::Complex128);
  out.writeln("far outside, up and right", labels_of(cm::detect(c, far)));
}
