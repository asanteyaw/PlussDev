#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  out.writeln("boxcar =", sig::boxcar(5).items<double>());
  out.writeln("bartlett =", sig::bartlett(5).items<double>());
  out.writeln("blackman =", sig::blackman(5).items<double>());
  out.writeln("hamming =", sig::hamming(5).items<double>());
  out.writeln("hann =", sig::hann(5).items<double>());
}
