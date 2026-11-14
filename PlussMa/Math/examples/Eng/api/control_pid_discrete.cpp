#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const control::Pid controller = control::pid(
      pluss::make_series(1.5), pluss::make_series(0.4),
      pluss::make_series(0.2), pluss::make_series(0.05), 0.1,
      control::PidRule::Trapezoid, control::PidRule::Backward);
  const pluss::Series response =
      control::freqresp(controller, pluss::make_series({0.5, 2.0}));

  std::cout << std::boolalpha
            << "continuous = " << controller.is_cont()
            << ", discrete = " << controller.is_disc() << "\n";
  std::cout << "sample time = " << controller.sample_time()
            << ", integral rule = " << static_cast<int>(controller.int_rule())
            << ", derivative rule = " << static_cast<int>(controller.diff_rule())
            << "\n";
  out.writeln("discrete response =",
              response.items<std::complex<double>>());
}
