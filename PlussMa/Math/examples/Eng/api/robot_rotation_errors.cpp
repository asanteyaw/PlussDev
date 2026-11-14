#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series angle = vec({0.5}).select(0, 0);
  report([&] { (void)rb::rotation(vec({0, 0, 0}), angle); });
  report([&] { (void)rb::rotation(vec({1, 0}), angle); });
  report([&] { (void)rb::rotation(pluss::series::make(std::vector<std::int64_t>{0, 0, 1}), angle); });
}
