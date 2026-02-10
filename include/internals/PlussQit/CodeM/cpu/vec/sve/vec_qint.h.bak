#pragma once

// DO NOT DEFINE STATIC DATA IN THIS HEADER!
// See Note [Do not compile initializers with SVE]

#include <PlussQit/cpu/vec/intrinsics.h>
#include <PlussQit/cpu/vec/vec_base.h>
#include <PlussQit/native/quantized/AffineQuantizerBase.h>
#include <MSharp/util/qint32.h>
#include <MSharp/util/qint8.h>
#include <MSharp/util/quint8.h>

#include <array>

// This file defines Vectorized<> for the quantized types.
//
//
// Currently, we simply use these classes as efficient converters between
// the quantized types and Vectorized<float>, usually in bandwidth-bound cases
// where doing the arithmetic in full-precision is acceptable (e.g.
// elementwise operators).
//
//
// Conversions are as follows:
//  Vectorized<qint8> -> 4x Vectorized<float>
//  Vectorized<quint8> -> 4x Vectorized<float>
//  Vectorized<qint32> -> 1x Vectorized<float>
//
// The size of the returned float vector is specified by the special
// constexpr function float_num_vecs. The type of the value returned
// from dequantize (and expected as an argument to quantize) is
// specified by float_vec_return_type.
//
// When writing kernels with these vectors, it is expected that floating-
// point operations will be carried out in a loop over Vectorized<T>::float_num_vecs
// iterations.


namespace pluss::vec {
// Note [CPU_CAPABILITY namespace]
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This header, and all of its subheaders, will be compiled with
// different architecture flags for each supported set of vector
// intrinsics. So we need to make sure they aren't inadvertently
// linked together. We do this by declaring objects in an `inline
// namespace` which changes the name mangling, but can still be
// accessed as `pluss::vec`.
inline namespace CPU_CAPABILITY {

#if defined(CPU_CAPABILITY_SVE)

// NOTE: These are low-performance implementations that we fall back on
// if we are not building with SVE. This may not be an issue, because
// currently for quantization we assume the user has at least SVE
// installed, so these can simply act as a reference implementation.
//
// If in the future we relax this requirement (SVE+), we should probably
// revisit these implementations

template <
    typename T,
    typename float_vec_return_type_,
    typename int_vec_return_type_,
    int size_>
struct VectorizedQuantizedConverter {
  using size_type = int;
  static constexpr size_type size() {
    return size_;
  }

  static constexpr int float_num_vecs() {
    return size() / Vectorized<float>::size();
  }

  static constexpr int int_num_vecs() {
    return size() / Vectorized<int32_t>::size();
  }

  using float_vec_return_type = float_vec_return_type_;
  using int_vec_return_type = int_vec_return_type_;

  using value_type = typename T::underlying;
  std::array<value_type, size_> vals;

  VectorizedQuantizedConverter(T val) {
    for (size_t i = 0; i < size(); ++i) {
      vals[i] = val.val_;
    }
  }

  VectorizedQuantizedConverter(const void* ptr) {
    memcpy(vals.data(), ptr, sizeof(value_type) * size());
  }

  void store(void* ptr, int count = size()) const {
    memcpy(ptr, vals.data(), count * sizeof(value_type));
  }

  float_vec_return_type dequantize(
      Vectorized<float> scale,
      Vectorized<float> zero_point,
      Vectorized<float> scale_zp_premul) const {
    float_vec_return_type rv;
    float tmp_scale[Vectorized<float>::size()];
    float tmp_zero_point[Vectorized<float>::size()];
    scale.store(tmp_scale);
    zero_point.store(tmp_zero_point);
    for (int i = 0; i < float_num_vecs(); ++i) {
      float tmp_vals[Vectorized<float>::size()];
      for (int j = 0; j < Vectorized<float>::size(); ++j) {
        tmp_vals[j] =
          pluss::native::dequantize_val<T>(tmp_scale[j], tmp_zero_point[j], T(vals[Vectorized<float>::size() * i + j]));
      }
      rv[i] = Vectorized<float>::loadu(tmp_vals);
    }
    return rv;
  }

  float_vec_return_type dequantize(
      Vectorized<float> scale,
      Vectorized<float> zero_point) const {
    float_vec_return_type rv;
    float tmp_scale[Vectorized<float>::size()];
    float tmp_zero_point[Vectorized<float>::size()];
    scale.store(tmp_scale);
    zero_point.store(tmp_zero_point);
    for (int i = 0; i < float_num_vecs(); ++i) {
      float tmp_vals[Vectorized<float>::size()];
      for (int j = 0; j < Vectorized<float>::size(); ++j) {
        tmp_vals[j] =
          pluss::native::dequantize_val<T>(tmp_scale[j], tmp_zero_point[j], T(vals[Vectorized<float>::size() * i + j]));
      }
      rv[i] = Vectorized<float>::loadu(tmp_vals);
    }
    return rv;
  }

 protected:
  VectorizedQuantizedConverter() {}
};

template <>
struct Vectorized<MSharp::qint32> : public VectorizedQuantizedConverter<
                                 MSharp::qint32,
                                 std::array<Vectorized<float>, 1>,
                                 std::array<Vectorized<MSharp::qint32>, 1>,
                                 VECTOR_WIDTH / 4> {
  Vectorized()
      : VectorizedQuantizedConverter<
            MSharp::qint32,
            std::array<Vectorized<float>, 1>,
            std::array<Vectorized<MSharp::qint32>, 1>,
            VECTOR_WIDTH / 4>() {}
  Vectorized(MSharp::qint32 val)
      : VectorizedQuantizedConverter<
            MSharp::qint32,
            std::array<Vectorized<float>, 1>,
            std::array<Vectorized<MSharp::qint32>, 1>,
            VECTOR_WIDTH / 4>(val) {}
  Vectorized(const void* ptr)
      : VectorizedQuantizedConverter<
            MSharp::qint32,
            std::array<Vectorized<float>, 1>,
            std::array<Vectorized<MSharp::qint32>, 1>,
            VECTOR_WIDTH / 4>(ptr) {}
#if 1
  static Vectorized<MSharp::qint32> loadu(const void* ptr) {
    return Vectorized<MSharp::qint32>(ptr);
  }

  static Vectorized<MSharp::qint32> loadu(const void* ptr, int64_t count) {
      __at_align__ value_type tmp_values[size()];
      // Ensure uninitialized memory does not change the output value See https://github.com/pytorch/pytorch/issues/32502
      // for more details. We do not initialize arrays to zero using "={0}" because gcc would compile it to two
      // instructions while a loop would be compiled to one instruction.
      for (const auto i : MSharp::irange(size())) {
        tmp_values[i] = 0;
      }
      std::memcpy(tmp_values, reinterpret_cast<const value_type*>(ptr), count * sizeof(value_type));
      return loadu(tmp_values);
  }
#else
  static Vectorized<MSharp::qint32> loadu(const void* ptr, int64_t count = size()) {
    if (count == size())
      return svld1_s32(ptrue, reinterpret_cast<const int32_t*>(ptr));
    svbool_t pg = svwhilelt_b32(0ull, count);
    return svld1_s32(pg, reinterpret_cast<const int32_t*>(ptr));
  }
#endif
  static Vectorized<MSharp::qint32> quantize(
      const float_vec_return_type& rhs,
      float scale,
      int32_t zero_point,
      float inverse_scale) {
    std::array<value_type, size()> qvals;
    std::array<float, float_num_vecs() * Vectorized<float>::size()> float_vals;

    for (int i = 0; i < float_num_vecs(); ++i) {
      rhs[i].store(&float_vals[i * Vectorized<float>::size()], Vectorized<float>::size());
    }

    pluss::native::quantize_vec<MSharp::qint32, /*precision=*/32>(
        scale,
        zero_point,
        float_vals.data(),
        (MSharp::qint32*)qvals.data(),
        Vectorized<float>::size() * float_num_vecs());

    return Vectorized<MSharp::qint32>::loadu(qvals.data());
  }

  Vectorized<MSharp::qint32> maximum(Vectorized<MSharp::qint32> b) const {
    Vectorized<MSharp::qint32> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::max<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::qint32> minimum(Vectorized<MSharp::qint32> b) const {
    Vectorized<MSharp::qint32> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::qint32> relu(Vectorized<MSharp::qint32> zero_point) const  {
    return maximum(zero_point);
  }


  Vectorized<MSharp::qint32> relu6(
      Vectorized<MSharp::qint32> zero_point,
      Vectorized<MSharp::qint32> q_six) {
    Vectorized<MSharp::qint32> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(
          std::max<value_type>(vals[i], zero_point.vals[i]), q_six.vals[i]);
    }
    return retval;
  }

  int_vec_return_type widening_subtract(Vectorized<MSharp::qint32> b) const {
    int_vec_return_type retval;
    for (size_t i = 0; i < size(); ++i) {
      retval[0].vals[i] = vals[i] - b.vals[i];
    }
    return retval;
  }

  static Vectorized<MSharp::qint32> requantize_from_int(
      const int_vec_return_type& inp,
      float multiplier,
      int32_t zero_point) {
    Vectorized<MSharp::qint32> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] =
          nearbyint(static_cast<float>(inp[0].vals[i]) * multiplier) +
          zero_point;
    }
    return retval;
  }
};

template <>
Vectorized<MSharp::qint32> inline maximum(const Vectorized<MSharp::qint32>& a, const Vectorized<MSharp::qint32>& b) {
  return a.maximum(b);
}

template <>
Vectorized<MSharp::qint32> inline operator*(
    const Vectorized<MSharp::qint32>& a,
    const Vectorized<MSharp::qint32>& b) {
  Vectorized<MSharp::qint32> retval;
  for (size_t i = 0; i < std::decay_t<decltype(a)>::size(); ++i) {
    retval.vals[i] = a.vals[i] * b.vals[i];
  }
  return retval;
}

template <>
Vectorized<MSharp::qint32> inline operator+(
    const Vectorized<MSharp::qint32>& a,
    const Vectorized<MSharp::qint32>& b) {
  Vectorized<MSharp::qint32> retval;
  for (size_t i = 0; i < std::decay_t<decltype(a)>::size(); ++i) {
    retval.vals[i] = a.vals[i] + b.vals[i];
  }
  return retval;
}

template <>
struct Vectorized<MSharp::qint8> : public VectorizedQuantizedConverter<
                                MSharp::qint8,
                                std::array<Vectorized<float>, 4>,
                                std::array<Vectorized<MSharp::qint32>, 4>,
                                VECTOR_WIDTH> {
  Vectorized()
      : VectorizedQuantizedConverter<
            MSharp::qint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>() {}
  Vectorized(MSharp::qint8 val)
      : VectorizedQuantizedConverter<
            MSharp::qint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>(val) {}
  Vectorized(const void* ptr)
      : VectorizedQuantizedConverter<
            MSharp::qint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>(ptr) {}

  static Vectorized<MSharp::qint8> loadu(const void* ptr) {
    return Vectorized<MSharp::qint8>(ptr);
  }

  static Vectorized<MSharp::qint8> loadu(const void* ptr, int64_t count) {
      __at_align__ value_type tmp_values[size()];
      // Ensure uninitialized memory does not change the output value See https://github.com/pytorch/pytorch/issues/32502
      // for more details. We do not initialize arrays to zero using "={0}" because gcc would compile it to two
      // instructions while a loop would be compiled to one instruction.
      for (const auto i : MSharp::irange(size())) {
        tmp_values[i] = 0;
      }
      std::memcpy(tmp_values, reinterpret_cast<const value_type*>(ptr), count * sizeof(value_type));
      return loadu(tmp_values);
  }

  static Vectorized<MSharp::qint8> quantize(
      const float_vec_return_type& rhs,
      float scale,
      int32_t zero_point,
      float inverse_scale) {
    std::array<value_type, size()> qvals;
    std::array<float, float_num_vecs() * Vectorized<float>::size()> float_vals;

    for (int i = 0; i < float_num_vecs(); ++i) {
      rhs[i].store(&float_vals[i * Vectorized<float>::size()], Vectorized<float>::size());
    }

    pluss::native::quantize_vec<MSharp::qint8>(
        scale,
        zero_point,
        float_vals.data(),
        (MSharp::qint8*)qvals.data(),
        Vectorized<float>::size() * float_num_vecs());

    return Vectorized<MSharp::qint8>::loadu(qvals.data());
  }

  Vectorized<MSharp::qint8> maximum(Vectorized<MSharp::qint8> b) const {
    Vectorized<MSharp::qint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::max<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::qint8> minimum(Vectorized<MSharp::qint8> b) const {
    Vectorized<MSharp::qint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::qint8> relu(Vectorized<MSharp::qint8> zero_point) const {
    return maximum(zero_point);
  }

  Vectorized<MSharp::qint8> relu6(
      Vectorized<MSharp::qint8> zero_point,
      Vectorized<MSharp::qint8> q_six) {
    Vectorized<MSharp::qint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(
          std::max<value_type>(vals[i], zero_point.vals[i]), q_six.vals[i]);
    }
    return retval;
  }

  int_vec_return_type widening_subtract(Vectorized<MSharp::qint8> b) const {
    int_vec_return_type retval;
    constexpr int elem_per_int_vec = size() / int_num_vecs();
    for (size_t i = 0; i < int_num_vecs(); ++i) {
      for (size_t j = 0; j < elem_per_int_vec; ++j) {
        retval[i].vals[j] =
            static_cast<int32_t>(vals[i * elem_per_int_vec + j]) -
            static_cast<int32_t>(b.vals[i * elem_per_int_vec + j]);
      }
    }
    return retval;
  }
  static Vectorized<MSharp::qint8> requantize_from_int(
      const int_vec_return_type& inp,
      float multiplier,
      int32_t zero_point) {
    constexpr int elem_per_int_vec = size() / int_num_vecs();
    constexpr auto min_val = std::numeric_limits<value_type>::min();
    constexpr auto max_val = std::numeric_limits<value_type>::max();
    Vectorized<MSharp::qint8> retval;
    for (size_t i = 0; i < int_num_vecs(); ++i) {
      for (size_t j = 0; j < elem_per_int_vec; ++j) {
        int32_t rounded =
            nearbyint(static_cast<float>(inp[i].vals[j]) * multiplier) +
            zero_point;
        retval.vals[i * elem_per_int_vec + j] =
            std::min<int32_t>(std::max<int32_t>(rounded, min_val), max_val);
      }
    }
    return retval;
  }
};

template <>
Vectorized<MSharp::qint8> inline maximum(const Vectorized<MSharp::qint8>& a, const Vectorized<MSharp::qint8>& b) {
  return a.maximum(b);
}

template <>
struct Vectorized<MSharp::quint8> : public VectorizedQuantizedConverter<
                                 MSharp::quint8,
                                 std::array<Vectorized<float>, 4>,
                                 std::array<Vectorized<MSharp::qint32>, 4>,
                                 VECTOR_WIDTH> {
  Vectorized()
      : VectorizedQuantizedConverter<
            MSharp::quint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>() {}
  Vectorized(MSharp::quint8 val)
      : VectorizedQuantizedConverter<
            MSharp::quint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>(val) {}
  Vectorized(const void* ptr)
      : VectorizedQuantizedConverter<
            MSharp::quint8,
            std::array<Vectorized<float>, 4>,
            std::array<Vectorized<MSharp::qint32>, 4>,
            VECTOR_WIDTH>(ptr) {}
#if 1
  static Vectorized<MSharp::quint8> loadu(const void* ptr) {
    return Vectorized<MSharp::quint8>(ptr);
  }

  static Vectorized<MSharp::quint8> loadu(const void* ptr, int64_t count) {
      __at_align__ value_type tmp_values[size()];
      // Ensure uninitialized memory does not change the output value See https://github.com/pytorch/pytorch/issues/32502
      // for more details. We do not initialize arrays to zero using "={0}" because gcc would compile it to two
      // instructions while a loop would be compiled to one instruction.
      for (const auto i : MSharp::irange(size())) {
        tmp_values[i] = 0;
      }
      std::memcpy(tmp_values, reinterpret_cast<const value_type*>(ptr), count * sizeof(value_type));
      return loadu(tmp_values);
  }
#else
  static Vectorized<MSharp::quint8> loadu(const void* ptr, int64_t count = size()) {
    if (count == size())
      return svld1_u8(ptrue, reinterpret_cast<const uint8_t*>(ptr));
    svbool_t pg = svwhilelt_b8(0ull, count);
    return svld1_u8(pg, reinterpret_cast<const uint8_t*>(ptr));
  }
#endif
  static Vectorized<MSharp::quint8> quantize(
      const float_vec_return_type& rhs,
      float scale,
      int32_t zero_point,
      float inverse_scale) {
    std::array<value_type, size()> qvals;
    std::array<float, float_num_vecs() * Vectorized<float>::size()> float_vals;

    for (int i = 0; i < float_num_vecs(); ++i) {
      rhs[i].store(&float_vals[i * Vectorized<float>::size()], Vectorized<float>::size());
    }

    pluss::native::quantize_vec<MSharp::quint8>(
        scale,
        zero_point,
        float_vals.data(),
        (MSharp::quint8*)qvals.data(),
        Vectorized<float>::size() * float_num_vecs());

    return Vectorized<MSharp::quint8>::loadu(qvals.data());
  }

  Vectorized<MSharp::quint8> maximum(Vectorized<MSharp::quint8> b) const {
    Vectorized<MSharp::quint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::max<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::quint8> minimum(Vectorized<MSharp::quint8> b) const {
    Vectorized<MSharp::quint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(vals[i], b.vals[i]);
    }
    return retval;
  }

  Vectorized<MSharp::quint8> relu(Vectorized<MSharp::quint8> zero_point) const {
    return maximum(zero_point);
  }


  Vectorized<MSharp::quint8> relu6(
      Vectorized<MSharp::quint8> zero_point,
      Vectorized<MSharp::quint8> q_six) {
    Vectorized<MSharp::quint8> retval;
    for (size_t i = 0; i < size(); ++i) {
      retval.vals[i] = std::min<value_type>(
          std::max<value_type>(vals[i], zero_point.vals[i]), q_six.vals[i]);
    }
    return retval;
  }

  int_vec_return_type widening_subtract(Vectorized<MSharp::quint8> b) const {
    int_vec_return_type retval;
    constexpr int elem_per_int_vec = size() / int_num_vecs();
    for (size_t i = 0; i < int_num_vecs(); ++i) {
      for (size_t j = 0; j < elem_per_int_vec; ++j) {
        retval[i].vals[j] =
            static_cast<int32_t>(vals[i * elem_per_int_vec + j]) -
            static_cast<int32_t>(b.vals[i * elem_per_int_vec + j]);
      }
    }
    return retval;
  }
  static Vectorized<MSharp::quint8> requantize_from_int(
      const int_vec_return_type& inp,
      float multiplier,
      int32_t zero_point) {
    constexpr int elem_per_int_vec = size() / int_num_vecs();
    constexpr auto min_val = std::numeric_limits<value_type>::min();
    constexpr auto max_val = std::numeric_limits<value_type>::max();
    Vectorized<MSharp::quint8> retval;
    for (size_t i = 0; i < int_num_vecs(); ++i) {
      for (size_t j = 0; j < elem_per_int_vec; ++j) {
        int32_t rounded =
            nearbyint(static_cast<float>(inp[i].vals[j]) * multiplier) +
            zero_point;
        retval.vals[i * elem_per_int_vec + j] =
            std::min<int32_t>(std::max<int32_t>(rounded, min_val), max_val);
      }
    }
    return retval;
  }
};

template <>
Vectorized<MSharp::quint8> inline maximum(const Vectorized<MSharp::quint8>& a, const Vectorized<MSharp::quint8>& b) {
  return a.maximum(b);
}

#endif // defined(CPU_CAPABILITY_SVE)

}}
