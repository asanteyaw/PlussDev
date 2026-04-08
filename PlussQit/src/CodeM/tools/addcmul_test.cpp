#include <iostream>
#include <cstring>
#include <span>
#include "CodeM/core/Series.h"
#include "CodeM/core/OpsInternal.h"

using namespace pluss::codem;
using namespace pluss::codem::ops_internal;

int main() {
  // small broadcast test
  auto make_f64 = [](const std::vector<double>& data,
                     std::span<const std::int64_t> shape) {
    Series out = Series::empty(shape, ScalarType::Float64);
    if (!data.empty()) {
      std::memcpy(out.raw_mutable_data(), data.data(), sizeof(double) * data.size());
    }
    return out;
  };

  std::vector<double> a_flat = {1.0, 2.0, 3.0, 4.0};
  std::vector<std::int64_t> shape = {4};
  Series a = make_f64(a_flat, shape);
  Series b = make_f64({10.0,20.0,30.0,40.0}, shape);
  Series c = make_f64({2.0,2.0,2.0,2.0}, shape);

  Series out = addcmul(a,b,c,0.5);
  Series expected = add(a, mul(mul(b,c), 0.5));

  // simple elementwise compare
  auto equal = [](const Series& x, const Series& y){
    if (x.numel() != y.numel()) return false;
    const double* xp = reinterpret_cast<const double*>(x.raw_data());
    const double* yp = reinterpret_cast<const double*>(y.raw_data());
    for (std::int64_t i=0;i<x.numel();++i) if (std::abs(xp[i]-yp[i])>1e-12) return false;
    return true;
  };

  if (!equal(out, expected)) {
    std::cerr << "addcmul test failed\n";
    return 2;
  }

  // inplace
  Series a2 = make_f64(a_flat, shape);
  addcmul_(a2, b, c, 0.5);
  if (!equal(a2, expected)) {
    std::cerr << "addcmul_ test failed\n";
    return 3;
  }

  // addcdiv basic
  Series out2 = addcdiv(a,b,c,0.5);
  Series expected2 = add(a, mul(b, 0.5) / c);
  // compute expected2 using ops
  Series tmp = div(mul(b, 0.5), c);
  Series expected2b = add(a, tmp);
  if (!equal(out2, expected2b)) {
    std::cerr << "addcdiv test failed\n";
    return 4;
  }

  // inplace addcdiv
  Series a3 = make_f64(a_flat, shape);
  addcdiv_(a3, b, c, 0.5);
  if (!equal(a3, expected2b)) {
    std::cerr << "addcdiv_ test failed\n";
    return 5;
  }

  std::cout << "ok\n";
  return 0;
}
