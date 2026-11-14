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
  // the pulse paired with itself must be zero at every other symbol instant (no intersymbol interference)
  for (double rolloff : {0.0, 0.25, 0.5, 1.0}) {
    const pluss::Series taps = cm::rrc(rolloff, 8, 16);
    const pluss::Series both = pluss::subsp::eng::signal::conv(taps, taps);
    const std::vector<double> r = flat(both);
    const std::size_t mid = r.size() / 2;
    double worst = 0.0;
    for (int k = 1; k <= 8; ++k) worst = std::max(worst, std::abs(r[mid + 8 * static_cast<std::size_t>(k)]));
    out.writeln("rolloff", rolloff, " peak", r[mid], " largest interference at symbol instants", worst);
  }
}
