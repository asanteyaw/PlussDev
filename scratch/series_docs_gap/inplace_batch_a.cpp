#include <pluss/pluss.h>

#include <iostream>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}

template <class F>
void probe(const char* name, std::initializer_list<double> vals, F&& call) {
  auto x = pluss::make_series(std::vector<double>(vals.begin(), vals.end()), {(std::int64_t)vals.size()});
  auto& ref = call(x);
  heading(name);
  std::cout << "x=" << x << "\n";
  std::cout << std::boolalpha << "same object: " << (&ref == &x) << "\n";
}
}  // namespace

int main() {
  probe("acos_", {-0.5, 0.0, 0.5}, [](pluss::Series& s) -> pluss::Series& { return s.acos_(); });
  probe("acosh_", {1.0, 2.0, 5.0}, [](pluss::Series& s) -> pluss::Series& { return s.acosh_(); });
  probe("asin_", {-0.5, 0.0, 0.5}, [](pluss::Series& s) -> pluss::Series& { return s.asin_(); });
  probe("asinh_", {-1.0, 0.0, 2.0}, [](pluss::Series& s) -> pluss::Series& { return s.asinh_(); });
  probe("atan_", {-1.0, 0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.atan_(); });
  probe("atanh_", {-0.5, 0.0, 0.5}, [](pluss::Series& s) -> pluss::Series& { return s.atanh_(); });
  probe("ceil_", {1.2, -1.2, 2.0}, [](pluss::Series& s) -> pluss::Series& { return s.ceil_(); });
  probe("cos_", {0.0, 1.0, 2.0}, [](pluss::Series& s) -> pluss::Series& { return s.cos_(); });
  probe("cosh_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.cosh_(); });
  probe("digamma_", {1.0, 2.0, 5.0}, [](pluss::Series& s) -> pluss::Series& { return s.digamma_(); });
  probe("erf_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.erf_(); });
  probe("erfc_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.erfc_(); });
  probe("erfcinv_", {0.5, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.erfcinv_(); });
  probe("erfinv_", {0.0, 0.5}, [](pluss::Series& s) -> pluss::Series& { return s.erfinv_(); });
  probe("exp2_", {0.0, 1.0, 3.0}, [](pluss::Series& s) -> pluss::Series& { return s.exp2_(); });
  probe("exp_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.exp_(); });
  probe("expm1_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.expm1_(); });
  probe("floor_", {1.7, -1.2}, [](pluss::Series& s) -> pluss::Series& { return s.floor_(); });
  probe("frac_", {1.75, -1.25}, [](pluss::Series& s) -> pluss::Series& { return s.frac_(); });
  probe("i0_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.i0_(); });
  probe("lgamma_", {1.0, 2.0, 5.0}, [](pluss::Series& s) -> pluss::Series& { return s.lgamma_(); });
  probe("log10_", {1.0, 10.0, 100.0}, [](pluss::Series& s) -> pluss::Series& { return s.log10_(); });
  probe("log1p_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.log1p_(); });
  probe("log2_", {1.0, 2.0, 8.0}, [](pluss::Series& s) -> pluss::Series& { return s.log2_(); });
  probe("log_", {1.0, 2.718281828459045}, [](pluss::Series& s) -> pluss::Series& { return s.log_(); });
  probe("neg_", {1.0, -2.0, 3.0}, [](pluss::Series& s) -> pluss::Series& { return s.neg_(); });
  probe("reciprocal_", {1.0, 2.0, 4.0}, [](pluss::Series& s) -> pluss::Series& { return s.reciprocal_(); });
  probe("relu_", {-1.0, 0.0, 2.0}, [](pluss::Series& s) -> pluss::Series& { return s.relu_(); });
  probe("round_", {1.4, 1.5, 2.5}, [](pluss::Series& s) -> pluss::Series& { return s.round_(); });
  probe("rsqrt_", {1.0, 4.0, 16.0}, [](pluss::Series& s) -> pluss::Series& { return s.rsqrt_(); });
  probe("sigmoid_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.sigmoid_(); });
  probe("sign_", {-3.0, 0.0, 4.0}, [](pluss::Series& s) -> pluss::Series& { return s.sign_(); });
  probe("signbit_", {-1.0, 0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.signbit_(); });
  probe("sgn_", {-3.0, 0.0, 4.0}, [](pluss::Series& s) -> pluss::Series& { return s.sgn_(); });
  probe("silu_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.silu_(); });
  probe("sin_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.sin_(); });
  probe("sinc_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.sinc_(); });
  probe("sinh_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.sinh_(); });
  probe("softplus_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.softplus_(); });
  probe("sqrt_", {1.0, 4.0, 9.0}, [](pluss::Series& s) -> pluss::Series& { return s.sqrt_(); });
  probe("square_", {2.0, -3.0}, [](pluss::Series& s) -> pluss::Series& { return s.square_(); });
  probe("tan_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.tan_(); });
  probe("tanh_", {0.0, 1.0}, [](pluss::Series& s) -> pluss::Series& { return s.tanh_(); });
  probe("trunc_", {1.7, -1.7}, [](pluss::Series& s) -> pluss::Series& { return s.trunc_(); });
  return 0;
}
