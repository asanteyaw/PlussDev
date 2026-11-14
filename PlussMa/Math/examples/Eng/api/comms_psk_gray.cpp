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
  const auto neighbours = [&](const cm::Constellation &c, const char *name) {
    // walk once round the circle and count the bit changes between neighbouring points
    const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
    std::vector<std::int64_t> by_angle(re.size());
    for (std::size_t label = 0; label < re.size(); ++label) {
      double angle = std::atan2(im[label], re[label]);
      if (angle < -1e-9) angle += 2.0 * M_PI;
      by_angle[static_cast<std::size_t>(std::llround(angle / (2.0 * M_PI / static_cast<double>(re.size())))) % re.size()] = static_cast<std::int64_t>(label);
    }
    std::vector<int> changes;
    for (std::size_t k = 0; k < by_angle.size(); ++k) {
      std::int64_t x = by_angle[k] ^ by_angle[(k + 1) % by_angle.size()];
      int n = 0;
      for (; x; x >>= 1) n += static_cast<int>(x & 1);
      changes.push_back(n);
    }
    out.writeln(name, "labels round the circle", by_angle, " bit changes between neighbours", changes);
  };
  neighbours(cm::psk(8), "Gray   ");
  cm::ConstellationOptions natural;
  natural.gray = false;
  neighbours(cm::psk(8, natural), "natural");
}
