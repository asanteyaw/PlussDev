// Series normal_cdf_(const Series& x, double mean, double stddev) {
//   PLUSS_CHECK(stddev > 0.0, "subsp::stats::distr::normal cdf: std must be > 0");
//   const Series xx = as_f64_(x, "cdf");
//   return xx.sub(mean).div(stddev * kSqrt2).erf().add(1.0).mul(0.5);
// }