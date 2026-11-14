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
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 400, 71));
  const pluss::Series wave = cm::shape(symbols, taps, 8);
  const pluss::Series late = cm::delay(wave, pluss::series::make(std::vector<double>{2.37}).select(0, 0));
  const auto evm_of = [&](const pluss::Series &w) { return cm::evm(cm::match(w, taps, 8, 380).slice(-1, 10, 370), symbols.slice(-1, 10, 370)).item<double>(); };
  const double estimate = cm::timing(late, 8).item<double>();
  out.writeln("estimated delay", estimate);
  out.writeln("evm sampling the delayed waveform as it is", evm_of(late));
  out.writeln("evm after removing the estimated delay   ", evm_of(cm::delay(late, pluss::series::make(std::vector<double>{-estimate}).select(0, 0))));
  // noisy: the estimate stays close at 10 dB Es/N0
  const pluss::Series noisy = cm::noise(late, 0.1, 5);
  out.writeln("estimated delay at 10 dB", cm::timing(noisy, 8).item<double>());
}
