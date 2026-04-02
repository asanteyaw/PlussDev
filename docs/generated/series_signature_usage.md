# Complete Series Method Reference: Signatures + Real Calls

```cpp
auto x = pluss::make_series(std::vector<double>{-3,-2,-1,0,1,2,3}, {7});
auto y = pluss::make_series(std::vector<double>{3,2,1,0,-1,-2,-3}, {7});
auto pos = pluss::make_series(std::vector<double>{0.2,0.5,1.0,2.0,3.0,4.0,5.0}, {7});
auto unit = pluss::make_series(std::vector<double>{-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9}, {7});
auto m = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto n = pluss::make_series(std::vector<double>{6,5,4,3,2,1}, {2,3});
auto mm = pluss::make_series(std::vector<double>{1,2,3,4,5,6,7,8,9}, {3,3});
auto cond = x.gt(0.0);
auto mask = pluss::Series::ones({2,3}, pluss::ScalarType::Bool);
auto gather_src = pluss::make_series(std::vector<std::int64_t>{10,20,30,40,50,60}, {2,3});
auto gather_idx = pluss::make_series(std::vector<std::int64_t>{2,1,0,0,1,2}, {2,3});
auto scatter_base = pluss::Series::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6}, {2,3});
auto flat_idx = pluss::make_series(std::vector<std::int64_t>{0,3,5}, {3});
auto flat_src = pluss::make_series(std::vector<std::int64_t>{99,88,77}, {3});
```

- Signature: `Series view(std::span<const std::int64_t>)`
  Example: `auto out = m.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));`
- Signature: `Series reshape(std::span<const std::int64_t>)`
  Example: `auto out = m.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));`
- Signature: `Series transpose(std::int64_t, std::int64_t)`
  Example: `auto out = m.transpose(0, 1);`
- Signature: `Series permute(std::span<const std::int64_t>)`
  Example: `auto out = mm.permute(std::span<const std::int64_t>(perm_10.data(), perm_10.size()));`
- Signature: `Series unsqueeze(std::int64_t)`
  Example: `auto out = x.unsqueeze(0);`
- Signature: `Series squeeze()`
  Example: `auto out = x.unsqueeze(0).squeeze();`
- Signature: `Series squeeze(std::int64_t)`
  Example: `auto out = x.unsqueeze(0).squeeze(0);`
- Signature: `Series flatten(std::int64_t, std::int64_t)`
  Example: `auto out = mm.flatten(0, 1);`
- Signature: `Series select(std::int64_t, std::int64_t)`
  Example: `auto out = mm.select(0, 1);`
- Signature: `Series take(const Series&)`
  Example: `auto out = gather_src.take(flat_idx);`
- Signature: `Series nonzero()`
  Example: `auto out = cond.nonzero();`
- Signature: `Series add(double)`
  Example: `auto out = x.add(1.5);`
- Signature: `Series sub(double)`
  Example: `auto out = x.sub(1.5);`
- Signature: `Series mul(double)`
  Example: `auto out = x.mul(2.0);`
- Signature: `Series div(double)`
  Example: `auto out = x.div(2.0);`
- Signature: `Series pow(double)`
  Example: `auto out = pos.pow(2.0);`
- Signature: `Series maximum(double)`
  Example: `auto out = x.maximum(0.0);`
- Signature: `Series minimum(double)`
  Example: `auto out = x.minimum(0.0);`
- Signature: `Series eq(double)`
  Example: `auto out = x.eq(0.0);`
- Signature: `Series ne(double)`
  Example: `auto out = x.ne(0.0);`
- Signature: `Series lt(double)`
  Example: `auto out = x.lt(0.0);`
- Signature: `Series le(double)`
  Example: `auto out = x.le(0.0);`
- Signature: `Series gt(double)`
  Example: `auto out = x.gt(0.0);`
- Signature: `Series ge(double)`
  Example: `auto out = x.ge(0.0);`
- Signature: `Series add(const Series&)`
  Example: `auto out = x.add(y);`
- Signature: `Series sub(const Series&)`
  Example: `auto out = x.sub(y);`
- Signature: `Series mul(const Series&)`
  Example: `auto out = x.mul(y);`
- Signature: `Series div(const Series&)`
  Example: `auto out = pos.div(pos.add(1.0));`
- Signature: `Series pow(const Series&)`
  Example: `auto out = pos.pow(pos.mul(0.0).add(2.0));`
- Signature: `Series bitwise_and(const Series&)`
  Example: `auto out = bi.bitwise_and(bi.add(1));`
- Signature: `Series bitwise_or(const Series&)`
  Example: `auto out = bi.bitwise_or(bi.add(1));`
- Signature: `Series bitwise_xor(const Series&)`
  Example: `auto out = bi.bitwise_xor(bi.add(1));`
- Signature: `Series bitwise_not()`
  Example: `auto out = bi.bitwise_not();`
- Signature: `Series logical_and(const Series&)`
  Example: `auto out = x.gt(0.0).logical_and(x.lt(3.0));`
- Signature: `Series logical_or(const Series&)`
  Example: `auto out = x.gt(0.0).logical_or(x.lt(-2.0));`
- Signature: `Series logical_xor(const Series&)`
  Example: `auto out = x.gt(0.0).logical_xor(x.lt(0.0));`
- Signature: `Series logical_not()`
  Example: `auto out = x.gt(0.0).logical_not();`
- Signature: `Series exp()`
  Example: `auto out = pos.exp();`
- Signature: `Series log()`
  Example: `auto out = pos.log();`
- Signature: `Series sqrt()`
  Example: `auto out = pos.sqrt();`
- Signature: `Series sin()`
  Example: `auto out = x.sin();`
- Signature: `Series cos()`
  Example: `auto out = x.cos();`
- Signature: `Series tan()`
  Example: `auto out = unit.tan();`
- Signature: `Series asin()`
  Example: `auto out = unit.asin();`
- Signature: `Series acos()`
  Example: `auto out = unit.acos();`
- Signature: `Series atan()`
  Example: `auto out = x.atan();`
- Signature: `Series sinh()`
  Example: `auto out = x.sinh();`
- Signature: `Series cosh()`
  Example: `auto out = x.cosh();`
- Signature: `Series tanh()`
  Example: `auto out = x.tanh();`
- Signature: `Series floor()`
  Example: `auto out = x.div(2.0).floor();`
- Signature: `Series ceil()`
  Example: `auto out = x.div(2.0).ceil();`
- Signature: `Series round()`
  Example: `auto out = x.div(2.0).round();`
- Signature: `Series trunc()`
  Example: `auto out = x.div(2.0).trunc();`
- Signature: `Series reciprocal()`
  Example: `auto out = pos.reciprocal();`
- Signature: `Series sigmoid()`
  Example: `auto out = x.sigmoid();`
- Signature: `Series relu()`
  Example: `auto out = x.relu();`
- Signature: `Series log1p()`
  Example: `auto out = pos.log1p();`
- Signature: `Series expm1()`
  Example: `auto out = unit.expm1();`
- Signature: `Series erf()`
  Example: `auto out = unit.erf();`
- Signature: `Series erfc()`
  Example: `auto out = unit.erfc();`
- Signature: `Series lgamma()`
  Example: `auto out = pos.lgamma();`
- Signature: `Series asinh()`
  Example: `auto out = x.asinh();`
- Signature: `Series acosh()`
  Example: `auto out = pos.add(1.0).acosh();`
- Signature: `Series atanh()`
  Example: `auto out = unit.atanh();`
- Signature: `Series isnan()`
  Example: `auto out = x.isnan();`
- Signature: `Series isinf()`
  Example: `auto out = x.isinf();`
- Signature: `Series isfinite()`
  Example: `auto out = x.isfinite();`
- Signature: `Series sum(std::int64_t, bool)`
  Example: `auto out = mm.sum(1, false);`
- Signature: `Series mean(std::int64_t, bool)`
  Example: `auto out = mm.mean(1, true);`
- Signature: `Series amax(std::int64_t, bool)`
  Example: `auto out = mm.amax(1, false);`
- Signature: `Series amin(std::int64_t, bool)`
  Example: `auto out = mm.amin(1, false);`
- Signature: `Series all(std::int64_t, bool)`
  Example: `auto out = mm.gt(0.0).all(1, false);`
- Signature: `Series any(std::int64_t, bool)`
  Example: `auto out = mm.gt(7.0).any(1, false);`
- Signature: `Series mean()`
  Example: `auto out = mm.mean();`
- Signature: `Series norm(double)`
  Example: `auto out = x.norm(2.0);`
- Signature: `Series var(bool)`
  Example: `auto out = x.var(false);`
- Signature: `Series std(bool)`
  Example: `auto out = x.std(false);`
- Signature: `Series cumsum(std::int64_t, bool)`
  Example: `auto out = mm.cumsum(1, false);`
- Signature: `Series cumprod(std::int64_t, bool)`
  Example: `auto out = mm.cumprod(1, false);`
- Signature: `Series where(const Series&, const Series&)`
  Example: `auto out = x.where(cond, y);`
- Signature: `Series where(const Series&, double)`
  Example: `auto out = x.where(cond, 42.0);`
- Signature: `Series clamp_min(double)`
  Example: `auto out = x.clamp_min(0.0);`
- Signature: `Series clamp_max(double)`
  Example: `auto out = x.clamp_max(0.0);`
- Signature: `Series clamp_min(const Series&)`
  Example: `auto out = x.clamp_min(y);`
- Signature: `Series clamp_max(const Series&)`
  Example: `auto out = x.clamp_max(y);`
- Signature: `Series nan_to_num(double, double, double)`
  Example: `auto out = x.nan_to_num(0.0, 1e9, -1e9);`
- Signature: `Series masked_select(const Series&)`
  Example: `auto out = m.masked_select(mask);`
- Signature: `Series masked_scatter(const Series&, const Series&)`
  Example: `auto out = m.masked_scatter(mask, n);`
- Signature: `Series gather(std::int64_t, const Series&)`
  Example: `auto out = gather_src.gather(1, gather_idx);`
- Signature: `Series scatter(std::int64_t, const Series&, const Series&)`
  Example: `auto out = scatter_base.scatter(1, gather_idx, scatter_val);`
- Signature: `Series scatter_add(std::int64_t, const Series&, const Series&)`
  Example: `auto out = scatter_base.scatter_add(1, gather_idx, scatter_val);`
- Signature: `Series scatter_reduce_sum(std::int64_t, const Series&, const Series&, bool)`
  Example: `auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true);`
- Signature: `Series scatter_reduce_amax(std::int64_t, const Series&, const Series&, bool)`
  Example: `auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true);`
- Signature: `Series scatter_reduce_amin(std::int64_t, const Series&, const Series&, bool)`
  Example: `auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true);`
- Signature: `Series unique(bool)`
  Example: `auto out = i.unique(true);`
- Signature: `Series bincount(std::int64_t)`
  Example: `auto out = i.sub(1.0).to(ScalarType::Int64).bincount(0);`
- Signature: `Series histc(std::int64_t, double, double)`
  Example: `auto out = pos.histc(5, 0.0, 5.0);`
- Signature: `Series& cumsum_(std::int64_t, bool)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("cumsum_", t, 1LL, false);`
- Signature: `Series& cumprod_(std::int64_t, bool)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("cumprod_", t, 1LL, false);`
- Signature: `Series& scatter_(std::int64_t, const Series&, const Series&)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("scatter_", t, 1LL, gather_idx, scatter_val);`
- Signature: `Series& scatter_add_(std::int64_t, const Series&, const Series&)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("scatter_add_", t, 1LL, gather_idx, scatter_val);`
- Signature: `Series& masked_scatter_(const Series&, const Series&)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("masked_scatter_", t, mask, n);`
- Signature: `Series& put_(const Series&, const Series&, bool)`
  Example: `auto t = x.clone(); pluss::codem::_compat::invoke_("put_", t, flat_idx, flat_src, false);`
- Signature: `Series pdf()`
  Example: `auto out = pos.stat.pdf();`
- Signature: `Series logpdf()`
  Example: `auto out = pos.stat.logpdf();`
- Signature: `Series logcdf()`
  Example: `auto out = pos.stat.logcdf();`
- Signature: `Series cdf()`
  Example: `auto out = pos.stat.cdf();`
- Signature: `Series dct()`
  Example: `auto out = x.dct();`
- Signature: `Series idct()`
  Example: `auto out = x.dct().idct();`
- Signature: `Series dst()`
  Example: `auto out = x.dst();`
- Signature: `Series idst()`
  Example: `auto out = x.dst().idst();`
- Signature: `Series hilbert()`
  Example: `auto out = x.hilbert();`
- Signature: `Series histcounts(std::int64_t, double, double)`
  Example: `auto out = pos.histcounts(5, 0.0, 5.0);`
- Signature: `Series skewness()`
  Example: `auto out = x.stat.skewness();`
- Signature: `Series kurtosis()`
  Example: `auto out = x.stat.kurtosis();`
- Signature: `Series nanvar()`
  Example: `auto out = x.nanvar();`
- Signature: `Series nanstd()`
  Example: `auto out = x.nanstd();`
- Signature: `Series ediff1d()`
  Example: `auto out = x.ediff1d();`
- Signature: `Series movsum(std::int64_t)`
  Example: `auto out = x.stat.movsum(3);`
- Signature: `Series movmean(std::int64_t)`
  Example: `auto out = x.stat.movmean(3);`
- Signature: `Series movmax(std::int64_t)`
  Example: `auto out = x.stat.movmax(3);`
- Signature: `Series movmin(std::int64_t)`
  Example: `auto out = x.stat.movmin(3);`
- Signature: `Series movstd(std::int64_t)`
  Example: `auto out = x.stat.movstd(3);`
- Signature: `Series movvar(std::int64_t)`
  Example: `auto out = x.stat.movvar(3);`
- Signature: `Series pdist2(const Series&)`
  Example: `auto out = m.pdist2(n);`
- Signature: `Series cdist(const Series&)`
  Example: `auto out = m.cdist(n);`
- Signature: `Series normr()`
  Example: `auto out = m.normr();`
- Signature: `Series normc()`
  Example: `auto out = m.normc();`
- Signature: `Series standardize()`
  Example: `auto out = x.standardize();`
- Signature: `Series center()`
  Example: `auto out = x.center();`
- Signature: `Series besselj()`
  Example: `auto out = pos.besselj();`
- Signature: `Series bessely()`
  Example: `auto out = pos.bessely();`
- Signature: `Series besseli()`
  Example: `auto out = pos.besseli();`
- Signature: `Series besselk()`
  Example: `auto out = pos.besselk();`
- Signature: `Series beta(double)`
  Example: `auto out = pos.beta(2.0);`
- Signature: `Series betaln(double)`
  Example: `auto out = pos.betaln(2.0);`
- Signature: `Series erfcinv()`
  Example: `auto out = unit.abs().erfcinv();`
- Signature: `Series zeta(double)`
  Example: `auto out = pos.zeta(1.0);`
- Signature: `Series airy()`
  Example: `auto out = pos.airy();`
- Signature: `Series unwrap()`
  Example: `auto out = x.unwrap();`
- Signature: `Series cplxpair()`
  Example: `auto out = x.hilbert().cplxpair();`
- Signature: `Series vdot(const Series&)`
  Example: `auto out = x.vdot(y);`
- Signature: `Series real()`
  Example: `auto out = x.hilbert().real();`
- Signature: `Series imag()`
  Example: `auto out = x.hilbert().imag();`
- Signature: `Series angle()`
  Example: `auto out = x.hilbert().angle();`
- Signature: `Series fftshift()`
  Example: `auto out = mm.fftshift();`
- Signature: `Series ifftshift()`
  Example: `auto out = mm.fftshift().ifftshift();`
- Signature: `Series abs2()`
  Example: `auto out = x.hilbert().abs2();`
- Signature: `Series addbmm(const Series& batch1, const Series& batch2)`
  Example: `auto out = pluss::zeros({2,2}).addbmm(pluss::ones({3,2,4}), pluss::ones({3,4,2}));`
- Signature: `Series addbmm(const Series& batch1, const Series& batch2, double beta, double alpha)`
  Example: `auto out = pluss::zeros({2,2}).addbmm(pluss::ones({3,2,4}), pluss::ones({3,4,2}), 0.5, 2.0);`
- Signature: `Series view_as(const Series& other)`
  Example: `auto out = pluss::arange(0, 6).view_as(pluss::ones({3,2}));`
- Signature: `Series vsplit(std::int64_t sections)`
  Example: `auto out = pluss::arange(0, 12).reshape({6,2}).vsplit(2);`
- Signature: `Series xlogy(const Series& other)`
  Example: `auto out = x.abs().xlogy(y.abs().add(1.0));`
- Signature: `Series xlogy(double scalar)`
  Example: `auto out = x.abs().xlogy(2.0);`
- Signature: `Series& xlogy_(const Series& other)`
  Example: `auto t = x.abs().clone(); t.xlogy_(y.abs().add(1.0));`
- Signature: `Series& zero_()`
  Example: `auto t = x.clone(); t.zero_();`
