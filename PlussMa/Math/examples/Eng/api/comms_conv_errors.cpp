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

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series bits = random_bits(8, 1);
  report([&] { (void)cm::encode(cm::ConvolutionalCode{1, {1, 1}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07, 011}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07, 0}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{}, vec({0, 1, 2})); });
}
