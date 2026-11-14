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
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  const pluss::Series waveform = cm::shape(cm::modulate(cm::psk(4), random_bits(8, 1)), taps, 4);
  report([&] { (void)cm::match(waveform, taps, 4, 50); });
  report([&] { (void)cm::match(waveform, taps, 0, 4); });
  report([&] { (void)cm::match(waveform, taps, 4, 0); });
  report([&] { (void)cm::match(waveform, vec({}), 4, 4); });
}
