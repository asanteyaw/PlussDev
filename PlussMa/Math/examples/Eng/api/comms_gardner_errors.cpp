#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series wave = chain(cm::modulate(cm::psk(4), random_bits(2 * 100, 1)), taps, 0.0);
  report([&] { (void)cm::gardner(wave, 1, 0.02); });
  report([&] { (void)cm::gardner(wave, 4, 0.0); });
  report([&] { (void)cm::gardner(wave, 4, 0.7); });
  report([&] { (void)cm::gardner(wave.slice(-1, 0, 20), 4, 0.02); });
  report([&] { (void)cm::matched(pluss::Series{}, taps); });
  report([&] { (void)cm::matched(wave, vec({})); });
}
