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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series symbols = cm::modulate(cm::qam(16), random_bits(4 * 16, 1));
  report([&] { (void)cm::to_tones(symbols, 7, 1); });
  report([&] { (void)cm::to_tones(symbols, 16, 16); });
  report([&] { (void)cm::to_tones(random_bits(16, 1), 16, 4); });
  report([&] { (void)cm::from_tones(cm::to_tones(symbols, 16, 4).slice(-1, 0, 19), 16, 4); });
}
