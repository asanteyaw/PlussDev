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
  // the closest pairs of points differ in exactly one bit when the labels are Gray coded, and in more when natural
  for (bool gray : {true, false}) {
    cm::ConstellationOptions options;
    options.gray = gray;
    const cm::Constellation c = cm::qam(16, options);
    const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
    double nearest = 1e9;
    for (std::size_t a = 0; a < 16; ++a)
      for (std::size_t b = a + 1; b < 16; ++b) nearest = std::min(nearest, std::hypot(re[a] - re[b], im[a] - im[b]));
    int worst = 0;
    for (std::size_t a = 0; a < 16; ++a)
      for (std::size_t b = a + 1; b < 16; ++b)
        if (std::abs(std::hypot(re[a] - re[b], im[a] - im[b]) - nearest) < 1e-9) {
          int n = 0;
          for (std::size_t x = a ^ b; x; x >>= 1) n += static_cast<int>(x & 1);
          worst = std::max(worst, n);
        }
    out.writeln(gray ? "Gray   " : "natural", " most bits that differ between nearest neighbours", worst);
  }
}
