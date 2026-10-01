#include <pluss/pluss.h>
#include <iostream>
namespace { void heading(const char* l){ std::cout << "\n=== " << l << " ===\n"; } }
int main() {
  heading("absolute_ == abs_");
  { auto x = pluss::make_series({-3.0, 1.0}); x.absolute_(); std::cout << x; }
  heading("arccos_ == acos_");
  { auto x = pluss::make_series({0.5}); x.arccos_(); std::cout << x; }
  heading("divide_(Series) == div_(Series)");
  { auto x = pluss::make_series({10.0}); auto y = pluss::make_series({2.0}); x.divide_(y); std::cout << x; }
  heading("greater_(Series) == gt_(Series)");
  { auto x = pluss::make_series({1.0,2.0}); auto y = pluss::make_series({0.0,5.0}); x.greater_(y); std::cout << x; }
  heading("positive_()");
  { auto x = pluss::make_series({-1.0, 2.0}); auto& ref = x.positive_(); std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("addbmm_");
  { auto x = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
    auto batch1 = pluss::make_series({{{1.0, 0.0}, {0.0, 1.0}}, {{1.0, 0.0}, {0.0, 1.0}}});
    auto batch2 = pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}}, {{1.0, 1.0}, {1.0, 1.0}}});
    x.addbmm_(batch1, batch2, 1.0, 1.0);
    std::cout << x; }

  heading("fill_(double)");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    x.fill_(9.0); std::cout << x; }

  heading("zero_()");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    x.zero_(); std::cout << x; }

  heading("fill_diagonal_(value)");
  { auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
    x.fill_diagonal_(0.0); std::cout << x; }

  heading("conj_physical_() on real dtype");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    x.conj_physical_(); std::cout << x; }

  heading("bitwise_left_shift_(Series)");
  { auto x = pluss::make_series({1, 2}, pluss::ScalarType::Int64);
    auto y = pluss::make_series({2, 3}, pluss::ScalarType::Int64);
    x.bitwise_left_shift_(y); std::cout << x; }

  heading("bitwise_right_shift_(Series)");
  { auto x = pluss::make_series({16, 8}, pluss::ScalarType::Int64);
    auto y = pluss::make_series({2, 1}, pluss::ScalarType::Int64);
    x.bitwise_right_shift_(y); std::cout << x; }

  heading("set_(Series other)");
  { auto x = pluss::make_series({1.0, 2.0});
    auto other = pluss::make_series({7.0, 8.0, 9.0});
    x.set_(other); std::cout << x; }

  heading("set_() no-op");
  { auto x = pluss::make_series({1.0, 2.0});
    auto& ref = x.set_();
    std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("rename_() no-op");
  { auto x = pluss::make_series({1.0, 2.0});
    auto& ref = x.rename_();
    std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("sparse_resize_() no-op");
  { auto x = pluss::make_series({1.0, 2.0});
    auto& ref = x.sparse_resize_();
    std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("sparse_resize_and_clear_() no-op");
  { auto x = pluss::make_series({1.0, 2.0});
    auto& ref = x.sparse_resize_and_clear_();
    std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("resize_as_sparse_() no-op");
  { auto x = pluss::make_series({1.0, 2.0});
    auto& ref = x.resize_as_sparse_();
    std::cout << "x=" << x << "\nsame object: " << std::boolalpha << (&ref==&x); }

  heading("resize_(sizes) -- uninitialized result, shape only");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    const std::array<std::int64_t, 1> sizes{5};
    x.resize_(pluss::msharp::RangeView::Int(sizes.data(), sizes.size()));
    std::cout << "shape=[" << x.sizes()[0] << "] numel=" << x.numel(); }

  heading("resize_as_(other) -- uninitialized result, shape only");
  { auto x = pluss::make_series({1.0, 2.0, 3.0});
    auto other = pluss::make_series({{1.0,2.0},{3.0,4.0}});
    x.resize_as_(other);
    std::cout << "shape=[" << x.sizes()[0] << "," << x.sizes()[1] << "] numel=" << x.numel(); }

  return 0;
}
