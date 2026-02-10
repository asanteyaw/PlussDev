#if !defined(C10_INTERNAL_INCLUDE_COMPLEX_REMAINING_H)
#error \
    "c10/util/complex_utils.h is not meant to be individually included. Include c10/util/complex.h instead."
#endif

#include <limits>

namespace MSharp {

template <typename T>
struct is_complex : public std::false_type {};

template <typename T>
struct is_complex<std::complex<T>> : public std::true_type {};

template <typename T>
struct is_complex<MSharp::complex<T>> : public std::true_type {};

// Extract double from std::complex<double>; is identity otherwise
// TODO: Write in more idiomatic C++17
template <typename T>
struct scalar_value_type {
  using type = T;
};
template <typename T>
struct scalar_value_type<std::complex<T>> {
  using type = T;
};
template <typename T>
struct scalar_value_type<MSharp::complex<T>> {
  using type = T;
};

} // namespace MSharp

namespace std {

template <typename T>
class numeric_limits<MSharp::complex<T>> : public numeric_limits<T> {};

template <typename T>
bool isnan(const MSharp::complex<T>& v) {
  return std::isnan(v.real()) || std::isnan(v.imag());
}

} // namespace std
