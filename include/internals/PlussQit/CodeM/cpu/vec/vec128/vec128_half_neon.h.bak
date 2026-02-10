#pragma once

// DO NOT DEFINE STATIC DATA IN THIS HEADER!
// See Note [Do not compile initializers with AVX]

#include <PlussQit/cpu/vec/intrinsics.h>
#include <PlussQit/cpu/vec/vec128/vec128_convert.h>
#include <PlussQit/cpu/vec/vec128/vec128_float_neon.h>
#include <PlussQit/cpu/vec/vec128/vec128_reduced_precision_common_neon.h>
#include <PlussQit/cpu/vec/vec_base.h>
#include <MSharp/util/Half.h>
#include <MSharp/util/irange.h>

namespace pluss::vec {
// See Note [CPU_CAPABILITY namespace]
inline namespace CPU_CAPABILITY {

// Right now contains only aarch64 implementation.
// Due to follow two reasons aarch32 is not currently supported.
// 1. Due to difference in ISA been aarch32 and aarch64, intrinsics
//    that work for aarch64 dont work for aarch32.
// 2. Android NDK r21 has problems with compiling aarch32.
//    Clang seg faults.
//    https://github.com/android/ndk/issues/1248
//    https://bugs.llvm.org/show_bug.cgi?id=45824
// Most likely we will do aarch32 support with inline asm.
#if !defined(C10_MOBILE) && defined(__aarch64__)

#ifdef __BIG_ENDIAN__
#error "Big endian is not supported."
#endif

template <int index, bool mask_val>
struct BlendHalfRegs {
  static float16x8_t impl(
      const float16x8_t& a,
      const float16x8_t& b,
      float16x8_t& res);
};

template <int index>
struct BlendHalfRegs<index, true> {
  static float16x8_t impl(
      const float16x8_t& a,
      const float16x8_t& b,
      float16x8_t& res) {
    return vsetq_lane_f16(vgetq_lane_f16(b, index), res, index);
  }
};

template <int index>
struct BlendHalfRegs<index, false> {
  static float16x8_t impl(
      const float16x8_t& a,
      const float16x8_t& b,
      float16x8_t& res) {
    return vsetq_lane_f16(vgetq_lane_f16(a, index), res, index);
  }
};

// On ARM, Half type supports float16_t->Half constructor and Half->float16_t
// conversion
template <>
class Vectorized<MSharp::Half> : public Vectorized16<float16x8_t, MSharp::Half, BlendHalfRegs, Vectorized<MSharp::Half>> {
  using Base = Vectorized16<float16x8_t, MSharp::Half, BlendHalfRegs, Vectorized<MSharp::Half>>;
  friend Base;
 private:
  // We use these private map functions to implement various methods
  Vectorized<MSharp::Half> map_with_vec_float_method(
      Vectorized<float> (Vectorized<float>::*m)() const) const {
    float32x4_t v00 = vcvt_f32_f16(vget_low_f16(values));
    float32x4_t v01 = vcvt_f32_f16(vget_high_f16(values));
    Vectorized<float> mv0 = (Vectorized<float>(v00).*m)();
    Vectorized<float> mv1 = (Vectorized<float>(v01).*m)();
    float16x4_t r00 = vcvt_f16_f32(mv0);
    float16x4_t r01 = vcvt_f16_f32(mv1);
    return Vectorized<MSharp::Half>(vcombine_f16(r00, r01));
  }

  Vectorized<MSharp::Half> map2_with_vec_float_method(
      const Vectorized<MSharp::Half>& second,
      Vectorized<float> (Vectorized<float>::*m)(const Vectorized<float>&)
          const) const {
    float32x4_t v00 = vcvt_f32_f16(vget_low_f16(values));
    float32x4_t v01 = vcvt_f32_f16(vget_high_f16(values));
    float32x4_t second_v00 = vcvt_f32_f16(vget_low_f16(second.values));
    float32x4_t second_v01 = vcvt_f32_f16(vget_high_f16(second.values));
    Vectorized<float> mv0 = (Vectorized<float>(v00).*m)(Vectorized<float>(second_v00));
    Vectorized<float> mv1 = (Vectorized<float>(v01).*m)(Vectorized<float>(second_v01));
    float16x4_t r00 = vcvt_f16_f32(mv0);
    float16x4_t r01 = vcvt_f16_f32(mv1);

    // Pack result into Vectorized<MSharp::Half>
    return Vectorized<MSharp::Half>(vcombine_f16(r00, r01));
  }

  Vectorized<MSharp::Half> map2_bitmask_with_vec_float_method(
      const Vectorized<MSharp::Half>& second,
      Vectorized<float> (Vectorized<float>::*m)(const Vectorized<float>&)
          const) const {
    float32x4_t v00 = vcvt_f32_f16(vget_low_f16(values));
    float32x4_t v01 = vcvt_f32_f16(vget_high_f16(values));
    float32x4_t second_v00 = vcvt_f32_f16(vget_low_f16(second.values));
    float32x4_t second_v01 = vcvt_f32_f16(vget_high_f16(second.values));
    Vectorized<float> mv0 = (Vectorized<float>(v00).*m)(Vectorized<float>(second_v00));
    Vectorized<float> mv1 = (Vectorized<float>(v01).*m)(Vectorized<float>(second_v01));
    // Assume the operator returns a bitmask, not "real" floats, and
    // just narrow the bits. All-ones is a NaN and will get mangled by conversion!
    float16x4_t r00 = vreinterpret_f16_u16(vmovn_u32(vreinterpretq_u32_f32(mv0)));
    float16x4_t r01 = vreinterpret_f16_u16(vmovn_u32(vreinterpretq_u32_f32(mv1)));

    // Pack result into Vectorized<MSharp::Half>
    return Vectorized<MSharp::Half>(vcombine_f16(r00, r01));
  }

 public:
  using Vectorized16::Vectorized16;

  Vectorized() = default;

  // A ctor that accepts MSharp::Half is needed to fit interface with vec_base.h
  // A second constructor that takes float16_t is also included
  Vectorized(MSharp::Half val)
      : Vectorized((float16_t)val) {}
  Vectorized(float16_t val)
      : Vectorized16(vdupq_n_f16(val)) {}
  Vectorized(
      value_type val0,
      value_type val1,
      value_type val2,
      value_type val3,
      value_type val4,
      value_type val5,
      value_type val6,
      value_type val7)
      : Vectorized16(float16x8_t{
            val0,
            val1,
            val2,
            val3,
            val4,
            val5,
            val6,
            val7}) {}


  static Vectorized<MSharp::Half> blendv(
      const Vectorized<MSharp::Half>& a,
      const Vectorized<MSharp::Half>& b,
      const Vectorized<MSharp::Half>& mask) {
    // Note: using blendv is very awkward because 0xFFFF is one of
    // many NaN's in FP16 It's unfortunate that the mask has type Half
    // (required from vec_base)

    // TODO
    // NB: This requires that each value, i.e., each uint value,
    // of the mask either all be zeros or all be 1s.
    // We perhaps need some kind of an assert?
    // But that will affect performance.

    // NOTE [vbslq_f16]: vbslq_f16 doesn't work on clang without
    // __ARM_FEATURE_FP16_VECTOR_ARITHMETIC. vbslq_u16 generates the
    // same instruction anyway. see https://godbolt.org/z/cY4a55Y7P
    Vectorized<MSharp::Half> vec(mask.values);
    vec.values = vreinterpretq_f16_u16(
        vbslq_u16(
            vreinterpretq_u16_f16(vec.values),
            vreinterpretq_u16_f16(b.values),
            vreinterpretq_u16_f16(a.values)));
    return vec;
  }
  static Vectorized<MSharp::Half> set(
      const Vectorized<MSharp::Half>& a,
      const Vectorized<MSharp::Half>& b,
      int64_t count = size()) {
    uint16_t pre_mask[size()] = {0};
    for (int i = 0; i < count; i++) {
      pre_mask[i] = 0xFFFF;
    }
    uint16x8_t mask = vld1q_u16(pre_mask);

    // Using blendv is awkward because 0xFFFF is one of many NaN's in FP16
    // so we directly use vbslq_u16 instead. (See NOTE [vbslq_f16] above.)
    Vectorized<MSharp::Half> vec(
        vreinterpretq_f16_u16(
            vbslq_u16(
                mask,
                vreinterpretq_u16_f16(b.values),
                vreinterpretq_u16_f16(a.values))));

    return vec;
  }
  static Vectorized<MSharp::Half> loadu(const void* ptr, int64_t count = size()) {
    if (count == size()) {
      return vld1q_f16(reinterpret_cast<const float16_t*>(ptr));
    }
    __at_align__ float16_t tmp_values[size()];
    for (const auto i : MSharp::irange(size())) {
      tmp_values[i] = 0;
    }
    std::memcpy(
        tmp_values,
        reinterpret_cast<const float16_t*>(ptr),
        count * sizeof(float16_t));
    return vld1q_f16(reinterpret_cast<const float16_t*>(tmp_values));
  }
  void store(void* ptr, int64_t count = size()) const {
    if (count == size()) {
      vst1q_f16(reinterpret_cast<float16_t*>(ptr), values);
      return;
    } else {
      float16_t tmp_values[size()];
      vst1q_f16(reinterpret_cast<float16_t*>(tmp_values), values);
      std::memcpy(ptr, tmp_values, count * sizeof(float16_t));
    }
  }
  // For boolean version where we want to if any 1/all zero
  // etc. can be done faster in a different way.
  Vectorized<MSharp::Half> isnan() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return vreinterpretq_f16_u16(vmvnq_u16(vceqq_f16(values, values)));
#else
    // NOTE: we could make this faster by doing vectorized checks of
    // exponent/payload bits.
    __at_align__ MSharp::Half tmp[size()];
    __at_align__ MSharp::Half res[size()];
    store(tmp);
    for (const auto i : MSharp::irange(size())) {
      if (_isnan(tmp[i])) {
        std::memset(static_cast<void*>(&res[i]), 0xFF, sizeof(MSharp::Half));
      } else {
        std::memset(static_cast<void*>(&res[i]), 0, sizeof(MSharp::Half));
      }
    }
    return loadu(res);
#endif
  }
  bool has_inf_nan() const {
    __at_align__ MSharp::Half tmp[size()];
    store(tmp);
    for (const auto i : MSharp::irange(size())) {
      if (_isnan(tmp[i]) || _isinf(tmp[i])) {
        return true;
      }
    }
    return false;
  }
  Vectorized<MSharp::Half> abs() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vabsq_f16(values));
#else
    return map_with_vec_float_method(&Vectorized<float>::abs);
#endif
  }
  Vectorized<MSharp::Half> frac() const;
  Vectorized<MSharp::Half> neg() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vnegq_f16(values));
#else
    return map_with_vec_float_method(&Vectorized<float>::neg);
#endif
  }
  Vectorized<MSharp::Half> trunc() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vrndq_f16(values));
#else
    return map_with_vec_float_method(&Vectorized<float>::trunc);
#endif
  }
  Vectorized<MSharp::Half> sqrt() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vsqrtq_f16(values));
#else
    return map_with_vec_float_method(&Vectorized<float>::sqrt);
#endif
  }
  Vectorized<MSharp::Half> reciprocal() const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    auto ones = vdupq_n_f16(1.0f);
    return Vectorized<MSharp::Half>(vdivq_f16(ones, values));
#else
    return map_with_vec_float_method(&Vectorized<float>::reciprocal);
#endif
  }
  Vectorized<MSharp::Half> operator==(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vceqq_f16(values, other.values)));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator==);
#endif
  }

  Vectorized<MSharp::Half> operator!=(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(
                                     vmvnq_u16(vceqq_f16(values, other.values))));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator!=);
#endif
  }

  Vectorized<MSharp::Half> operator<(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vcltq_f16(values, other.values)));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator<);
#endif
  }

  Vectorized<MSharp::Half> operator<=(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vcleq_f16(values, other.values)));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator<=);
#endif
  }

  Vectorized<MSharp::Half> operator>(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vcgtq_f16(values, other.values)));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator>);
#endif
  }

  Vectorized<MSharp::Half> operator>=(const Vectorized<MSharp::Half>& other) const {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vcgeq_f16(values, other.values)));
#else
    return map2_bitmask_with_vec_float_method(other, &Vectorized<float>::operator>=);
#endif
  }

  Vectorized<MSharp::Half> eq(const Vectorized<MSharp::Half>& other) const;
  Vectorized<MSharp::Half> ne(const Vectorized<MSharp::Half>& other) const;
  Vectorized<MSharp::Half> gt(const Vectorized<MSharp::Half>& other) const;
  Vectorized<MSharp::Half> ge(const Vectorized<MSharp::Half>& other) const;
  Vectorized<MSharp::Half> lt(const Vectorized<MSharp::Half>& other) const;
  Vectorized<MSharp::Half> le(const Vectorized<MSharp::Half>& other) const;
}; // Vectorized<Half>

inline std::tuple<Vectorized<float>, Vectorized<float>> convert_half_float(const Vectorized<Half>& a) {
  static_assert(Vectorized<Half>::size() == 2 * Vectorized<float>::size());
  float16x8_t x = a;
  float32x4_t x1 = vcvt_f32_f16(vget_low_f16(x));
  float32x4_t x2 = vcvt_f32_f16(vget_high_f16(x));
  return { Vectorized<float>(x1), Vectorized<float>(x2) };
}
inline Vectorized<Half> convert_float_half(const Vectorized<float>& a, const Vectorized<float>& b) {
  static_assert(Vectorized<Half>::size() == 2 * Vectorized<float>::size());
  float32x4_t x = a;
  float32x4_t y = b;
  float16x4_t x1 = vcvt_f16_f32(x);
  float16x4_t x2 = vcvt_f16_f32(y);
  return Vectorized<Half>(vcombine_f16(x1, x2));
}

template <typename Op>
Vectorized<MSharp::Half> binary_operator_via_float(
    Op op,
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
  const auto [a_float_low, a_float_high] = convert_half_float(a);
  const auto [b_float_low, b_float_high] = convert_half_float(b);
  return convert_float_half(
      op(a_float_low, b_float_low),
      op(a_float_high, b_float_high));
}

template <>
Vectorized<MSharp::Half> inline operator+(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vaddq_f16(a, b));
#else
  return binary_operator_via_float(std::plus<Vectorized<float>>(), a, b);
#endif
}

template <>
Vectorized<MSharp::Half> inline operator-(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vsubq_f16(a, b));
#else
  return binary_operator_via_float(std::minus<Vectorized<float>>(), a, b);
#endif
}

template <>
Vectorized<MSharp::Half> inline operator*(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vmulq_f16(a, b));
#else
  return binary_operator_via_float(std::multiplies<Vectorized<float>>(), a, b);
#endif
}

template <>
Vectorized<MSharp::Half> inline operator/(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vdivq_f16(a, b));
#else
  return binary_operator_via_float(std::divides<Vectorized<float>>(), a, b);
#endif
}

// frac. Implement this here so we can use subtraction
inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::frac() const {
  return *this - this->trunc();
}

// Implements the IEEE 754 201X `maximum` operation, which propagates NaN if
// either input is a NaN.
template <>
Vectorized<MSharp::Half> inline maximum(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vmaxq_f16(a, b));
#else
  return binary_operator_via_float(
      static_cast<Vectorized<float>(*)(const Vectorized<float>&, const Vectorized<float>&)>(&maximum),
      a,
      b);
#endif
}

// Implements the IEEE 754 201X `minimum` operation, which propagates NaN if
// either input is a NaN.
template <>
Vectorized<MSharp::Half> inline minimum(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vminq_f16(a, b));
#else
  return binary_operator_via_float(
      static_cast<Vectorized<float>(*)(const Vectorized<float>&, const Vectorized<float>&)>(&minimum),
      a,
      b);
#endif
}

template <>
Vectorized<MSharp::Half> inline clamp(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& min,
    const Vectorized<MSharp::Half>& max) {
  return minimum(max, maximum(min, a));
}

template <>
Vectorized<MSharp::Half> inline clamp_max(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& max) {
  return minimum(max, a);
}

template <>
Vectorized<MSharp::Half> inline clamp_min(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& min) {
  return maximum(min, a);
}

template <>
Vectorized<MSharp::Half> inline operator&(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
  return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vandq_u16(
      vreinterpretq_u16_f16(a), vreinterpretq_u16_f16(b))));
}

template <>
Vectorized<MSharp::Half> inline operator|(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
  return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(vorrq_u16(
      vreinterpretq_u16_f16(a), vreinterpretq_u16_f16(b))));
}

template <>
Vectorized<MSharp::Half> inline operator^(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b) {
  return Vectorized<MSharp::Half>(vreinterpretq_f16_u16(veorq_u16(
      vreinterpretq_u16_f16(a), vreinterpretq_u16_f16(b))));
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::eq(
    const Vectorized<MSharp::Half>& other) const {
  return (*this == other) & Vectorized<MSharp::Half>(1);
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::ne(
    const Vectorized<MSharp::Half>& other) const {
  return (*this != other) & Vectorized<MSharp::Half>(1);
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::gt(
    const Vectorized<MSharp::Half>& other) const {
  return (*this > other) & Vectorized<MSharp::Half>(1);
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::ge(
    const Vectorized<MSharp::Half>& other) const {
  return (*this >= other) & Vectorized<MSharp::Half>(1);
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::lt(
    const Vectorized<MSharp::Half>& other) const {
  return (*this < other) & Vectorized<MSharp::Half>(1);
}

inline Vectorized<MSharp::Half> Vectorized<MSharp::Half>::le(
    const Vectorized<MSharp::Half>& other) const {
  return (*this <= other) & Vectorized<MSharp::Half>(1);
}

// These are global functions, so the defaults in vec_base.h should
// work fine if __ARM_FEATURE_FP16_VECTOR_ARITHMETIC is not available.
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
template <>
inline void convert(const float16_t* src, int16_t* dst, int64_t n) {
  int64_t i;
#ifndef __msvc_cl__
#pragma unroll
#endif
  for (i = 0; i <= (n - Vectorized<MSharp::Half>::size());
       i += Vectorized<MSharp::Half>::size()) {
    vst1q_s16(dst + i, vcvtq_s16_f16(vld1q_f16(src + i)));
  }
#ifndef __msvc_cl__
#pragma unroll
#endif
  for (; i < n; i++) {
    dst[i] = static_cast<int16_t>(src[i]);
  }
}

template <>
inline void convert(const int16_t* src, float16_t* dst, int64_t n) {
  int64_t i;
#ifndef __msvc_cl__
#pragma unroll
#endif
  for (i = 0; i <= (n - Vectorized<MSharp::Half>::size());
       i += Vectorized<MSharp::Half>::size()) {
    vst1q_f16(dst + i, vcvtq_f16_s16(vld1q_s16(src + i)));
  }
#ifndef __msvc_cl__
#pragma unroll
#endif
  for (; i < n; i++) {
    dst[i] = static_cast<float16_t>(src[i]);
  }
}
#endif // __ARM_FEATURE_FP16_VECTOR_ARITHMETIC

template <>
Vectorized<MSharp::Half> inline fmadd(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b,
    const Vectorized<MSharp::Half>& c) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vfmaq_f16(c, a, b));
#else
  return a * b + c;
#endif
}

template <>
Vectorized<MSharp::Half> inline fmsub(
    const Vectorized<MSharp::Half>& a,
    const Vectorized<MSharp::Half>& b,
    const Vectorized<MSharp::Half>& c) {
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
  return Vectorized<MSharp::Half>(vfmsq_f16(c, a, b));
#else
  return a * b - c;
#endif
}
#endif // !defined(C10_MOBILE) && defined(__aarch64__)

} // namespace CPU_CAPABILITY
} // namespace pluss::vec
