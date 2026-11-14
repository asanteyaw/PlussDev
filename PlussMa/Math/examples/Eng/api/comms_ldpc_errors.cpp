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
static pluss::Series soft_of(const pluss::Series &codeword, double amplitude) {
  return pluss::series::make(std::vector<double>{1.0}).add(codeword.to(pluss::ScalarType::Float64).mul(-2.0)).mul(amplitude);
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
  report([&] { (void)cm::gallager(1000, 3, 6, 1); });
  report([&] { (void)cm::gallager(1200, 1, 6, 1); });
  report([&] { (void)cm::gallager(1200, 6, 6, 1); });
  const cm::LdpcCode code = cm::gallager(600, 3, 6, 1);
  report([&] { (void)cm::encode(code, random_bits(5, 1)); });
  report([&] { (void)cm::encode(code, vec({0, 2, 1})); });
  report([&] { (void)cm::decode(code, vec({1, 2, 3})); });
  report([&] { (void)cm::decode(code, pluss::series::zeros({600}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), 0); });
  report([&] { (void)cm::decode(cm::LdpcCode{}, vec({1, 2, 3})); });
}
