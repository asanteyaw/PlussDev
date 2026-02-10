#pragma once

#include <PlussQit/core/SeriesBase.h>
#include <MSharp/util/Exception.h>

namespace pluss {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class PLUSS_API OptionalSeriesRef {
 public:
  OptionalSeriesRef() = default;

  ~OptionalSeriesRef() {
    ref_.unsafeReleaseSeriesImpl();
  }

  OptionalSeriesRef(const SeriesBase& src)
      : ref_(Series::unsafe_borrow_t{}, src) {
    TORCH_INTERNAL_ASSERT_DEBUG_ONLY(src.defined());
  }

    OptionalSeriesRef(const OptionalSeriesRef& rhs)
      : ref_(Series::unsafe_borrow_t{}, rhs.ref_) {}

  OptionalSeriesRef(OptionalSeriesRef&& rhs) = default;
  OptionalSeriesRef& operator=(OptionalSeriesRef rhs) {
    std::swap(ref_, rhs.ref_);
    return *this;
  }

  bool has_value() const {
    return ref_.defined();
  }

  const Series& getSeriesRef() const & {
    return ref_;
  }

  const Series& operator*() const & {
    return ref_;
  }

  const Series* operator->() const & {
    return &ref_;
  }

  operator bool() const {
    return ref_.defined();
  }

 private:
  Series ref_;
};

// Use to convert a SeriesBase (that may be undefined) to a pluss::Series
// without bumping refcount.
class PLUSS_API SeriesRef {
 public:
  ~SeriesRef() {
    ref_.unsafeReleaseSeriesImpl();
  }

    SeriesRef(const SeriesBase& src)
      : ref_(Series::unsafe_borrow_t{}, src) {}
    SeriesRef(SeriesRef&& other) = default;
    SeriesRef(const SeriesRef&) = default;
    SeriesRef& operator=(const SeriesRef&) = default;
    SeriesRef& operator=(SeriesRef&&) = default;

  const Series& operator*() const & {
    return ref_;
  }
 private:
  Series ref_;
};

template <typename T>
auto Series::register_hook(T&& hook) const -> Series::hook_return_void_t<T> {
  // Return the grad argument in case of a hook with void return type to have an
  // std::function with Series return type
  static_assert(std::is_same_v<decltype(hook(Series())), void>,
                "Expected hook to return void");
  return _register_hook([fn=std::forward<T>(hook)](const SeriesBase& grad_base) {
    SeriesRef grad(grad_base);
    fn(*grad);
    return Series();
  });
}

template <typename T>
auto Series::register_hook(T&& hook) const -> Series::hook_return_var_t<T> {
  return _register_hook([fn=std::forward<T>(hook)](const SeriesBase& grad_base) {
    SeriesRef grad(grad_base);
    Series ret = fn(*grad);
    return SeriesBase(std::move(ret));
  });
}

} // namespace pluss
