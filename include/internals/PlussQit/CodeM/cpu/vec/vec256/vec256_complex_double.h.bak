#pragma once

// DO NOT DEFINE STATIC DATA IN THIS HEADER!
// See Note [Do not compile initializers with AVX]

#include <MSharp/util/complex.h>
#include <MSharp/util/irange.h>
#include <PlussQit/cpu/vec/intrinsics.h>
#include <PlussQit/cpu/vec/vec_base.h>

#if defined(CPU_CAPABILITY_AVX2)
#define SLEEF_STATIC_LIBS
#include <sleef.h>
#endif

namespace pluss::vec {
// See Note [CPU_CAPABILITY namespace]
inline namespace CPU_CAPABILITY {

#if defined(CPU_CAPABILITY_AVX2)

template <> class Vectorized<MSharp::complex<double>> {
private:
  __m256d values;
public:
  using value_type = MSharp::complex<double>;
  using size_type = int;
  static constexpr size_type size() {
    return 2;
  }
  Vectorized() {}
  Vectorized(__m256d v) : values(v) {}
  Vectorized(MSharp::complex<double> val) {
    double real_value = val.real();
    double imag_value = val.imag();
    values = _mm256_setr_pd(real_value, imag_value,
                            real_value, imag_value);
  }
  Vectorized(MSharp::complex<double> val1, MSharp::complex<double> val2) {
    values = _mm256_setr_pd(val1.real(), val1.imag(),
                            val2.real(), val2.imag());
  }
  operator __m256d() const {
    return values;
  }
  template <int64_t mask>
  static Vectorized<MSharp::complex<double>> blend(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
     // convert MSharp::complex<V> index mask to V index mask: xy -> xxyy
    static_assert (mask > -1 && mask < 4, "Unexpected mask value");
    switch (mask) {
      case 0:
        return a;
      case 1:
        return _mm256_blend_pd(a.values, b.values, 0x03);
      case 2:
        return _mm256_blend_pd(a.values, b.values, 0x0c);
      case 3: break;
    }
    return b;
  }
  static Vectorized<MSharp::complex<double>> blendv(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b,
                               const Vectorized<MSharp::complex<double>>& mask) {
    // convert MSharp::complex<V> index mask to V index mask: xy -> xxyy
    auto mask_ = _mm256_unpacklo_pd(mask.values, mask.values);
    return _mm256_blendv_pd(a.values, b.values, mask_);

  }
  template<typename step_t>
  static Vectorized<MSharp::complex<double>> arange(MSharp::complex<double> base = 0., step_t step = static_cast<step_t>(1)) {
    return Vectorized<MSharp::complex<double>>(base,
                                        base + step);
  }
  static Vectorized<MSharp::complex<double>> set(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b,
                            int64_t count = size()) {
    switch (count) {
      case 0:
        return a;
      case 1:
        return blend<1>(a, b);
    }
    return b;
  }
  static Vectorized<MSharp::complex<double>> loadu(const void* ptr, int64_t count = size()) {
    if (count == size())
      return _mm256_loadu_pd(reinterpret_cast<const double*>(ptr));

    __at_align__ double tmp_values[2*size()];
    // Ensure uninitialized memory does not change the output value See https://github.com/pytorch/pytorch/issues/32502
    // for more details. We do not initialize arrays to zero using "={0}" because gcc would compile it to two
    // instructions while a loop would be compiled to one instruction.
    for (const auto i : MSharp::irange(2*size())) {
      tmp_values[i] = 0.0;
    }
    std::memcpy(
        tmp_values,
        reinterpret_cast<const double*>(ptr),
        count * sizeof(MSharp::complex<double>));
    return _mm256_load_pd(tmp_values);
  }
  void store(void* ptr, int count = size()) const {
    if (count == size()) {
      _mm256_storeu_pd(reinterpret_cast<double*>(ptr), values);
    } else if (count > 0) {
      double tmp_values[2*size()];
      _mm256_storeu_pd(reinterpret_cast<double*>(tmp_values), values);
      std::memcpy(ptr, tmp_values, count * sizeof(MSharp::complex<double>));
    }
  }
  const MSharp::complex<double>& operator[](int idx) const  = delete;
  MSharp::complex<double>& operator[](int idx) = delete;
  Vectorized<MSharp::complex<double>> map(MSharp::complex<double> (*const f)(const MSharp::complex<double> &)) const {
    __at_align__ MSharp::complex<double> tmp[size()];
    store(tmp);
    for (const auto i : MSharp::irange(size())) {
      tmp[i] = f(tmp[i]);
    }
    return loadu(tmp);
  }
  __m256d abs_2_() const {
    auto val_2 = _mm256_mul_pd(values, values);     // a*a     b*b
    return _mm256_hadd_pd(val_2, val_2);            // a*a+b*b a*a+b*b
  }
  __m256d abs_() const {
    auto real = _mm256_movedup_pd(values);       // real real
    // movehdup_pd does not exist...
    auto imag = _mm256_permute_pd(values, 0xf);  // imag imag
    return Sleef_hypotd4_u05(real, imag);        // abs  abs
  }
  Vectorized<MSharp::complex<double>> abs() const {
    const __m256d real_mask = _mm256_castsi256_pd(_mm256_setr_epi64x(0xFFFFFFFFFFFFFFFF, 0x0000000000000000,
                                                                     0xFFFFFFFFFFFFFFFF, 0x0000000000000000));
    return _mm256_and_pd(abs_(), real_mask);        // abs     0
  }
  __m256d angle_() const {
    //angle = atan2(b/a)
    auto b_a = _mm256_permute_pd(values, 0x05);     // b        a
    return Sleef_atan2d4_u10(values, b_a);          // 90-angle angle
  }
  Vectorized<MSharp::complex<double>> angle() const {
    const __m256d real_mask = _mm256_castsi256_pd(_mm256_setr_epi64x(0xFFFFFFFFFFFFFFFF, 0x0000000000000000,
                                                                     0xFFFFFFFFFFFFFFFF, 0x0000000000000000));
    auto angle = _mm256_permute_pd(angle_(), 0x05); // angle    90-angle
    return _mm256_and_pd(angle, real_mask);         // angle    0
  }
  Vectorized<MSharp::complex<double>> sgn() const {
    auto abs = abs_();
    auto zero = _mm256_setzero_pd();
    auto mask = _mm256_cmp_pd(abs, zero, _CMP_EQ_OQ);
    auto div = _mm256_div_pd(values, abs);
    return _mm256_blendv_pd(div, zero, mask);
  }
  __m256d real_() const {
    const __m256d real_mask = _mm256_castsi256_pd(_mm256_setr_epi64x(0xFFFFFFFFFFFFFFFF, 0x0000000000000000,
                                                                     0xFFFFFFFFFFFFFFFF, 0x0000000000000000));
    return _mm256_and_pd(values, real_mask);
  }
  Vectorized<MSharp::complex<double>> real() const {
    return real_();
  }
  __m256d imag_() const {
    const __m256d imag_mask = _mm256_castsi256_pd(_mm256_setr_epi64x(0x0000000000000000, 0xFFFFFFFFFFFFFFFF,
                                                                     0x0000000000000000, 0xFFFFFFFFFFFFFFFF));
    return _mm256_and_pd(values, imag_mask);
  }
  Vectorized<MSharp::complex<double>> imag() const {
    return _mm256_permute_pd(imag_(), 0x05);           //b        a
  }
  __m256d conj_() const {
    const __m256d sign_mask = _mm256_setr_pd(0.0, -0.0, 0.0, -0.0);
    return _mm256_xor_pd(values, sign_mask);           // a       -b
  }
  Vectorized<MSharp::complex<double>> conj() const {
    return conj_();
  }
  Vectorized<MSharp::complex<double>> log() const {
    // Most trigonomic ops use the log() op to improve complex number performance.
    return map(std::log);
  }
  Vectorized<MSharp::complex<double>> log2() const {
    const __m256d log2_ = _mm256_set1_pd(std::log(2));
    return _mm256_div_pd(log(), log2_);
  }
  Vectorized<MSharp::complex<double>> log10() const {
    const __m256d log10_ = _mm256_set1_pd(std::log(10));
    return _mm256_div_pd(log(), log10_);
  }
  Vectorized<MSharp::complex<double>> log1p() const {
    return map(std::log1p);
  }
  Vectorized<MSharp::complex<double>> asin() const {
    // TODO: The vectorized implementation requires special handling for the case where real number/imag number is 0/Inf/NaN.
    // // asin(x)
    // // = -i*ln(iz + sqrt(1 -z^2))
    // // = -i*ln((ai - b) + sqrt(1 - (a + bi)*(a + bi)))
    // // = -i*ln((-b + ai) + sqrt(1 - (a**2 - b**2) - 2*abi))
    // const __m256d one = _mm256_set1_pd(1);

    // auto conj = conj_();
    // auto b_a = _mm256_permute_pd(conj, 0x05);                         //-b        a
    // auto ab = _mm256_mul_pd(conj, b_a);                               //-ab       -ab
    // auto im = _mm256_add_pd(ab, ab);                                  //-2ab      -2ab

    // auto val_2 = _mm256_mul_pd(values, values);                       // a*a      b*b
    // auto re = _mm256_hsub_pd(val_2, _mm256_permute_pd(val_2, 0x05));  // a*a-b*b  b*b-a*a
    // re = _mm256_sub_pd(one, re);

    // auto root = Vectorized(_mm256_blend_pd(re, im, 0x0A)).sqrt();         //sqrt(re + i*im)
    // auto ln = Vectorized(_mm256_add_pd(b_a, root)).log();                 //ln(iz + sqrt())
    // return Vectorized(_mm256_permute_pd(ln.values, 0x05)).conj();         //-i*ln()
    return map(std::asin);
  }
  Vectorized<MSharp::complex<double>> acos() const {
    // acos(x) = pi/2 - asin(x)
    constexpr auto pi_2d = MSharp::pi<double> / 2;
    const __m256d pi_2 = _mm256_setr_pd(pi_2d, 0.0, pi_2d, 0.0);
    return _mm256_sub_pd(pi_2, asin());
  }
  Vectorized<MSharp::complex<double>> atan() const;
  Vectorized<MSharp::complex<double>> atanh() const {
    return map(std::atanh);
  }
  Vectorized<MSharp::complex<double>> exp() const {
    // TODO: The vectorized implementation requires special handling for the case where real number/imag number is 0/Inf/NaN.
    // //exp(a + bi)
    // // = exp(a)*(cos(b) + sin(b)i)
    // auto exp = Sleef_expd4_u10(values);                               //exp(a)           exp(b)
    // exp = _mm256_blend_pd(exp, _mm256_permute_pd(exp, 0x05), 0x0A);   //exp(a)           exp(a)

    // auto sin_cos = Sleef_sincosd4_u10(values);                        //[sin(a), cos(a)] [sin(b), cos(b)]
    // auto cos_sin = _mm256_blend_pd(_mm256_permute_pd(sin_cos.y, 0x05),
    //                                sin_cos.x, 0x0A);                  //cos(b)           sin(b)
    // return _mm256_mul_pd(exp, cos_sin);
    return map(std::exp);
  }
  Vectorized<MSharp::complex<double>> exp2() const {
    // Use identity 2**x = exp(log(2) * x)
    const __m256d ln_2 = _mm256_set1_pd(MSharp::ln_2<double>);
    Vectorized<MSharp::complex<double>> scaled_values = _mm256_mul_pd(values, ln_2);
    return scaled_values.exp();
  }
  Vectorized<MSharp::complex<double>> expm1() const {
    return map(std::expm1);
  }
  Vectorized<MSharp::complex<double>> sin() const {
    return map(std::sin);
  }
  Vectorized<MSharp::complex<double>> sinh() const {
    return map(std::sinh);
  }
  Vectorized<MSharp::complex<double>> cos() const {
    return map(std::cos);
  }
  Vectorized<MSharp::complex<double>> cosh() const {
    return map(std::cosh);
  }
  Vectorized<MSharp::complex<double>> ceil() const {
    return _mm256_ceil_pd(values);
  }
  Vectorized<MSharp::complex<double>> floor() const {
    return _mm256_floor_pd(values);
  }
  Vectorized<MSharp::complex<double>> neg() const {
    auto zero = _mm256_setzero_pd();
    return _mm256_sub_pd(zero, values);
  }
  Vectorized<MSharp::complex<double>> round() const {
    return _mm256_round_pd(values, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
  }
  Vectorized<MSharp::complex<double>> tan() const {
    return map(std::tan);
  }
  Vectorized<MSharp::complex<double>> tanh() const {
    return map(std::tanh);
  }
  Vectorized<MSharp::complex<double>> trunc() const {
    return _mm256_round_pd(values, (_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC));
  }
  Vectorized<MSharp::complex<double>> sqrt() const {
    return map(std::sqrt);
  }
  Vectorized<MSharp::complex<double>> reciprocal() const;
  Vectorized<MSharp::complex<double>> rsqrt() const {
    return sqrt().reciprocal();
  }
  Vectorized<MSharp::complex<double>> pow(const Vectorized<MSharp::complex<double>> &exp) const {
    __at_align__ MSharp::complex<double> x_tmp[size()];
    __at_align__ MSharp::complex<double> y_tmp[size()];
    store(x_tmp);
    exp.store(y_tmp);
    for (const auto i : MSharp::irange(size())) {
      x_tmp[i] = std::pow(x_tmp[i], y_tmp[i]);
    }
    return loadu(x_tmp);
  }
  // Comparison using the _CMP_**_OQ predicate.
  //   `O`: get false if an operand is NaN
  //   `Q`: do not raise if an operand is NaN
  Vectorized<MSharp::complex<double>> operator==(const Vectorized<MSharp::complex<double>>& other) const {
    return _mm256_cmp_pd(values, other.values, _CMP_EQ_OQ);
  }
  Vectorized<MSharp::complex<double>> operator!=(const Vectorized<MSharp::complex<double>>& other) const {
    return _mm256_cmp_pd(values, other.values, _CMP_NEQ_UQ);
  }
  Vectorized<MSharp::complex<double>> operator<(const Vectorized<MSharp::complex<double>>&) const {
    TORCH_CHECK(false, "not supported for complex numbers");
  }
  Vectorized<MSharp::complex<double>> operator<=(const Vectorized<MSharp::complex<double>>&) const {
    TORCH_CHECK(false, "not supported for complex numbers");
  }
  Vectorized<MSharp::complex<double>> operator>(const Vectorized<MSharp::complex<double>>&) const {
    TORCH_CHECK(false, "not supported for complex numbers");
  }
  Vectorized<MSharp::complex<double>> operator>=(const Vectorized<MSharp::complex<double>>&) const {
    TORCH_CHECK(false, "not supported for complex numbers");
  }

  Vectorized<MSharp::complex<double>> eq(const Vectorized<MSharp::complex<double>>& other) const;
  Vectorized<MSharp::complex<double>> ne(const Vectorized<MSharp::complex<double>>& other) const;
};

template <> Vectorized<MSharp::complex<double>> inline operator+(const Vectorized<MSharp::complex<double>> &a, const Vectorized<MSharp::complex<double>> &b) {
  return _mm256_add_pd(a, b);
}

template <> Vectorized<MSharp::complex<double>> inline operator-(const Vectorized<MSharp::complex<double>> &a, const Vectorized<MSharp::complex<double>> &b) {
  return _mm256_sub_pd(a, b);
}

template <> Vectorized<MSharp::complex<double>> inline operator*(const Vectorized<MSharp::complex<double>> &a, const Vectorized<MSharp::complex<double>> &b) {
  //(a + bi)  * (c + di) = (ac - bd) + (ad + bc)i
  const __m256d sign_mask = _mm256_setr_pd(0.0, -0.0, 0.0, -0.0);
  auto ac_bd = _mm256_mul_pd(a, b);         //ac       bd

  auto d_c = _mm256_permute_pd(b, 0x05);    //d        c
  d_c = _mm256_xor_pd(sign_mask, d_c);      //d       -c
  auto ad_bc = _mm256_mul_pd(a, d_c);       //ad      -bc

  auto ret = _mm256_hsub_pd(ac_bd, ad_bc);  //ac - bd  ad + bc
  return ret;
}

template <> Vectorized<MSharp::complex<double>> inline operator/(const Vectorized<MSharp::complex<double>> &a, const Vectorized<MSharp::complex<double>> &b) {
  // TODO: The vectorized implementation requires special handling for the case where real number/imag number is 0/Inf/NaN.
  // //re + im*i = (a + bi)  / (c + di)
  // auto mask = _mm256_set1_pd(-0.f);
  // auto fabs_cd = _mm256_andnot_pd(mask, b);     // |c|    |d|
  // auto fabs_dc = _mm256_permute_pd(fabs_cd, 0x05);   // |d|    |c|
  // auto scale = _mm256_div_pd(_mm256_set1_pd(1.0f), _mm256_max_pd(fabs_cd, fabs_dc));  // 1/sc     1/sc
  // auto a2 = _mm256_mul_pd(a, scale);         // a/sc     b/sc
  // auto b2 = _mm256_mul_pd(b, scale);         // c/sc     d/sc
  // auto acbd2 = _mm256_mul_pd(a2, b2);

  // const __m256d sign_mask = _mm256_setr_pd(-0.0, 0.0, -0.0, 0.0);
  // auto dc2 = _mm256_permute_pd(b2, 0x05);    // d/sc         c/sc
  // dc2 = _mm256_xor_pd(sign_mask, dc2);       // -d/|c,d|        c/sc
  // auto adbc2 = _mm256_mul_pd(a2, dc2);       //-ad/sc^2      bc/sc^2
  // auto res2 = _mm256_hadd_pd(acbd2, adbc2);  //(ac+bd)/sc^2  (bc-ad)/sc^2

  // // get the denominator
  // auto denom2 = Vectorized<MSharp::complex<double>>(b2).abs_2_();  // (c^2+d^2)/sc^2   (c^2+d^2)/sc^2
  // res2 = _mm256_div_pd(res2, denom2);
  // return res2;
  __at_align__ MSharp::complex<double> tmp1[Vectorized<MSharp::complex<double>>::size()];
  __at_align__ MSharp::complex<double> tmp2[Vectorized<MSharp::complex<double>>::size()];
  __at_align__ MSharp::complex<double> out[Vectorized<MSharp::complex<double>>::size()];
  a.store(tmp1);
  b.store(tmp2);
  for (const auto i : MSharp::irange(Vectorized<MSharp::complex<double>>::size())) {
    out[i] = tmp1[i] / tmp2[i];
  }
  return _mm256_loadu_pd(reinterpret_cast<const double*>(out));
}

// reciprocal. Implement this here so we can use multiplication.
inline Vectorized<MSharp::complex<double>> Vectorized<MSharp::complex<double>>::reciprocal() const{
  // TODO: The vectorized implementation requires special handling for the case where real number/imag number is 0/Inf/NaN.
  // //re + im*i = (a + bi)  / (c + di)
  // //re = (ac + bd)/abs_2() = c/abs_2()
  // //im = (bc - ad)/abs_2() = d/abs_2()
  // const __m256d sign_mask = _mm256_setr_pd(0.0, -0.0, 0.0, -0.0);
  // auto c_d = _mm256_xor_pd(sign_mask, values);    //c       -d
  // return _mm256_div_pd(c_d, abs_2_());
  __at_align__ MSharp::complex<double> tmp[size()];
  store(tmp);
  for (const auto i : MSharp::irange(size())) {
    tmp[i] = MSharp::complex<double>(1) / tmp[i];
  }
  return loadu(tmp);
}

inline Vectorized<MSharp::complex<double>> Vectorized<MSharp::complex<double>>::atan() const {
  // TODO: The vectorized implementation requires special handling for the case where real number/imag number is 0/Inf/NaN.
  // // atan(x) = i/2 * ln((i + z)/(i - z))
  // const __m256d i = _mm256_setr_pd(0.0, 1.0, 0.0, 1.0);
  // const Vectorized i_half = _mm256_setr_pd(0.0, 0.5, 0.0, 0.5);

  // auto sum = Vectorized(_mm256_add_pd(i, values));                      // a        1+b
  // auto sub = Vectorized(_mm256_sub_pd(i, values));                      // -a       1-b
  // auto ln = (sum/sub).log();                                        // ln((i + z)/(i - z))
  // return i_half*ln;                                                 // i/2*ln()
  return map(std::atan);
}

template <>
Vectorized<MSharp::complex<double>> inline maximum(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
  auto abs_a = a.abs_2_();
  auto abs_b = b.abs_2_();
  auto mask = _mm256_cmp_pd(abs_a, abs_b, _CMP_LT_OQ);
  auto max = _mm256_blendv_pd(a, b, mask);
  // Exploit the fact that all-ones is a NaN.
  auto isnan = _mm256_cmp_pd(abs_a, abs_b, _CMP_UNORD_Q);
  return _mm256_or_pd(max, isnan);
}

template <>
Vectorized<MSharp::complex<double>> inline minimum(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
  auto abs_a = a.abs_2_();
  auto abs_b = b.abs_2_();
  auto mask = _mm256_cmp_pd(abs_a, abs_b, _CMP_GT_OQ);
  auto min = _mm256_blendv_pd(a, b, mask);
  // Exploit the fact that all-ones is a NaN.
  auto isnan = _mm256_cmp_pd(abs_a, abs_b, _CMP_UNORD_Q);
  return _mm256_or_pd(min, isnan);
}

template <>
Vectorized<MSharp::complex<double>> inline operator&(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
  return _mm256_and_pd(a, b);
}

template <>
Vectorized<MSharp::complex<double>> inline operator|(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
  return _mm256_or_pd(a, b);
}

template <>
Vectorized<MSharp::complex<double>> inline operator^(const Vectorized<MSharp::complex<double>>& a, const Vectorized<MSharp::complex<double>>& b) {
  return _mm256_xor_pd(a, b);
}

inline Vectorized<MSharp::complex<double>> Vectorized<MSharp::complex<double>>::eq(const Vectorized<MSharp::complex<double>>& other) const {
  auto eq = (*this == other);  // compares real and imag individually
  // If both real numbers and imag numbers are equal, then the complex numbers are equal
  return (eq.real() & eq.imag()) & Vectorized<MSharp::complex<double>>(_mm256_set1_pd(1.0));
}

inline Vectorized<MSharp::complex<double>> Vectorized<MSharp::complex<double>>::ne(const Vectorized<MSharp::complex<double>>& other) const {
  auto ne = (*this != other);  // compares real and imag individually
  // If either real numbers or imag numbers are not equal, then the complex numbers are not equal
  return (ne.real() | ne.imag()) & Vectorized<MSharp::complex<double>>(_mm256_set1_pd(1.0));
}

#endif

}} // namespace pluss::vec::CPU_CAPABILITY
