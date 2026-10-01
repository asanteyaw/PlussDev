#include <pluss/pluss.h>

#include <iostream>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}
}  // namespace

int main() {
  heading("cumprod_(dim)");
  { auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
    x.cumprod_(1); std::cout << x; }
  heading("cumsum_(dim)");
  { auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
    x.cumsum_(1); std::cout << x; }
  heading("deg2rad_()");
  { auto x = pluss::make_series({0.0, 90.0, 180.0});
    x.deg2rad_(); std::cout << x; }
  heading("rad2deg_()");
  { auto x = pluss::make_series({0.0, 1.5707963267948966, 3.141592653589793});
    x.rad2deg_(); std::cout << x; }
  heading("float_power_(Series)");
  { auto x = pluss::make_series({2.0, 3.0});
    auto y = pluss::make_series({2.0, 3.0});
    x.float_power_(y); std::cout << x; }
  heading("igamma_(Series)");
  { auto x = pluss::make_series({1.0, 2.0});
    auto y = pluss::make_series({1.0, 1.0});
    x.igamma_(y); std::cout << x; }
  heading("igammac_(Series)");
  { auto x = pluss::make_series({1.0, 2.0});
    auto y = pluss::make_series({1.0, 1.0});
    x.igammac_(y); std::cout << x; }
  heading("mvlgamma_(p)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    x.mvlgamma_(1); std::cout << x; }
  heading("polygamma_(n)");
  { auto x = pluss::make_series({1.0, 2.0});
    x.polygamma_(0); std::cout << x; }
  heading("renorm_(p, dim, maxnorm)");
  { auto x = pluss::make_series({{3.0, 4.0}, {6.0, 8.0}});
    x.renorm_(2.0, 0, 1.0); std::cout << x; }
  heading("squeeze_()");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3});
    x.squeeze_(); std::cout << x; }
  heading("squeeze_(dim)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3});
    x.squeeze_(0); std::cout << x; }
  heading("swapaxes_(dim0, dim1)");
  { auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    x.swapaxes_(0, 1); std::cout << x; }
  heading("swapdims_(dim0, dim1)");
  { auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    x.swapdims_(0, 1); std::cout << x; }
  heading("t_()");
  { auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    x.t_(); std::cout << x; }
  heading("transpose_(dim0, dim1)");
  { auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    x.transpose_(0, 1); std::cout << x; }
  heading("unsqueeze_(dim)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    x.unsqueeze_(0); std::cout << x; }
  heading("as_strided_(sizes, strides)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6});
    const std::array<std::int64_t, 2> sizes{2, 3};
    const std::array<std::int64_t, 2> strides{3, 1};
    x.as_strided_(pluss::msharp::RangeView::Int(sizes.data(), sizes.size()),
                   pluss::msharp::RangeView::Int(strides.data(), strides.size()));
    std::cout << x; }
  heading("index_add_(dim, index, source)");
  { auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);
    auto src = pluss::make_series({5.0, 7.0}, {2});
    x.index_add_(0, idx, src); std::cout << x; }
  heading("index_copy_(dim, index, source)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);
    auto src = pluss::make_series({9.0, 8.0}, {2});
    x.index_copy_(0, idx, src); std::cout << x; }
  heading("index_fill_(dim, index, value)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);
    x.index_fill_(0, idx, 9.0); std::cout << x; }
  heading("index_reduce_(dim, index, source, reduce)");
  { auto x = pluss::make_series({1.0, 1.0, 1.0}, {3});
    auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);
    auto src = pluss::make_series({5.0, 7.0}, {2});
    x.index_reduce_(0, idx, src, "prod"); std::cout << x; }
  heading("masked_fill_(mask, double value)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
    x.masked_fill_(mask, 0.0); std::cout << x; }
  heading("masked_fill_(mask, Series value)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
    auto value = pluss::make_series(0.0);
    x.masked_fill_(mask, value); std::cout << x; }

  heading("abs_()");
  { auto x = pluss::make_series({-3.0, -1.0, 4.0});
    auto& ref = x.abs_();
    std::cout << "x=" << x << "\n";
    std::cout << std::boolalpha << "same object: " << (&ref == &x); }

  return 0;
}
