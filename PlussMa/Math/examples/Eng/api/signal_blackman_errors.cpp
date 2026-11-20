#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  refused(out, "a negative size", [] { sig::blackman(-1); });
  refused(out, "an integer dtype", [] { sig::blackman(4, {.dtype = pluss::ScalarType::Int64}); });
  refused(out, "float64 on the MPS", [] { sig::blackman(4, {.platform = pluss::Platform::MPS}); });
  out.writeln("size zero is empty:", sig::blackman(0).numel() == 0);
  out.writeln("size one is a single coefficient:", sig::blackman(1).items<double>());
}
