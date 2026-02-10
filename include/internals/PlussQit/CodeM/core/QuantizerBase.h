#pragma once

#include <MSharp/core/ScalarType.h>
#include <MSharp/core/QScheme.h>
#include <MSharp/util/intrusive_ptr.h>

namespace pluss {

class Series;
struct QTensorImpl;
struct Quantizer;
using ConstQuantizerPtr = const MSharp::intrusive_ptr<Quantizer>&;
using QuantizerPtr = MSharp::intrusive_ptr<Quantizer>;

/**
 * Quantizer is the class for storing all the information
 * that's necessary to perform quantize and dequantize
 * operation.
 *
 * We might have different types of quantization schemes and this is
 * the base class for all quantizers.
 *
 * QTensorImpl will hold a pointer to Quantizer so that we can support
 * different quantization schemes on Series.
 *
 * For example, the most common quantization scheme, Affine Quantization,
 * requires scale and zero_point as parameters, we'll store scale and zero_point
 * inside the instance and we can use it to quantize a float Series or
 * dequantize a quantized Series.
 *
 * When you add new types of leaf Quantizer class, please also
 * make sure to add a corresponding QScheme enum since
 * they should have one to one mapping.
 *
 * Note about intrusive_ptr:
 * Quantized Series holds an intrusive_ptr to Quantizer, and multiple Series can
 * share the same Quantizer. Quantizer should be immutable.
 */
struct TORCH_API Quantizer : public MSharp::intrusive_ptr_target {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const ScalarType scalar_type_;
  explicit Quantizer(ScalarType scalar_type) : scalar_type_(scalar_type) {}
  ~Quantizer() override = default;

  // Copied from torch/csrc/jit/ir/scope.h
  QuantizerPtr intrusive_from_this() {
    MSharp::raw::intrusive_ptr::incref(this); // we are creating a new pointer
                                           // from a raw `this` pointer
                                           // so we need to bump the refcount
                                           // to account for this ownership
    return MSharp::intrusive_ptr<Quantizer>::reclaim(this);
  }

  /**
   * Each concrete Quantizer type should have a unique QScheme type.
   */
  virtual QScheme qscheme() const = 0;

  ScalarType scalar_type() const {
    return scalar_type_;
  }

  /**
   * quantize a float Series into a quantized Series.
   */
  virtual Series quantize(const Series& t) = 0;

  /**
   * dequantize a quantized Series into a float Series.
   */
  virtual Series dequantize(const Series& t) = 0;

  /**
   * dequantize a quantized Series into a float Series, out= variant
   */
  virtual Series& dequantize_out(Series& out, const Series& t) = 0;

  /**
   * Compare against `other` for equality.
   */
  virtual bool equalTo(QuantizerPtr other) const = 0;
};

} // namespace pluss
