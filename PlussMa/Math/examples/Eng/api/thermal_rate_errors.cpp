#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)th::rate(vec({0.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter); });   // a zero capacity rate
  report([&] { (void)th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({-5.0}), th::Flow::Counter); }); // negative UA
  report([&] { (void)th::rate(vec({1000.0}), vec({2000.0}), vec({std::nan("")}), vec({20.0}), vec({1500.0}), th::Flow::Counter); });
}
