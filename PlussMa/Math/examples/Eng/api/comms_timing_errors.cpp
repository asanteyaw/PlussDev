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
static pluss::Series wave_of(const cm::Constellation &c, const pluss::Series &taps, std::uint64_t seed, std::int64_t symbols) {
  return cm::shape(cm::modulate(c, random_bits(c.bits * symbols, seed)), taps, 8);
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
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(cm::psk(4), taps, 1, 100);
  report([&] { (void)cm::timing(wave, 2); });
  report([&] { (void)cm::timing(wave.slice(-1, 0, 20), 8); });
  report([&] { (void)cm::delay(wave, vec({std::nan("")}).select(0, 0)); });
  report([&] { (void)cm::delay(pluss::Series{}, vec({1.0}).select(0, 0)); });
}
