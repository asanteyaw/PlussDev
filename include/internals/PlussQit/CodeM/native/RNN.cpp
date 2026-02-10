#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/RNN.h>

#include <PlussQit/core/Series.h>
#include <PlussQit/core/List.h>
#include <PlussQit/Context.h>
#include <PlussQit/TensorOperators.h>
#include <PlussQit/mps/MPSDevice.h>
#include <PlussQit/native/quantized/PackedParams.h>
#include <PlussQit/native/quantized/library.h>
#include <PlussQit/native/quantized/cpu/fbgemm_utils.h>
#include <PlussQit/native/quantized/cpu/QnnpackUtils.h>
#include <MSharp/core/GradMode.h>
#include <MSharp/macros/Macros.h>
#include <MSharp/util/irange.h>
#include <torch/custom_class.h>
#include <torch/library.h>
#include <PlussQit/Config.h>
#if AT_MKLDNN_ENABLED()
#include <PlussQit/native/mkldnn/Utils.h>
#endif

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_lstm_mps.h>
#include <PlussQit/ops/_thnn_differentiable_gru_cell_backward_native.h>
#include <PlussQit/ops/_thnn_differentiable_lstm_cell_backward_native.h>
#include <PlussQit/ops/_thnn_fused_gru_cell.h>
#include <PlussQit/ops/_thnn_fused_lstm_cell.h>
#include <PlussQit/ops/_thnn_fused_lstm_cell_backward.h>
#include <PlussQit/ops/_thnn_fused_lstm_cell_backward_impl.h>
#include <PlussQit/ops/_thnn_fused_lstm_cell_backward_native.h>
#include <PlussQit/ops/_use_cudnn_rnn_flatten_weight_native.h>
#include <PlussQit/ops/cat.h>
#include <PlussQit/ops/cudnn_is_acceptable.h>
#include <PlussQit/ops/dropout.h>
#include <PlussQit/ops/fbgemm_linear_int8_weight_fp32_activation.h>
#include <PlussQit/ops/fbgemm_linear_quantize_weight_native.h>
#include <PlussQit/ops/fbgemm_pack_quantized_matrix_native.h>
#include <PlussQit/ops/gru_cell_native.h>
#include <PlussQit/ops/gru_native.h>
#include <PlussQit/ops/linear.h>
#include <PlussQit/ops/lstm_cell_native.h>
#include <PlussQit/ops/lstm_native.h>
#include <PlussQit/ops/matmul.h>
#include <PlussQit/ops/quantized_gru_cell_native.h>
#include <PlussQit/ops/quantized_lstm_cell_native.h>
#include <PlussQit/ops/quantized_rnn_relu_cell_native.h>
#include <PlussQit/ops/quantized_rnn_tanh_cell_native.h>
#include <PlussQit/ops/relu.h>
#include <PlussQit/ops/rnn_relu_cell_native.h>
#include <PlussQit/ops/rnn_relu_native.h>
#include <PlussQit/ops/rnn_tanh_cell_native.h>
#include <PlussQit/ops/rnn_tanh_native.h>
#include <PlussQit/ops/sigmoid_backward.h>
#include <PlussQit/ops/stack.h>
#include <PlussQit/ops/tanh.h>
#include <PlussQit/ops/tanh_backward.h>
#include <PlussQit/ops/zeros_like.h>
#include <PlussQit/ops/zeros_like_ops.h>
#include <utility>
#endif

namespace pluss::native {

namespace {

// Check if pytorch is compiled with MIOpen.
bool use_miopen(const pluss::Series& input, const double dropout_state) {
    bool is_miopen_acceptable = ((input.scalar_type() == pluss::kFloat)|| (input.scalar_type() == pluss::kHalf)) &&
                                (detail::getCUDAHooks().compiledWithMIOpen()) &&
                                (input.is_cuda()) &&
                                (pluss::globalContext().userEnabledCuDNN());
    // MIOpen functions returns miopenStatusBadParm on empty
    // tensors. Maybe some functions actually support empty tensors, but
    // native kernels shouldn't be much slower because the output is also
    // likely empty.
    if (input.sym_numel() == 0) return false;

    return is_miopen_acceptable;
}

bool use_mkldnn(const Series& input, TensorList params, TensorList hx) {
#if AT_MKLDNN_ENABLED()
  if (!pluss::globalContext().userEnabledMkldnn()) {
    return false;
  }
  auto is_cpu_backend = [&](const TensorList tensors) {
    bool backend_cpu = true;
    for (const auto& t : tensors) {
      if (!(t.options().backend() == pluss::Backend::CPU)) {
        backend_cpu = false;
        break;
      }
    }
    return backend_cpu;
  };
  return input.options().backend() == pluss::Backend::CPU &&
      is_cpu_backend(params) && is_cpu_backend(hx) &&
      (input.scalar_type() == kFloat ||
       (input.scalar_type() == kBFloat16 && mkldnn_bf16_device_check()) ||
       (input.scalar_type() == kHalf && !pluss::GradMode::is_enabled() &&
        mkldnn_fp16_device_check())) &&
      input.numel() != 0;
#endif
  return false;
}

template<typename T>
using pair_of = std::pair<T, T>;

template<typename T>
using tpair_of = std::tuple<T, T>;

// Those could have been function pointers, but MSVC chokes on function pointers as template parameters
struct tanh_f {
  Series operator()(const Series& t) const { return pluss::tanh(t); }
};

struct relu_f {
  Series operator()(const Series& t) const { return pluss::relu(t); }
};

struct PackedSequence {
  PackedSequence() = default;
  PackedSequence(Series _data, Series _batch_sizes)
    : data(std::move(_data)), batch_sizes(std::move(_batch_sizes)) {}

  Series data;
  Series batch_sizes;
};

// Simple type for __getstate__/__setstate__ serialization
//
// Element 0 is a string key to say what kind of CellParam this is. It
// should be a valid key into cell_params_deserializers
// Element 1 is the Tensors contained within the CellParams instance
// Element 2 is the doubles (if any) contained in the CellParams instance
// Element 3 is the longs (if any) contained within the CellParams instance
using CellParamsSerializationType = std::tuple<
    std::string,
    std::vector<pluss::Series>,
    std::vector<double>,
    std::vector<int64_t>,
    std::vector<MSharp::intrusive_ptr<LinearPackedParamsBase>>>;

// Base class so we can polymorphically handle these
struct CellParamsBase : torch::CustomClassHolder {
  virtual Series matmul_ih(const Series& input) const = 0;
  virtual Series matmul_hh(const Series& h) const = 0;
  // by default doing nothing. CellParams will override this
  // to define correct behavior for LSTMs with projections.
  // This function is not pure virtual, because it's useful to
  // provide this default implementation, so that all cell params
  // that don't support projections work correctly (e.g. QuantizedCellParams variations)
  virtual Series matmul_hr(const Series& h) const {
    return h;
  }
  virtual Series linear_ih(const Series& input_ih) const = 0;
  virtual Series linear_hh(const Series& input_hh) const = 0;

  virtual const Series& b_ih() const = 0;
  virtual const Series& b_hh() const = 0;

  virtual CellParamsSerializationType __getstate__() const = 0;
};

// Pretty much all cells we support take the same set of arguments, but threading those
// 4 arguments manually is really annoying. Their lifetime is externally managed, so we only
// pass this struct of references around. LSTMs with projections have 5th argument w_hr, for all
// other models it's always going to be undefined.
struct CellParams : public CellParamsBase {
  CellParams(
      const Series& _w_ih,
      const Series& _w_hh,
      const Series& _b_ih,
      const Series& _b_hh,
      const Series& _w_hr)
      : w_ih(_w_ih), w_hh(_w_hh), b_ih_(_b_ih), b_hh_(_b_hh), w_hr(_w_hr) {}

  const Series& w_ih;
  const Series& w_hh;
  const Series& b_ih_; /* optional */
  const Series& b_hh_; /* optional */
  const Series& w_hr;  /* only defined for LSTMs with projections */

  Series matmul_ih(const Series& input) const override {
    return pluss::matmul(input, w_ih.t());
  }
  Series matmul_hh(const Series& h) const override {
    return pluss::matmul(h, w_hh.t());
  }
  Series matmul_hr(const Series& h) const override {
    if (w_hr.defined()) {
      return pluss::matmul(h, w_hr.t());
    }
    return h;
  }
  Series linear_ih(const Series& input) const override {
    return pluss::linear(input, w_ih, b_ih_);
  }
  Series linear_hh(const Series& h) const override {
    return pluss::linear(h, w_hh, b_hh_);
  }
  const Series& b_ih() const override {
    return b_ih_;
  }
  const Series& b_hh() const override {
    return b_hh_;
  }
  CellParamsSerializationType __getstate__() const override {
    TORCH_INTERNAL_ASSERT(false, "Not yet implemented");
  }
  static MSharp::intrusive_ptr<CellParamsBase> __setstate__(
      const CellParamsSerializationType& state) {
    TORCH_INTERNAL_ASSERT(false, "Not yet implemented");
  }
};

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params(
    const pluss::Series& w_ih,
    const pluss::Series& w_hh,
    pluss::Series bias_ih,
    pluss::Series bias_hh);

struct QuantizedCellParams : public CellParamsBase {
  QuantizedCellParams(
      Series _w_ih,
      Series _w_hh,
      Series _b_ih,
      Series _b_hh,
      Series _packed_ih,
      Series _packed_hh,
      Series _col_offsets_ih,
      Series _col_offsets_hh,
      Scalar _scale_ih,
      Scalar _scale_hh,
      Scalar _zero_point_ih,
      Scalar _zero_point_hh)
      : w_ih(std::move(_w_ih)),
        w_hh(std::move(_w_hh)),
        b_ih_(std::move(_b_ih)),
        b_hh_(std::move(_b_hh)),
        packed_ih(std::move(_packed_ih)),
        packed_hh(std::move(_packed_hh)),
        col_offsets_ih(std::move(_col_offsets_ih)),
        col_offsets_hh(std::move(_col_offsets_hh)),
        scale_ih(std::move(_scale_ih)),
        scale_hh(std::move(_scale_hh)),
        zero_point_ih(std::move(_zero_point_ih)),
        zero_point_hh(std::move(_zero_point_hh)) {}

  const Series w_ih;
  const Series w_hh;
  const Series b_ih_;
  const Series b_hh_;
  const Series packed_ih;
  const Series packed_hh;
  const Series col_offsets_ih;
  const Series col_offsets_hh;
  const Scalar scale_ih;
  const Scalar scale_hh;
  const Scalar zero_point_ih;
  const Scalar zero_point_hh;

  Series matmul_ih(const Series& input) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }
  Series matmul_hh(const Series& h) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }
  Series linear_ih(const Series& input) const override {
    return pluss::fbgemm_linear_int8_weight_fp32_activation(
        input, w_ih, packed_ih, col_offsets_ih, scale_ih, zero_point_ih, b_ih_);
  }
  Series linear_hh(const Series& h) const override {
    return pluss::fbgemm_linear_int8_weight_fp32_activation(
        h, w_hh, packed_hh, col_offsets_hh, scale_hh, zero_point_hh, b_hh_);
  }
  const Series& b_ih() const override {
    return b_ih_;
  }
  const Series& b_hh() const override {
    return b_hh_;
  }
  CellParamsSerializationType __getstate__() const override {
    std::vector<pluss::Series> tensors_to_serialize = {
        w_ih, w_hh, b_ih_, b_hh_, col_offsets_ih, col_offsets_hh};
    std::vector<double> doubles_to_serialize = {scale_ih.toDouble(),
                                                scale_hh.toDouble()};
    std::vector<int64_t> longs_to_serialize = {zero_point_ih.toLong(),
                                               zero_point_hh.toLong()};
    return CellParamsSerializationType(
        "quantized",
        tensors_to_serialize,
        doubles_to_serialize,
        longs_to_serialize,
        {});
  }
  static MSharp::intrusive_ptr<CellParamsBase> __setstate__(
      CellParamsSerializationType state) {
    auto [_, tensors, doubles, longs, __] =
        std::move(state);
    TORCH_INTERNAL_ASSERT(tensors.size() == 6);
    TORCH_INTERNAL_ASSERT(doubles.size() == 2);
    TORCH_INTERNAL_ASSERT(longs.size() == 2);

    pluss::Series qw_ih = std::move(tensors[0]), qw_hh = std::move(tensors[1]),
               b_ih = std::move(tensors[2]), b_hh = std::move(tensors[3]),
               col_offsets_ih = std::move(tensors[4]),
               col_offsets_hh = std::move(tensors[5]);
    double scale_ih = doubles[0], scale_hh = doubles[1];
    int64_t zero_point_ih = longs[0], zero_point_hh = longs[1];

    pluss::Series packed_ih = pluss::native::fbgemm_pack_quantized_matrix(qw_ih);
    pluss::Series packed_hh = pluss::native::fbgemm_pack_quantized_matrix(qw_hh);

    return MSharp::make_intrusive<QuantizedCellParams>(
        /*w_ih=*/std::move(qw_ih),
        /*w_hh=*/std::move(qw_hh),
        /*b_ih_=*/std::move(b_ih),
        /*b_hh_=*/std::move(b_hh),
        /*packed_ih=*/std::move(packed_ih),
        /*packed_hh=*/std::move(packed_hh),
        /*col_offsets_ih=*/std::move(col_offsets_ih),
        /*col_offsets_hh=*/std::move(col_offsets_hh),
        /*scale_ih=*/scale_ih,
        /*scale_hh=*/scale_hh,
        /*zero_point_ih=*/zero_point_ih,
        /*zero_point_hh=*/zero_point_hh);
  }
};

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params(
    const pluss::Series& w_ih,
    const pluss::Series& w_hh,
    pluss::Series b_ih,
    pluss::Series b_hh) {
  auto make_vals = [&](const pluss::Series& W) {
    auto params = pluss::native::fbgemm_linear_quantize_weight(W);
    pluss::Series packed_weight =
        pluss::native::fbgemm_pack_quantized_matrix(std::get<0>(params));
    return std::tuple_cat(
        std::make_tuple(std::move(packed_weight)), std::move(params));
  };

  auto [packed_ih, qw_ih, col_offsets_ih, scale_ih, zero_point_ih] =
      make_vals(w_ih);
  auto [packed_hh, qw_hh, col_offsets_hh, scale_hh, zero_point_hh] =
      make_vals(w_hh);

  return MSharp::make_intrusive<QuantizedCellParams>(
      /*qw_ih=*/std::move(qw_ih),
      /*qw_hh=*/std::move(qw_hh),
      /*b_ih=*/std::move(b_ih),
      /*b_hh=*/std::move(b_hh),
      /*packed_ih=*/std::move(packed_ih),
      /*packed_hh=*/std::move(packed_hh),
      /*col_offsets_ih=*/std::move(col_offsets_ih),
      /*col_offsets_hh=*/std::move(col_offsets_hh),
      /*scale_ih=*/scale_ih,
      /*scale_hh=*/scale_hh,
      /*zero_point_ih=*/zero_point_ih,
      /*zero_point_hh=*/zero_point_hh);
}

// QuantizedCellParams vs. QuantizedCellParamsDynamic
//
// QuantizedCellParams uses the legacy
// fbgemm_linear_int8_weight_fp32_activation API, which requires the explicit
// scale and zero point parameters for the weight. QuantizedCellParamsDynamic
// uses the new fbgemm_linear_dynamic API, which doesn't require the explicit
// scale and zero point parameters. These quantization parameters are
// encapsulated in the `PackedLinearWeight` struct in
// aten/src/PlussQit/native/quantized/cpu/fbgemm_utils.h.

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params_dynamic(
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_ih_packed,
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_hh_packed,
    pluss::Series bias_ih,
    pluss::Series bias_hh,
    bool reduce_range);

struct QuantizedCellParamsDynamic : public CellParamsBase {
  QuantizedCellParamsDynamic(
      MSharp::intrusive_ptr<LinearPackedParamsBase>
          _packed_w_ih, /* Prepacked Weight Series */
      MSharp::intrusive_ptr<LinearPackedParamsBase>
          _packed_w_hh, /* Prepacked Weight Series */
      Series _b_ih, /* float Bias Series */
      Series _b_hh, /* float Bias Series */
      bool _reduce_range = false /* Use reduced range for activation tensors */)
      : packed_w_ih(std::move(_packed_w_ih)),
        packed_w_hh(std::move(_packed_w_hh)),
        b_ih_(std::move(_b_ih)),
        b_hh_(std::move(_b_hh)),
        reduce_range_(_reduce_range) {}

  MSharp::intrusive_ptr<LinearPackedParamsBase> packed_w_ih;
  MSharp::intrusive_ptr<LinearPackedParamsBase> packed_w_hh;
  const Series b_ih_;
  const Series b_hh_;
  bool reduce_range_;

  Series matmul_ih(const Series& input) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }
  Series matmul_hh(const Series& h) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }

  Series linear_ih(const Series& input_ih) const override {
    return packed_w_ih->apply_dynamic(input_ih, reduce_range_);
  }
  Series linear_hh(const Series& input_hh) const override {
    return packed_w_hh->apply_dynamic(input_hh, reduce_range_);
  }

  const Series& b_ih() const override {
    return b_ih_;
  }
  const Series& b_hh() const override {
    return b_hh_;
  }
  CellParamsSerializationType __getstate__() const override {
    std::vector<pluss::Series> tensors_to_serialize{
        /*b_ih=*/b_ih_,
        /*b_hh=*/b_hh_,
    };

    std::vector<MSharp::intrusive_ptr<LinearPackedParamsBase>>
        packed_params_to_serialize{packed_w_ih, packed_w_hh};

    // reduce_range parameter is serialized along with the int field values.
    return CellParamsSerializationType(
        "quantized_dynamic",
        tensors_to_serialize,
        {},
        {reduce_range_},
        packed_params_to_serialize);
  }
  static MSharp::intrusive_ptr<CellParamsBase> __setstate__(
      CellParamsSerializationType state) {
    auto [_, tensors, __, serialized_ints, packed_params] =
        std::move(state);
    TORCH_INTERNAL_ASSERT(tensors.size() == 2);
    TORCH_INTERNAL_ASSERT(packed_params.size() == 2);

    bool reduce_range = serialized_ints.empty() ? false : serialized_ints[0];
    return make_quantized_cell_params_dynamic(
        /*w_ih_packed=*/std::move(packed_params[0]),
        /*w_hh_packed=*/std::move(packed_params[1]),
        /*bias_ih=*/std::move(tensors[0]),
        /*bias_hh=*/std::move(tensors[1]),
        /*reduce_range=*/reduce_range);
  }
};

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params_dynamic(
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_ih_packed,
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_hh_packed,
    pluss::Series bias_ih,
    pluss::Series bias_hh,
    bool reduce_range) {

  return MSharp::make_intrusive<QuantizedCellParamsDynamic>(
      /*_packed_w_ih=*/std::move(w_ih_packed),
      /*_packed_w_hh=*/std::move(w_hh_packed),
      /*_b_ih=*/std::move(bias_ih),
      /*_b_hh=*/std::move(bias_hh),
      /*_reduce_range=*/reduce_range);
}

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params_fp16(
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_ih_packed,
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_hh_packed);

struct QuantizedCellParamsFP16 : public CellParamsBase {
  QuantizedCellParamsFP16(
      MSharp::intrusive_ptr<LinearPackedParamsBase> _packed_ih,
      MSharp::intrusive_ptr<LinearPackedParamsBase> _packed_hh)
      : packed_ih(std::move(_packed_ih)), packed_hh(std::move(_packed_hh)) {}

  MSharp::intrusive_ptr<LinearPackedParamsBase> packed_ih;
  MSharp::intrusive_ptr<LinearPackedParamsBase> packed_hh;
  const Series b_ih_;
  const Series b_hh_;

  Series matmul_ih(const Series& /* unused */) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }
  Series matmul_hh(const Series& /* unused */) const override {
    TORCH_CHECK(false, "matmul is not supported with quantized cell params");
  }
  Series linear_ih(const Series& input) const override {
    return packed_ih->apply_dynamic(input);
  }
  Series linear_hh(const Series& h) const override {
    return packed_hh->apply_dynamic(h);
  }

  const Series& b_ih() const override {
    return b_ih_;
  }
  const Series& b_hh() const override {
    return b_hh_;
  }
  CellParamsSerializationType __getstate__() const override {
    std::vector<MSharp::intrusive_ptr<LinearPackedParamsBase>>
        packed_params_to_serialize{packed_ih, packed_hh};

    return CellParamsSerializationType(
        "quantized_fp16", {}, {}, {}, packed_params_to_serialize);
  }
  static MSharp::intrusive_ptr<CellParamsBase> __setstate__(
      CellParamsSerializationType state) {
    auto packed_params = std::get<4>(std::move(state));
    TORCH_INTERNAL_ASSERT(packed_params.size() == 2);
    return make_quantized_cell_params_fp16(
        /*w_ih_packed=*/std::move(packed_params[0]),
        /*w_hh_packed=*/std::move(packed_params[1]));
  }
};

MSharp::intrusive_ptr<CellParamsBase> make_quantized_cell_params_fp16(
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_ih_packed,
    MSharp::intrusive_ptr<LinearPackedParamsBase> w_hh_packed) {
  return MSharp::make_intrusive<QuantizedCellParamsFP16>(
      std::move(w_ih_packed), std::move(w_hh_packed));
}

static std::unordered_map<
    std::string,
    MSharp::intrusive_ptr<CellParamsBase> (*)(CellParamsSerializationType)>
    cell_params_deserializers = {
        {"quantized", &QuantizedCellParams::__setstate__},
        {"quantized_dynamic", &QuantizedCellParamsDynamic::__setstate__},
        {"quantized_fp16", &QuantizedCellParamsFP16::__setstate__}};

// Stupid wrapper to convert from -> to .
struct QRNNCellParamsWrapper {
  QRNNCellParamsWrapper(MSharp::intrusive_ptr<CellParamsBase> param)
      : param_(std::move(param)) {}

  Series matmul_ih(const Series& input) const {
    return param_->matmul_ih(input);
  }
  Series matmul_hh(const Series& h) const {
    return param_->matmul_hh(h);
  }
  Series matmul_hr(const Series& h) const {
    return param_->matmul_hr(h);
  }
  Series linear_ih(const Series& input) const {
    return param_->linear_ih(input);
  }
  Series linear_hh(const Series& h) const {
    return param_->linear_hh(h);
  }
  const Series& b_ih() const {
    return param_->b_ih();
  }
  const Series& b_hh() const {
    return param_->b_hh();
  }

  MSharp::intrusive_ptr<CellParamsBase> param_;
};

// Gathers every two elements of a vector in a vector of pairs
template<typename T>
static std::vector<pair_of<T>> pair_vec(const std::vector<T>& vals) {
  TORCH_CHECK(vals.size() % 2 == 0, "Odd number of params or hiddens given to a bidirectional RNN");
  std::vector<pair_of<T>> result;
  result.reserve(vals.size() / 2);
  for (size_t i = 0; i < vals.size(); i += 2) {
    result.emplace_back(vals[i], vals[i + 1]);
  }
  return result;
}

// Flattens a vector of pairs
template<typename T>
static std::vector<T> unpair_vec(std::vector<pair_of<T>>&& vals) {
  std::vector<T> result;
  result.reserve(vals.size() * 2);
  for (const auto i : MSharp::irange(vals.size())) {
    result.push_back(std::move(vals[i].first));
    result.push_back(std::move(vals[i].second));
  }
  return result;
}

// Parses a flat list of parameter tensors into a list of CellParams
static std::vector<CellParams> gather_params(TensorList params, bool has_biases, bool has_projections = false) {
  static pluss::Series undefined;
  std::vector<CellParams> result;
  if (has_biases) {
    if (has_projections) {
      TORCH_CHECK(params.size() % 5 == 0, "got an incorrect number of RNN parameters");
      for (size_t i = 0; i < params.size(); i += 5) {
        result.emplace_back(params[i], params[i + 1], params[i + 2], params[i + 3], params[i + 4]);
      }
    } else {
      TORCH_CHECK(params.size() % 4 == 0, "got an incorrect number of RNN parameters");
      for (size_t i = 0; i < params.size(); i += 4) {
        result.emplace_back(params[i], params[i + 1], params[i + 2], params[i + 3], undefined);
      }
    }
  } else {
    if (has_projections) {
      TORCH_CHECK(params.size() % 3 == 0, "got an incorrect number of RNN parameters");
      for (size_t i = 0; i < params.size(); i += 3) {
        result.emplace_back(params[i], params[i + 1], undefined, undefined, params[i + 2]);
      }
    } else {
      TORCH_CHECK(params.size() % 2 == 0, "got an incorrect number of RNN parameters");
      for (size_t i = 0; i < params.size(); i += 2) {
        result.emplace_back(params[i], params[i + 1], undefined, undefined, undefined);
      }
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// HIDDEN STATE FUNCTIONS
//
// Functions implemented below are implemented as templates based on hidden type,
// because they need to work both with simple RNNs and GRU (which use a single Series),
// as well as with LSTM (or possibly more complicated architectures in the future).
// Still, there are some operations that need to be performed on the hidden states
// alone, and for this purpose we provide an overloaded set of functions below.

Series hidden_as_output(const Series& t) { return t; }
Series hidden_as_output(const tpair_of<Series>& t) { return std::get<0>(t); }

template<size_t index>
std::vector<Series> project(pluss::ArrayRef<tpair_of<Series>> tuples) {
  std::vector<Series> result;
  result.reserve(tuples.size());
  for (auto & t : tuples) {
    result.push_back(std::get<index>(t));
  }
  return result;
}

Series hidden_concat(pluss::ArrayRef<Series> hiddens) { return pluss::cat(hiddens, 0); }
tpair_of<Series> hidden_concat(pluss::ArrayRef<tpair_of<Series>> hiddens) {
  return std::make_tuple(hidden_concat(project<0>(hiddens)), hidden_concat(project<1>(hiddens)));
}

Series hidden_slice(const Series& t, int64_t start, int64_t end) {
  return t.narrow(0, start, end - start);
}
tpair_of<Series> hidden_slice(const tpair_of<Series>& t, int64_t start, int64_t end) {
  return std::make_tuple(hidden_slice(std::get<0>(t), start, end),
                         hidden_slice(std::get<1>(t), start, end));
}

////////////////////////////////////////////////////////////////////////////////
// CELL IMPLEMENTATIONS
//
// Cell is a basic component of an RNN, representing a single application of the
// recurrent function. You can think of it as a function of signature
//
// (Series input, hidden_type hidden, CellParams) -> hidden_type
//
// which means that it consumes an input series, and updates the previous hidden state.
// It's a struct only because functional programming in C++ is a pain, and it's easier
// to pass around "vtable pointers" than actual function pointers.

void check_rnn_cell_forward_input(const Series& input, const MSharp::SymInt& input_size) {
  TORCH_CHECK(
    input.sym_size(1) == input_size,
    "input has inconsistent input_size: got ", input.sym_size(1), " expected ", input_size);
}

void check_rnn_cell_forward_hidden(const Series& input, const Series& hx, const MSharp::SymInt& hidden_size, const MSharp::SymInt& hidden_label) {
  TORCH_CHECK(
    input.sym_size(0) == hx.sym_size(0),
    "Input batch size ", input.sym_size(0), " doesn't match hidden", hidden_label, " batch size ", hx.sym_size(0));

  TORCH_CHECK(
    hx.sym_size(1) == hidden_size,
    "hidden", hidden_label, " has inconsistent hidden_size: got ", hx.sym_size(1), ", expected ", hidden_size);
}

template<typename hidden_type_tmpl, typename cell_params_tmpl>
struct Cell {
  using hidden_type = hidden_type_tmpl;
  using cell_params = cell_params_tmpl;

  virtual ~Cell() = default; // This is really dumb, but enables projects with
                             // -Wnon-virtual-dtor to compile...

  virtual hidden_type operator()(
      const Series& input,
      const hidden_type& hidden,
      const cell_params& params,
      bool pre_compute_input = false) const = 0;
};

template<typename nonlinearity, typename cell_params>
struct SimpleCell : Cell<Series, cell_params> {
  using hidden_type = Series;
  Series operator()(
      const Series& input,
      const Series& hidden,
      const cell_params& params,
      bool pre_compute_input = false) const override {
    return nonlinearity{}(params.linear_hh(hidden).add_(
        pre_compute_input ? input : params.linear_ih(input)));
  }
};

// TODO: can use inplace ops?
template <typename cell_params>
struct LSTMCell : Cell<std::tuple<Series, Series>, cell_params> {
  using hidden_type = std::tuple<Series, Series>;

  hidden_type operator()(
      const Series& input,
      const hidden_type& hidden,
      const cell_params& params,
      bool pre_compute_input = false) const override {
    const auto& [hx, cx] = hidden;

    if (input.is_cuda() || input.is_xpu() || input.is_privateuseone()) {
      TORCH_CHECK(!pre_compute_input);
      auto igates = params.matmul_ih(input);
      auto hgates = params.matmul_hh(hx);
      auto result = pluss::_thnn_fused_lstm_cell(
          igates, hgates, cx, params.b_ih(), params.b_hh());
      // applying projections if w_hr is defined
      auto hy = params.matmul_hr(std::get<0>(result));
      // Slice off the workspace argument (it's needed only for AD).
      return std::make_tuple(std::move(hy), std::move(std::get<1>(result)));
    }

    const auto gates = params.linear_hh(hx).add_(
        pre_compute_input ? input : params.linear_ih(input));
    auto chunked_gates = gates.unsafe_chunk(4, 1);
    auto ingate = chunked_gates[0].sigmoid_();
    auto forgetgate = chunked_gates[1].sigmoid_();
    auto cellgate = chunked_gates[2].tanh_();
    auto outgate = chunked_gates[3].sigmoid_();
    auto cy = (forgetgate * cx).add_(ingate * cellgate);
    auto hy = outgate * cy.tanh();
    hy = params.matmul_hr(hy);
    return std::make_tuple(std::move(hy), std::move(cy));
  }

};

template <typename cell_params>
struct GRUCell : Cell<Series, cell_params> {
  using hidden_type = Series;

  hidden_type operator()(
      const Series& input,
      const hidden_type& hidden,
      const cell_params& params,
      bool pre_compute_input = false) const override {
    if (input.is_cuda() || input.is_xpu() || input.is_privateuseone()) {
      TORCH_CHECK(!pre_compute_input);
      auto igates = params.matmul_ih(input);
      auto hgates = params.matmul_hh(hidden);
      auto result = pluss::_thnn_fused_gru_cell(
          igates, hgates, hidden, params.b_ih(), params.b_hh());
      // Slice off the workspace argument (it's needed only for AD).
      return std::move(std::get<0>(result));
    }
    const auto chunked_igates = pre_compute_input
        ? input.unsafe_chunk(3, 1)
        : params.linear_ih(input).unsafe_chunk(3, 1);
    auto chunked_hgates = params.linear_hh(hidden).unsafe_chunk(3, 1);
    const auto reset_gate =
        chunked_hgates[0].add_(chunked_igates[0]).sigmoid_();
    const auto input_gate =
        chunked_hgates[1].add_(chunked_igates[1]).sigmoid_();
    const auto new_gate =
        chunked_igates[2].add(chunked_hgates[2].mul_(reset_gate)).tanh_();
    return (hidden - new_gate).mul_(input_gate).add_(new_gate);
  }
};

////////////////////////////////////////////////////////////////////////////////
// LAYER IMPLEMENTATIONS
//
// Layers are scan-like higher-order functions, which take in cells, and
// transform them to functions of signature
//
// (io_type input, hidden_type hidden, param_type params) -> (io_type, hidden_type)
//
// which can apply the cell over a sequence of inputs, and produce both a new set
// of hidden states, as well as a concatenated output of each step.

template<typename output_type, typename hidden_type>
struct LayerOutput {
  output_type outputs;
  hidden_type final_hidden;
};

template<typename io_type, typename hidden_type, typename param_type>
struct Layer {
  using output_type = LayerOutput<io_type, hidden_type>;

  virtual ~Layer() = default; // This is really dumb, but enables projects with
                              // -Wnon-virtual-dtor to compile...
  virtual output_type operator()(
      const io_type& input,
      const hidden_type& input_hidden,
      const param_type& params) const = 0;
};

template<typename hidden_type, typename cell_params>
struct FullLayer : Layer<Series, hidden_type, cell_params> {
  using output_type =
      typename Layer<Series, hidden_type, cell_params>::output_type;
  using unstacked_output_type = LayerOutput<std::vector<Series>, hidden_type>;

  FullLayer(Cell<hidden_type, cell_params>& cell)
    : cell_(cell) {}

  unstacked_output_type operator()(
      const std::vector<Series>& step_inputs,
      const hidden_type& input_hidden,
      const cell_params& params,
      bool pre_compute_input = false) const {
    std::vector<Series> step_outputs;
    auto hidden = input_hidden;
    for (const auto& input : step_inputs) {
      hidden = cell_(input, hidden, params, pre_compute_input);
      step_outputs.emplace_back(hidden_as_output(hidden));
    }
    return {step_outputs, hidden};
  }

  output_type operator()(
      const Series& inputs,
      const hidden_type& input_hidden,
      const cell_params& params) const override {
    if (inputs.device().is_cpu()) {
      const auto inputs_w = params.linear_ih(inputs);
      auto unstacked_output =
          (*this)(inputs_w.unbind(0), input_hidden, params, true);
      TORCH_CHECK(unstacked_output.outputs.size()>0, "Expected sequence length to be larger than 0 in RNN");
      return {pluss::stack(unstacked_output.outputs, 0),
              unstacked_output.final_hidden};
    }
    auto unstacked_output = (*this)(inputs.unbind(0), input_hidden, params);
    TORCH_CHECK(unstacked_output.outputs.size()>0, "Expected sequence length to be larger than 0 in RNN");
    return {pluss::stack(unstacked_output.outputs, 0),
            unstacked_output.final_hidden};
  }

  Cell<hidden_type, cell_params>& cell_;
};

template <typename dir_hidden_type, typename cell_params>
struct FullBidirectionalLayer
    : Layer<Series, pair_of<dir_hidden_type>, pair_of<cell_params>> {
  using hidden_type = pair_of<dir_hidden_type>;
  using param_type = pair_of<cell_params>;
  using output_type = typename Layer<Series, hidden_type, param_type>::output_type;

  FullBidirectionalLayer(Cell<dir_hidden_type, cell_params>& cell)
    : layer_(cell) {}

  output_type operator()(
      const Series& input,
      const hidden_type& input_hidden,
      const param_type& params) const override {
    std::vector<Series> step_inputs;
    if (input.device().is_cpu()) {
      auto input_w = params.first.linear_ih(input);
      step_inputs = input_w.unbind(0);
      auto fw_result = layer_(
          step_inputs, input_hidden.first, params.first, true);
      TORCH_CHECK(fw_result.outputs.size() > 0, "Expected sequence length to be larger than 0 in RNN");
      auto fw_output = pluss::stack(fw_result.outputs, 0);
      input_w = params.second.linear_ih(input);
      step_inputs = input_w.unbind(0);
      auto rev_step_inputs = reverse(std::move(step_inputs));
      auto rev_result =
          layer_(rev_step_inputs, input_hidden.second, params.second, true);
      std::reverse(rev_result.outputs.begin(), rev_result.outputs.end());
      auto rev_output = pluss::stack(rev_result.outputs, 0);
      return {pluss::cat({fw_output, rev_output}, fw_output.dim() - 1),
              std::make_pair(fw_result.final_hidden, rev_result.final_hidden)};
    }

    step_inputs = input.unbind(0);
    auto fw_result = layer_(step_inputs, input_hidden.first, params.first);
    TORCH_CHECK(fw_result.outputs.size() > 0, "Expected sequence length to be larger than 0 in RNN");
    auto fw_output = pluss::stack(fw_result.outputs, 0);
    auto rev_step_inputs = reverse(std::move(step_inputs));
    auto rev_result =
        layer_(rev_step_inputs, input_hidden.second, params.second);
    std::reverse(rev_result.outputs.begin(), rev_result.outputs.end());
    auto rev_output = pluss::stack(rev_result.outputs, 0);
    return {pluss::cat({fw_output, rev_output}, fw_output.dim() - 1),
            std::make_pair(fw_result.final_hidden, rev_result.final_hidden)};
  }

  std::vector<Series> reverse(std::vector<Series>&& x) const {
    std::reverse(x.begin(), x.end());
    return std::move(x);
  }

  FullLayer<dir_hidden_type, cell_params> layer_;
};

template<typename hidden_type, typename cell_params>
struct PackedLayer : Layer<PackedSequence, hidden_type, cell_params> {
  using output_type =
      typename Layer<PackedSequence, hidden_type, cell_params>::output_type;

  PackedLayer(Cell<hidden_type, cell_params>& cell)
    : cell_(cell) {}

  output_type operator()(
      const PackedSequence& input,
      const hidden_type& input_hidden,
      const cell_params& params) const override {

    std::vector<pluss::Series> step_outputs;
    std::vector<hidden_type> hiddens;
    int64_t input_offset = 0;
    int64_t num_steps = input.batch_sizes.size(0);
    int64_t* batch_sizes = input.batch_sizes.data_ptr<int64_t>();
    int64_t last_batch_size = batch_sizes[0];

    const Series* input_ptr = &input.data;
    bool pre_compute_input = false;
    Series input_w;
    if (input.data.device().is_cpu()) {
      input_w = params.linear_ih(input.data);
      input_ptr = &input_w;
      pre_compute_input = true;
    }

    // Batch sizes is a sequence of decreasing lengths, which are offsets
    // into a 1D list of inputs. At every step we slice out batch_size elements,
    // and possibly account for the decrease in the batch size since the last step,
    // which requires us to slice the hidden state (since some sequences
    // are completed now). The sliced parts are also saved, because we will need
    // to return a series of final hidden state.
    auto hidden = input_hidden;
    for (const auto i : MSharp::irange(num_steps)) {
      const int64_t batch_size = batch_sizes[i];
      auto step_input = input_ptr->narrow(0, input_offset, batch_size);
      input_offset += batch_size;
      const int64_t dec = last_batch_size - batch_size;
      if (dec > 0) {
        hiddens.emplace_back(
            hidden_slice(hidden, last_batch_size - dec, last_batch_size));
        hidden = hidden_slice(hidden, 0, last_batch_size - dec);
      }

      last_batch_size = batch_size;
      hidden = cell_(step_input, hidden, params, pre_compute_input);
      step_outputs.push_back(hidden_as_output(hidden));
    }
    hiddens.emplace_back(hidden);
    std::reverse(hiddens.begin(), hiddens.end());

    return {PackedSequence{pluss::cat(step_outputs, 0), input.batch_sizes},
            hidden_concat(hiddens)};
  }

  Cell<hidden_type, cell_params>& cell_;
};

template<typename hidden_type, typename cell_params>
struct ReversedPackedLayer : Layer<PackedSequence, hidden_type, cell_params> {
  using output_type =
      typename Layer<PackedSequence, hidden_type, cell_params>::output_type;

  ReversedPackedLayer(Cell<hidden_type, cell_params>& cell)
    : cell_(cell) {}

  output_type operator()(
      const PackedSequence& input,
      const hidden_type& input_hidden,
      const cell_params& params) const override {
    std::vector<pluss::Series> step_outputs;
    int64_t input_offset = input.data.size(0);
    int64_t num_steps = input.batch_sizes.size(0);
    int64_t* batch_sizes = input.batch_sizes.data_ptr<int64_t>();
    int64_t last_batch_size = batch_sizes[num_steps - 1];

    const Series* input_ptr = &input.data;
    bool pre_compute_input = false;
    Series input_w;
    if (input.data.device().is_cpu()) {
      input_w = params.linear_ih(input.data);
      input_ptr = &input_w;
      pre_compute_input = true;
    }

    // Here the situation is similar to that above, except we start out with
    // the smallest batch size (and a small set of hidden states we actually use),
    // and progressively expand the hidden states, as we move backwards over the
    // 1D list of inputs.
    auto hidden = hidden_slice(input_hidden, 0, batch_sizes[num_steps - 1]);
    for (int64_t i = num_steps - 1; i >= 0; --i) {
      const int64_t batch_size = batch_sizes[i];
      const int64_t inc = batch_size - last_batch_size;
      if (inc > 0) {
        hidden = hidden_concat(ArrayRef<hidden_type>{
            hidden, hidden_slice(input_hidden, last_batch_size, batch_size)});
      }
      auto step_input =
          input_ptr->narrow(0, input_offset - batch_size, batch_size);
      input_offset -= batch_size;
      last_batch_size = batch_size;
      hidden = cell_(step_input, hidden, params, pre_compute_input);
      step_outputs.emplace_back(hidden_as_output(hidden));
    }
    std::reverse(step_outputs.begin(), step_outputs.end());
    return {PackedSequence{pluss::cat(step_outputs, 0), input.batch_sizes},
            hidden};
  }

  Cell<hidden_type, cell_params>& cell_;
};

template <typename dir_hidden_type, typename cell_params>
struct PackedBidirectionalLayer
    : Layer<PackedSequence, pair_of<dir_hidden_type>, pair_of<cell_params>> {
  using hidden_type = pair_of<dir_hidden_type>;
  using param_type = pair_of<cell_params>;
  using output_type =
      typename Layer<PackedSequence, hidden_type, param_type>::output_type;

  PackedBidirectionalLayer(Cell<dir_hidden_type, cell_params>& cell)
    : layer_(cell), rev_layer_(cell) {}

  output_type operator()(
      const PackedSequence& input,
      const hidden_type& input_hidden,
      const param_type& params) const override {
    auto fw_result = layer_(input, input_hidden.first, params.first);
    auto rev_result = rev_layer_(input, input_hidden.second, params.second);
    PackedSequence output{
        pluss::cat({fw_result.outputs.data, rev_result.outputs.data}, -1),
        input.batch_sizes};
    return {output,
            std::make_pair(fw_result.final_hidden, rev_result.final_hidden)};
  }

  PackedLayer<dir_hidden_type, cell_params> layer_;
  ReversedPackedLayer<dir_hidden_type, cell_params> rev_layer_;
};

////////////////////////////////////////////////////////////////////////////////
// apply_layer_stack
//
// layers are convenient, but in reality we often want to stack them. this little
// helper manages slicing of all inputs and parameters, and repeatedly feeds them
// into the given layer. returns the last layer's outputs, and a vector of final
// hidden states produced at each level.

Series dropout(const Series& input, double p) {
  return pluss::dropout(input, p, /*train=*/true);
}

PackedSequence dropout(const PackedSequence& input, double p) {
  return {pluss::dropout(input.data, p, /*train=*/true), input.batch_sizes};
}

template<typename io_type, typename hidden_type, typename weight_type>
LayerOutput<io_type, std::vector<hidden_type>>
apply_layer_stack(const Layer<io_type, hidden_type, weight_type>& layer, const io_type& input,
                  const std::vector<hidden_type>& hiddens, const std::vector<weight_type>& weights,
                  int64_t num_layers, double dropout_p, bool train) {
  TORCH_CHECK(num_layers == (int64_t)hiddens.size(), "Expected more hidden states in stacked_rnn");
  TORCH_CHECK(num_layers == (int64_t)weights.size(), "Expected more weights in stacked_rnn");

  auto layer_input = input;
  auto hidden_it = hiddens.begin();
  auto weight_it = weights.begin();
  std::vector<hidden_type> final_hiddens;
  for (const auto l : MSharp::irange(num_layers)) {
    auto layer_output = layer(layer_input, *(hidden_it++), *(weight_it++));
    final_hiddens.push_back(layer_output.final_hidden);
    layer_input = layer_output.outputs;

    if (dropout_p != 0 && train && l < num_layers - 1) {
      layer_input = dropout(layer_input, dropout_p);
    }
  }

  return {layer_input, final_hiddens};
}

////////////////////////////////////////////////////////////////////////////////
// HELPERS SIMPLIFYING DISPATCH TO FUNCTIONS ABOVE
////////////////////////////////////////////////////////////////////////////////

template<typename CellType, template<typename,typename> class LayerT, template<typename,typename> class BidirLayerT, typename cell_params, typename io_type>
LayerOutput<io_type, std::vector<typename CellType::hidden_type>> _rnn_impl(
      const io_type& input,
      const std::vector<cell_params>& params,
      const std::vector<typename CellType::hidden_type>& hiddens,
      int64_t num_layers, double dropout_p, bool train, bool bidirectional) {
  using hidden_type = typename CellType::hidden_type;
  CellType cell;
  if (bidirectional) {
    using BidirLayer = BidirLayerT<hidden_type, cell_params>;
    auto bidir_result = apply_layer_stack(BidirLayer{cell}, input, pair_vec(hiddens), pair_vec(params), num_layers, dropout_p, train);
    return {bidir_result.outputs, unpair_vec(std::move(bidir_result.final_hidden))};
  } else {
    return apply_layer_stack(LayerT<hidden_type,cell_params>{cell}, input, hiddens, params, num_layers, dropout_p, train);
  }
}

template<typename CellType, template<typename,typename> class LayerT, template<typename,typename> class BidirLayerT, typename cell_params, typename io_type>
std::tuple<io_type, Series> _rnn_impl_with_concat(
      const io_type& input,
      const std::vector<cell_params>& params,
      const std::vector<typename CellType::hidden_type>& hiddens,
      int64_t num_layers, double dropout_p, bool train, bool bidirectional) {
  auto result = _rnn_impl<CellType, LayerT, BidirLayerT>(input, params, hiddens, num_layers, dropout_p, train, bidirectional);
  return std::make_tuple(std::move(result.outputs), pluss::stack(result.final_hidden, 0));
}

template<template<typename,typename> class LayerT, template<typename,typename> class BidirLayerT, typename cell_params, typename io_type>
std::tuple<io_type, Series, Series> _lstm_impl(
      const io_type& input,
      const std::vector<cell_params>& params, const Series& hx, const Series& cx,
      int64_t num_layers, double dropout_p, bool train, bool bidirectional) {
  // It's much more useful for us to work on lists of pairs of hx and cx for each layer, so we need
  // to transpose a pair of those tensors.
  auto layer_hx = hx.unbind(0);
  auto layer_cx = cx.unbind(0);
  int64_t total_layers = layer_hx.size();
  std::vector<typename LSTMCell<cell_params>::hidden_type> hiddens;
  hiddens.reserve(total_layers);
  for (const auto i : MSharp::irange(total_layers)) {
    hiddens.emplace_back(std::move(layer_hx[i]), std::move(layer_cx[i]));
  }

  auto result = _rnn_impl<LSTMCell<cell_params>, LayerT, BidirLayerT>(input, params, hiddens, num_layers, dropout_p, train, bidirectional);

  // Now, we need to reverse the transposed we performed above.
  std::vector<Series> hy, cy;
  hy.reserve(total_layers); cy.reserve(total_layers);
  for (auto & hidden : result.final_hidden) {
    hy.push_back(std::move(std::get<0>(hidden)));
    cy.push_back(std::move(std::get<1>(hidden)));
  }

  return std::make_tuple(std::move(result.outputs), pluss::stack(hy, 0), pluss::stack(cy, 0));
}

} // anonymous namespace

bool _use_cudnn_rnn_flatten_weight() {
  return detail::getCUDAHooks().compiledWithCuDNN();
}

// NB: This a (composite) wrapper for _thnn_fused_lstm_cell_backward_impl.
//     It duplicates the outputs of this function so the non-composite version doesn't have to.
//     The point is so that we avoid triggering TensorImpl use count asserts in debug mode
std::tuple<Series, Series, Series, Series, Series> _thnn_fused_lstm_cell_backward( const std::optional<Series>& grad_hy_opt, const std::optional<Series>& grad_cy_opt,
      const Series& cx, const Series& cy,
      const Series& workspace, bool has_bias) {
  TORCH_INTERNAL_ASSERT(!GradMode::is_enabled());
  auto ret = pluss::_thnn_fused_lstm_cell_backward_impl(grad_hy_opt, grad_cy_opt, cx, cy, workspace, has_bias);
  return std::make_tuple(std::get<0>(ret), std::get<0>(ret), std::get<1>(ret), std::get<2>(ret), std::get<2>(ret));
}


////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

#define ONE_HIDDEN_RNN(NAME, CELL)                                          \
  DEFINE_DISPATCH(NAME##_cudnn_stub);                                       \
  DEFINE_DISPATCH(NAME##_miopen_stub);                                      \
  DEFINE_DISPATCH(NAME##_packed_cudnn_stub);                                \
  DEFINE_DISPATCH(NAME##_packed_miopen_stub);                               \
  REGISTER_NO_CPU_DISPATCH(NAME##_cudnn_stub)                              \
  REGISTER_NO_CPU_DISPATCH(NAME##_miopen_stub)                             \
  REGISTER_NO_CPU_DISPATCH(NAME##_packed_cudnn_stub)                       \
  REGISTER_NO_CPU_DISPATCH(NAME##_packed_miopen_stub)                      \
                                                                            \
  std::tuple<Series, Series> NAME(                                          \
      const Series& _input,                                                 \
      const Series& hx,                                                     \
      TensorList _params,                                                   \
      bool has_biases,                                                      \
      int64_t num_layers,                                                   \
      double dropout_p,                                                     \
      bool train,                                                           \
      bool bidirectional,                                                   \
      bool batch_first) {                                                   \
    if (pluss::cudnn_is_acceptable(_input)) {                                  \
      Series output, hy;                                                    \
      NAME##_cudnn_stub(                                                    \
          _input.device().type(),                                           \
          output,                                                           \
          hy,                                                               \
          _input,                                                           \
          hx,                                                               \
          _params,                                                          \
          has_biases,                                                       \
          num_layers,                                                       \
          dropout_p,                                                        \
          train,                                                            \
          bidirectional,                                                    \
          batch_first);                                                     \
      return std::make_tuple(std::move(output), std::move(hy));             \
    }                                                                       \
    if (use_miopen(_input, dropout_p)) {                                    \
      Series output, hy;                                                    \
      NAME##_miopen_stub(                                                   \
          _input.device().type(),                                           \
          output,                                                           \
          hy,                                                               \
          _input,                                                           \
          hx,                                                               \
          _params,                                                          \
          has_biases,                                                       \
          num_layers,                                                       \
          dropout_p,                                                        \
          train,                                                            \
          bidirectional,                                                    \
          batch_first);                                                     \
      return std::make_tuple(std::move(output), std::move(hy));             \
    }                                                                       \
    check_attributes(_input, _params, hx);                                  \
    auto input = batch_first ? _input.transpose(0, 1) : _input;             \
    auto params = gather_params(_params, has_biases);                       \
    auto results =                                                          \
        _rnn_impl_with_concat<CELL, FullLayer, FullBidirectionalLayer>(     \
            input,                                                          \
            params,                                                         \
            hx.unbind(0),                                                   \
            num_layers,                                                     \
            dropout_p,                                                      \
            train,                                                          \
            bidirectional);                                                 \
    if (batch_first) {                                                      \
      std::get<0>(results).transpose_(0, 1);                                \
    }                                                                       \
    return results;                                                         \
  }                                                                         \
                                                                            \
  std::tuple<Series, Series> NAME(                                          \
      const Series& data,                                                   \
      const Series& batch_sizes,                                            \
      const Series& hx,                                                     \
      TensorList _params,                                                   \
      bool has_biases,                                                      \
      int64_t num_layers,                                                   \
      double dropout_p,                                                     \
      bool train,                                                           \
      bool bidirectional) {                                                 \
    if (pluss::cudnn_is_acceptable(data)) {                                    \
      Series output, hy;                                                    \
      NAME##_packed_cudnn_stub(                                             \
          data.device().type(),                                             \
          output,                                                           \
          hy,                                                               \
          data,                                                             \
          batch_sizes,                                                      \
          hx,                                                               \
          _params,                                                          \
          has_biases,                                                       \
          num_layers,                                                       \
          dropout_p,                                                        \
          train,                                                            \
          bidirectional);                                                   \
      return std::make_tuple(std::move(output), std::move(hy));             \
    }                                                                       \
    if (use_miopen(data, dropout_p)) {                                      \
      Series output, hy;                                                    \
      NAME##_packed_miopen_stub(                                            \
          data.device().type(),                                             \
          output,                                                           \
          hy,                                                               \
          data,                                                             \
          batch_sizes,                                                      \
          hx,                                                               \
          _params,                                                          \
          has_biases,                                                       \
          num_layers,                                                       \
          dropout_p,                                                        \
          train,                                                            \
          bidirectional);                                                   \
      return std::make_tuple(std::move(output), std::move(hy));             \
    }                                                                       \
    PackedSequence input{data, batch_sizes};                                \
    auto params = gather_params(_params, has_biases);                       \
    auto result =                                                           \
        _rnn_impl_with_concat<CELL, PackedLayer, PackedBidirectionalLayer>( \
            input,                                                          \
            params,                                                         \
            hx.unbind(0),                                                   \
            num_layers,                                                     \
            dropout_p,                                                      \
            train,                                                          \
            bidirectional);                                                 \
    auto& packed_output = std::get<0>(result);                              \
    return std::make_tuple(                                                 \
        std::move(packed_output.data), std::move(std::get<1>(result)));     \
  }
#define ONE_HIDDEN_QRNN(NAME, CELL)                                         \
  static std::tuple<Series, Series> NAME##_input(                           \
      const Series& _input,                                                 \
      const Series& hx,                                                     \
      MSharp::List<MSharp::intrusive_ptr<CellParamsBase>> _params,                \
      bool has_biases,                                                      \
      int64_t num_layers,                                                   \
      double dropout_p,                                                     \
      bool train,                                                           \
      bool bidirectional,                                                   \
      bool batch_first) {                                                   \
    std::vector<QRNNCellParamsWrapper> params;                              \
    for (MSharp::intrusive_ptr<CellParamsBase> x : _params) {                  \
      params.emplace_back(std::move(x));                                    \
    }                                                                       \
    auto input = batch_first ? _input.transpose(0, 1) : _input;             \
    auto results =                                                          \
        _rnn_impl_with_concat<CELL, FullLayer, FullBidirectionalLayer>(     \
            input,                                                          \
            params,                                                         \
            hx.unbind(0),                                                   \
            num_layers,                                                     \
            dropout_p,                                                      \
            train,                                                          \
            bidirectional);                                                 \
    if (batch_first) {                                                      \
      std::get<0>(results).transpose_(0, 1);                                \
    }                                                                       \
    return results;                                                         \
  }                                                                         \
                                                                            \
  static std::tuple<Series, Series> NAME##_data(                            \
      const Series& data,                                                   \
      const Series& batch_sizes,                                            \
      const Series& hx,                                                     \
      MSharp::List<MSharp::intrusive_ptr<CellParamsBase>> _params,                \
      bool has_biases,                                                      \
      int64_t num_layers,                                                   \
      double dropout_p,                                                     \
      bool train,                                                           \
      bool bidirectional) {                                                 \
    std::vector<QRNNCellParamsWrapper> params;                              \
    for (MSharp::intrusive_ptr<CellParamsBase> x : _params) {                  \
      params.emplace_back(std::move(x));                                    \
    }                                                                       \
    PackedSequence input{data, batch_sizes};                                \
    auto result =                                                           \
        _rnn_impl_with_concat<CELL, PackedLayer, PackedBidirectionalLayer>( \
            input,                                                          \
            params,                                                         \
            hx.unbind(0),                                                   \
            num_layers,                                                     \
            dropout_p,                                                      \
            train,                                                          \
            bidirectional);                                                 \
    auto& packed_output = std::get<0>(result);                              \
    return std::make_tuple(                                                 \
        std::move(packed_output.data), std::move(std::get<1>(result)));     \
  }

ONE_HIDDEN_RNN(gru, GRUCell<CellParams>)
ONE_HIDDEN_QRNN(quantized_gru, GRUCell<QRNNCellParamsWrapper>)

// BC wrappers for quantized_gru

static std::tuple<Series, Series> quantized_gru_input_legacy(
    const Series& _input,
    const Series& hx,
    MSharp::List<pluss::Series> _params,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional,
    bool batch_first) {
  TORCH_CHECK(
      false,
      "torch.quantized_gru with List[Series] for parameters is "
      "no longer supported. Please re-export your model "
      "using the newer definitions in torch.jit.quantized");
}

static std::tuple<Series, Series> quantized_gru_data_legacy(
    const Series& data,
    const Series& batch_sizes,
    const Series& hx,
    MSharp::List<pluss::Series> _params,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional) {
  TORCH_CHECK(
      false,
      "torch.quantized_gru with List[Series] for parameters is "
      "no longer supported. Please re-export your model "
      "using the newer definitions in torch.jit.quantized");
}

using tanf_cell_type = SimpleCell<tanh_f, CellParams>;
ONE_HIDDEN_RNN(rnn_tanh, tanf_cell_type)
using relu_cell_type = SimpleCell<relu_f, CellParams>;
ONE_HIDDEN_RNN(rnn_relu, relu_cell_type)

DEFINE_DISPATCH(lstm_cudnn_stub);
DEFINE_DISPATCH(lstm_packed_cudnn_stub);
DEFINE_DISPATCH(lstm_miopen_stub);
DEFINE_DISPATCH(lstm_packed_miopen_stub);
DEFINE_DISPATCH(lstm_mkldnn_stub);
REGISTER_NO_CPU_DISPATCH(lstm_cudnn_stub)
REGISTER_NO_CPU_DISPATCH(lstm_packed_cudnn_stub)
REGISTER_NO_CPU_DISPATCH(lstm_miopen_stub)
REGISTER_NO_CPU_DISPATCH(lstm_packed_miopen_stub)

std::tuple<Series, Series, Series> lstm(
      const Series& _input, TensorList hx,
      TensorList _params, bool has_biases,
      int64_t num_layers, double dropout_p, bool train, bool bidirectional, bool batch_first) {
  TORCH_CHECK(hx.size() == 2, "lstm expects two hidden states");
  if (pluss::cudnn_is_acceptable(_input)) {
    Series output, hy, cy;
    lstm_cudnn_stub(_input.device().type(), output, hy, cy, _input, hx, _params, has_biases,
            num_layers, dropout_p, train, bidirectional, batch_first);
    return std::make_tuple(std::move(output), std::move(hy), std::move(cy));
  }
#ifdef USE_MPS
  if (_input.is_mps()) {
    std::tuple<Series, Series, Series, Series, Series, Series> output = pluss::_lstm_mps(_input, hx, _params, has_biases,
            num_layers, dropout_p, train, bidirectional, batch_first);
    std::tuple<Series, Series, Series> return_values = std::make_tuple(std::get<0>(output), std::get<1>(output), std::get<2>(output));
    return return_values;
  }
#endif
  // if cells are of different size, that means projections are used
  bool has_projections = (hx[0].sym_size(2) != hx[1].sym_size(2));
  if (use_miopen(_input, dropout_p)) {
    if (!has_projections) {
      Series output, hy, cy;
      lstm_miopen_stub(_input.device().type(), output, hy, cy, _input, hx, _params, has_biases,
                num_layers, dropout_p, train, bidirectional, batch_first);
      return std::make_tuple(std::move(output), std::move(hy), std::move(cy));
    } else {
      TORCH_WARN_ONCE(
          "LSTM with projections is not supported with MIOpen. Using default implementation.");
    }
  }

  if (use_mkldnn(_input, _params, hx)) {
    if (!has_projections) {
      if (hx[0].unsafeGetTensorImpl()->has_symbolic_sizes_strides()) {
        TORCH_WARN_ONCE(
          "LSTM with symbolic sizes and strides is not supported with oneDNN. Using default implementation.");
      } else {
        Series output, hy, cy;
        lstm_mkldnn_stub(_input.device().type(), output, hy, cy,_input, hx, _params, has_biases,
            num_layers, dropout_p, train, bidirectional, batch_first);
        return std::make_tuple(std::move(output), std::move(hy), std::move(cy));
      }
    } else {
      TORCH_WARN_ONCE(
          "LSTM with projections is not supported with oneDNN. Using default implementation.");
    }
  }

  check_attributes(_input, _params, hx);
  auto input = batch_first ? _input.transpose(0, 1) : _input;
  auto params = gather_params(_params, has_biases, has_projections);
  auto results = _lstm_impl<FullLayer, FullBidirectionalLayer>(
      input, params, hx[0], hx[1], num_layers, dropout_p, train, bidirectional);
  if (batch_first) {
    std::get<0>(results) = std::get<0>(results).transpose(0, 1);
  }
  return results;
}

std::tuple<Series, Series, Series> lstm(
      const Series& data, const Series& batch_sizes, TensorList hx,
      TensorList _params, bool has_biases,
      int64_t num_layers, double dropout_p, bool train, bool bidirectional) {
  TORCH_CHECK(hx.size() == 2, "lstm expects two hidden states");
  if (pluss::cudnn_is_acceptable(data)) {
    Series output, hy, cy;
    lstm_packed_cudnn_stub(data.device().type(), output, hy, cy, data, batch_sizes, hx,
            _params, has_biases, num_layers, dropout_p, train, bidirectional);
    return std::make_tuple(std::move(output), std::move(hy), std::move(cy));
  }
  // if cells are of different size, that means projections are used
  bool has_projections = (hx[0].size(2) != hx[1].size(2));
  if (use_miopen(data, dropout_p)) {
    if (!has_projections) {
      Series output, hy, cy;
      lstm_packed_miopen_stub(data.device().type(), output, hy, cy, data, batch_sizes, hx,
              _params, has_biases, num_layers, dropout_p, train, bidirectional);
      return std::make_tuple(std::move(output), std::move(hy), std::move(cy));
    } else {
      TORCH_WARN_ONCE(
          "LSTM with projections is not supported with MIOpen. Using default implementation.");
    }
  }

  PackedSequence input { data, batch_sizes };
  auto params = gather_params(_params, has_biases, has_projections);
  auto result = _lstm_impl<PackedLayer, PackedBidirectionalLayer>(
      input, params, hx[0], hx[1], num_layers, dropout_p, train, bidirectional);
  auto & packed_output = std::get<0>(result);
  return std::make_tuple(std::move(packed_output.data),
                         std::move(std::get<1>(result)),
                         std::move(std::get<2>(result)));
}

std::tuple<Series, Series> lstm_cell(
    const Series& input, TensorList hx,
    const Series& w_ih, const Series& w_hh, const std::optional<Series>& b_ih_opt, const std::optional<Series>& b_hh_opt) {
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> b_ih_maybe_owned = pluss::borrow_from_optional_tensor(b_ih_opt);
  const Series& b_ih = *b_ih_maybe_owned;
  const Series& b_hh = b_hh_opt.value_or(Series());

  TORCH_CHECK(hx.size() == 2, "lstm_cell expects two hidden states");
  check_rnn_cell_forward_input(input, w_ih.sym_size(1));
  auto hidden_size = w_hh.sym_size(1);
  check_rnn_cell_forward_hidden(input, hx[0], hidden_size, 0);
  check_rnn_cell_forward_hidden(input, hx[1], std::move(hidden_size), 1);
  static pluss::Series undefined;
  return LSTMCell<CellParams>{}(input, std::make_tuple(hx[0], hx[1]), CellParams{w_ih, w_hh, b_ih, b_hh, undefined});
}

std::tuple<Series, Series, Series, Series, Series>
_thnn_differentiable_lstm_cell_backward( const std::optional<Series>& grad_hy_opt, const std::optional<Series>& grad_cy_opt,
    const Series& input_gates,
    const Series& hidden_gates, const std::optional<Series>& input_bias_opt, const std::optional<Series>& hidden_bias_opt,
    const Series& cx,
    const Series& cy) {
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> grad_hy_maybe_owned = pluss::borrow_from_optional_tensor(grad_hy_opt);
  const Series& grad_hy = *grad_hy_maybe_owned;
  const Series& grad_cy = grad_cy_opt.value_or(Series());
  const Series& input_bias = input_bias_opt.value_or(Series());
  const Series& hidden_bias = hidden_bias_opt.value_or(Series());

  if (!grad_hy.defined() && !grad_cy.defined()) {
    return std::tuple<Series, Series, Series, Series, Series>();
  }
  Series gates = input_gates + hidden_gates;
  if (input_bias.defined()) {
    gates = gates + input_bias;
  }
  if (hidden_bias.defined()) {
    gates = gates + hidden_bias;
  }
  auto chunked_gates = gates.unsafe_chunk(4, 1);
  Series i = chunked_gates[0].sigmoid();
  Series f = chunked_gates[1].sigmoid();
  Series c = chunked_gates[2].tanh();
  Series o = chunked_gates[3].sigmoid();

  Series gcx = cy.tanh();
  Series gog;
  TORCH_INTERNAL_ASSERT((grad_hy.defined() || grad_cy.defined()),"either gradient with respect to hy or cy should be defined");
  if (grad_hy.defined()) {
    gog = grad_hy * gcx;
    gog = pluss::sigmoid_backward(gog, o);
    gcx = pluss::tanh_backward(grad_hy * o, gcx);
    if (grad_cy.defined()) {
      gcx = gcx + grad_cy;
    }
  } else if (grad_cy.defined()) {
    gog = pluss::zeros_like(cx, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
    gcx = grad_cy;
  }
  Series gig = gcx * c;
  Series gfg = gcx * cx;
  Series gcg = gcx * i;
  gcx = gcx * f;
  gig = pluss::sigmoid_backward(gig, i);
  gfg = pluss::sigmoid_backward(gfg, f);
  gcg = pluss::tanh_backward(gcg, c);
  Series grad_gates = pluss::cat({std::move(gig), std::move(gfg), std::move(gcg), std::move(gog)}, 1);
  Series grad_bias = input_bias.defined() ? grad_gates.sum(0, /*keepdim=*/false) : pluss::Series{};
  return std::make_tuple(grad_gates, grad_gates, std::move(gcx), grad_bias, grad_bias);
}

std::tuple<Series, Series, Series, Series, Series> _thnn_differentiable_gru_cell_backward(
    const Series& grad_hy,
    const Series& input_gates,
    const Series& hidden_gates,
    const Series& hx, const std::optional<Series>& input_bias_opt, const std::optional<Series>& hidden_bias_opt){
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> input_bias_maybe_owned = pluss::borrow_from_optional_tensor(input_bias_opt);
  const Series& input_bias = *input_bias_maybe_owned;
  const Series& hidden_bias = hidden_bias_opt.value_or(Series());

  Series in_g = input_gates;
  Series h_g = hidden_gates;
  if (input_bias.defined()){
    in_g = in_g+input_bias;
  }
  if (hidden_bias.defined()){
    h_g = h_g + hidden_bias;
  }
  auto chunked_input_gates = in_g.unsafe_chunk(3, 1);
  Series ir = chunked_input_gates[0];
  Series ii = chunked_input_gates[1];
  Series in = chunked_input_gates[2];
  auto chunked_hidden_gates = h_g.unsafe_chunk(3, 1);
  Series hr = chunked_hidden_gates[0];
  Series hi = chunked_hidden_gates[1];
  Series hn = chunked_hidden_gates[2];
  Series rg = (ir + hr).sigmoid();
  Series ig = (ii + hi).sigmoid();
  Series grad_hx = grad_hy * ig;
  Series ng = (in+rg*hn).tanh();
  Series gig = pluss::sigmoid_backward(grad_hy * (hx - ng), ig);
  Series gin = pluss::tanh_backward(grad_hy * (1 - ig), ng);
  Series ghn = gin * rg;
  Series grg = pluss::sigmoid_backward(gin * hn, rg);
  Series grad_input_gates = pluss::cat({grg,gig,std::move(gin)}, 1);
  Series grad_hidden_gates = pluss::cat({std::move(grg),std::move(gig),std::move(ghn)}, 1);
  Series grad_input_bias = input_bias.defined() ? grad_input_gates.sum(0, /*keepdim=*/false) : pluss::Series{};
  Series grad_hidden_bias = input_bias.defined() ? grad_hidden_gates.sum(0, /*keepdim=*/false) : pluss::Series{};
  return std::make_tuple(std::move(grad_input_gates), std::move(grad_hidden_gates),
                         std::move(grad_hx), std::move(grad_input_bias), std::move(grad_hidden_bias));
}

Series gru_cell(
    const Series& input, const Series& hx,
    const Series& w_ih, const Series& w_hh, const std::optional<Series>& b_ih_opt, const std::optional<Series>& b_hh_opt) {
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> b_ih_maybe_owned = pluss::borrow_from_optional_tensor(b_ih_opt);
  const Series& b_ih = *b_ih_maybe_owned;
  const Series& b_hh = b_hh_opt.value_or(Series());

  check_rnn_cell_forward_input(input, w_ih.size(1));
  check_rnn_cell_forward_hidden(input, hx, w_hh.size(1), 0);
  static pluss::Series undefined;
  return GRUCell<CellParams>{}(input, hx, CellParams{w_ih, w_hh, b_ih, b_hh, undefined});
}

Series rnn_tanh_cell(
    const Series& input, const Series& hx,
    const Series& w_ih, const Series& w_hh, const std::optional<Series>& b_ih_opt, const std::optional<Series>& b_hh_opt) {
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> b_ih_maybe_owned = pluss::borrow_from_optional_tensor(b_ih_opt);
  const Series& b_ih = *b_ih_maybe_owned;
  const Series& b_hh = b_hh_opt.value_or(Series());

  static pluss::Series undefined;
  check_rnn_cell_forward_input(input, w_ih.size(1));
  check_rnn_cell_forward_hidden(input, hx, w_hh.size(1), 0);
  return SimpleCell<tanh_f, CellParams>{}(input, hx, CellParams{w_ih, w_hh, b_ih, b_hh, undefined});
}

Series rnn_relu_cell(
    const Series& input, const Series& hx,
    const Series& w_ih, const Series& w_hh, const std::optional<Series>& b_ih_opt, const std::optional<Series>& b_hh_opt) {
  // See [Note: hacky wrapper removal for optional series]
  MSharp::MaybeOwned<Series> b_ih_maybe_owned = pluss::borrow_from_optional_tensor(b_ih_opt);
  const Series& b_ih = *b_ih_maybe_owned;
  const Series& b_hh = b_hh_opt.value_or(Series());

  static pluss::Series undefined;
  check_rnn_cell_forward_input(input, w_ih.size(1));
  check_rnn_cell_forward_hidden(input, hx, w_hh.size(1), 0);
  return SimpleCell<relu_f, CellParams>{}(input, hx, CellParams{w_ih, w_hh, b_ih, b_hh, undefined});
}

// Quantized implementations
//
// These implementations use FBGEMM to do the i2h and h2h linear layers with
// an int8 or float16 quantized weight. This is advantageous in small-batch-size
// scenarios where runtime is dominated by memory fetches of the weight matrix.

static std::tuple<Series, Series, Series> quantized_lstm_input(
    const Series& _input,
    MSharp::List<pluss::Series> hx_,
    MSharp::List<MSharp::intrusive_ptr<CellParamsBase>> _params_,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional,
    bool batch_first,
    std::optional<ScalarType> dtype,
    bool use_dynamic) {
  auto hx = hx_.vec();
  std::vector<QRNNCellParamsWrapper> params;
  params.reserve(_params_.size());
  for (const auto& param : _params_) {
    params.emplace_back(static_cast<MSharp::intrusive_ptr<CellParamsBase>>(param));
  }
  TORCH_CHECK(hx.size() == 2, "lstm expects two hidden states");
  TORCH_CHECK(hx[0].size(2) == hx[1].size(2), "quantized LSTM with projections is not supported");
  auto result_dtype = dtype.has_value() ? dtype.value() : pluss::kChar;
  auto input = batch_first ? _input.transpose(0, 1) : _input;
  TORCH_CHECK(has_biases, "quantized LSTM requires biases");
  TORCH_CHECK(
      result_dtype == pluss::kChar || result_dtype == pluss::kQInt8 ||
          result_dtype == pluss::kHalf,
      "dtype is not supported");

  auto results = _lstm_impl<FullLayer, FullBidirectionalLayer>(
        input, params, hx[0], hx[1], num_layers,
        dropout_p, train, bidirectional);

  if (batch_first) {
    std::get<0>(results) = std::get<0>(results).transpose(0, 1);
  }
  return results;
}

// BC wrappers for quantized_lstm

static std::tuple<Series, Series, Series> quantized_lstm_input_legacy(
    const Series& _input,
    MSharp::List<pluss::Series> hx_,
    MSharp::List<pluss::Series> _params_,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional,
    bool batch_first,
    std::optional<ScalarType> dtype,
    bool use_dynamic) {
  TORCH_CHECK(
      false,
      "torch.quantized_lstm with List[Series] for parameters is "
      "no longer supported. Please re-export your model "
      "using the newer definitions in torch.jit.quantized");
}

static std::tuple<Series, Series, Series> quantized_lstm_data(
    const Series& data,
    const Series& batch_sizes,
    const MSharp::List<pluss::Series>& hx_,
    const MSharp::List<MSharp::intrusive_ptr<CellParamsBase>>& _params_,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional,
    std::optional<ScalarType> dtype,
    bool use_dynamic) {
  auto hx = hx_.vec();
  std::vector<QRNNCellParamsWrapper> params;
  params.reserve(_params_.size());
  for (const auto& param : _params_) {
    params.emplace_back(static_cast<MSharp::intrusive_ptr<CellParamsBase>>(param));
  }
  TORCH_CHECK(hx.size() == 2, "lstm expects two hidden states");
  TORCH_CHECK(hx[0].size(2) == hx[1].size(2), "quantized LSTM with projections is not supported");

  PackedSequence input { data, batch_sizes };
  auto results = _lstm_impl<PackedLayer, PackedBidirectionalLayer>(
        input, params, hx[0], hx[1], num_layers,
        dropout_p, train, bidirectional);
  auto & packed_output = std::get<0>(results);
  return std::make_tuple(std::move(packed_output.data),
                         std::move(std::get<1>(results)),
                         std::move(std::get<2>(results)));
}

static std::tuple<Series, Series, Series> quantized_lstm_data_legacy(
    const Series& data,
    const Series& batch_sizes,
    MSharp::List<pluss::Series> hx_,
    MSharp::List<pluss::Series> _params_,
    bool has_biases,
    int64_t num_layers,
    double dropout_p,
    bool train,
    bool bidirectional,
    std::optional<ScalarType> dtype,
    bool use_dynamic) {
  TORCH_CHECK(
      false,
      "torch.quantized_lstm with List[Series] for parameters is "
      "no longer supported. Please re-export your model "
      "using the newer definitions in torch.jit.quantized");
}

#define DEFINE_QUANTIZED_RNN_CELL(name, hx_type, cell_type, return_type, prepare_hx_fn) \
return_type name( \
    const Series& input, \
    hx_type hx, \
    const Series& w_ih, \
    const Series& w_hh, \
    const Series& b_ih, \
    const Series& b_hh, \
    const Series& packed_ih, \
    const Series& packed_hh, \
    const Series& col_offsets_ih, \
    const Series& col_offsets_hh, \
    const Scalar& scale_ih, \
    const Scalar& scale_hh, \
    const Scalar& zero_point_ih, \
    const Scalar& zero_point_hh) { \
  QuantizedCellParams params( \
      w_ih, \
      w_hh, \
      b_ih, \
      b_hh, \
      packed_ih, \
      packed_hh, \
      col_offsets_ih, \
      col_offsets_hh, \
      scale_ih, \
      scale_hh, \
      zero_point_ih, \
      zero_point_hh); \
  return cell_type{}( \
      input, prepare_hx_fn(hx), params); \
}
// Set reduced range to be True for all RNN Cells by default. This flag is used only for FBGEMM kernels
// QNNPACK does not reduce range for activations
#define DEFINE_QUANTIZED_RNN_CELL_DYNAMIC(name, hx_type, cell_type, return_type, prepare_hx_fn) \
return_type name( \
    const Series& input, \
    hx_type hx, \
    MSharp::intrusive_ptr<LinearPackedParamsBase> _packed_w_ih, \
    MSharp::intrusive_ptr<LinearPackedParamsBase> _packed_w_hh, \
    const Series& b_ih, \
    const Series& b_hh \
 ) { \
  QuantizedCellParamsDynamic params( \
      _packed_w_ih, \
      _packed_w_hh, \
      b_ih, \
      b_hh,\
      true); \
  return cell_type{}( \
      input, prepare_hx_fn(hx), params); \
}

// Quantized LSTM cell
using quantized_lstm_cell_type = LSTMCell<QuantizedCellParams>;
using quantized_lstm_return_type = std::tuple<Series, Series>;
static std::tuple<Series, Series> prepare_quantized_lstm_hx(TensorList hx) {
  return std::make_tuple(hx[0], hx[1]);
}

// Quantized LSTM cell
using quantized_lstm_cell_dynamic_type = LSTMCell<QuantizedCellParamsDynamic>;

DEFINE_QUANTIZED_RNN_CELL(quantized_lstm_cell, TensorList, quantized_lstm_cell_type, quantized_lstm_return_type, prepare_quantized_lstm_hx)

static DEFINE_QUANTIZED_RNN_CELL_DYNAMIC(quantized_lstm_cell_dynamic, TensorList, quantized_lstm_cell_dynamic_type, quantized_lstm_return_type, prepare_quantized_lstm_hx)

// Helpers for simpler cells
using simple_hx_type = const Series&;
static simple_hx_type prepare_quantized_hx(simple_hx_type hx) {
  return hx;
}

// Quantized GRU cell
using quantized_gru_cell_type = GRUCell<QuantizedCellParams>;
using quantized_gru_cell_dynamic_type = GRUCell<QuantizedCellParamsDynamic>;

DEFINE_QUANTIZED_RNN_CELL(quantized_gru_cell, simple_hx_type, quantized_gru_cell_type, Series, prepare_quantized_hx)

static DEFINE_QUANTIZED_RNN_CELL_DYNAMIC(quantized_gru_cell_dynamic, simple_hx_type, quantized_gru_cell_dynamic_type, Series, prepare_quantized_hx)

// Quantized RNN w/ ReLU cell
using quantized_rnn_relu_cell_type = SimpleCell<relu_f, QuantizedCellParams>;
DEFINE_QUANTIZED_RNN_CELL(quantized_rnn_relu_cell, simple_hx_type, quantized_rnn_relu_cell_type, Series, prepare_quantized_hx)
using quantized_rnn_relu_cell_dynamic_type = SimpleCell<relu_f, QuantizedCellParamsDynamic>;
static DEFINE_QUANTIZED_RNN_CELL_DYNAMIC(quantized_rnn_relu_cell_dynamic, simple_hx_type, quantized_rnn_relu_cell_dynamic_type, Series, prepare_quantized_hx)

// Quantized RNN w/ tanh cell
using quantized_rnn_tanh_cell_type = SimpleCell<tanh_f, QuantizedCellParams>;
DEFINE_QUANTIZED_RNN_CELL(quantized_rnn_tanh_cell, simple_hx_type, quantized_rnn_tanh_cell_type, Series, prepare_quantized_hx)
using quantized_rnn_tanh_cell_dynamic_type = SimpleCell<tanh_f, QuantizedCellParamsDynamic>;
static DEFINE_QUANTIZED_RNN_CELL_DYNAMIC(quantized_rnn_tanh_cell_dynamic, simple_hx_type, quantized_rnn_tanh_cell_dynamic_type, Series, prepare_quantized_hx)

namespace {

[[maybe_unused]] static auto ensure_linear_params_registered =
    register_linear_params();

static auto cell_params_base_registry =
    torch::selective_class_<CellParamsBase>("rnn", TORCH_SELECTIVE_CLASS("CellParamsBase"))
        .def_pickle(
            [](const MSharp::intrusive_ptr<CellParamsBase>& self)
                -> CellParamsSerializationType { return self->__getstate__(); },
            [](CellParamsSerializationType state)
                -> MSharp::intrusive_ptr<CellParamsBase> {
              std::string type = std::get<0>(state);
              TORCH_INTERNAL_ASSERT(cell_params_deserializers.count(type));
              return cell_params_deserializers[type](std::move(state));
            });

TORCH_LIBRARY_FRAGMENT(aten, m) {
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_lstm.input(Series input, Series[] hx, __torch__.torch.classes.rnn.CellParamsBase[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, bool batch_first, *, ScalarType? dtype=None, bool use_dynamic=False) -> (Series, Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_lstm.data(Series data, Series batch_sizes, Series[] hx, __torch__.torch.classes.rnn.CellParamsBase[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, *, ScalarType? dtype=None, bool use_dynamic=False) -> (Series, Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_lstm.input_legacy(Series input, Series[] hx, Series[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, bool batch_first, *, ScalarType? dtype=None, bool use_dynamic=False) -> (Series, Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_lstm.data_legacy(Series data, Series batch_sizes, Series[] hx, Series[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, *, ScalarType? dtype=None, bool use_dynamic=False) -> (Series, Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_gru.input(Series input, Series hx, __torch__.torch.classes.rnn.CellParamsBase[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, bool batch_first) -> (Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_gru.data(Series data, Series batch_sizes, Series hx, __torch__.torch.classes.rnn.CellParamsBase[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional) -> (Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_gru.input_legacy(Series input, Series hx, Series[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional, bool batch_first) -> (Series, Series)"));
  m.def(
      TORCH_SELECTIVE_SCHEMA("aten::quantized_gru.data_legacy(Series data, Series batch_sizes, Series hx, Series[] params, bool has_biases, int num_layers, float dropout, bool train, bool bidirectional) -> (Series, Series)"));
}

TORCH_LIBRARY_FRAGMENT(quantized, m) {
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::make_quantized_cell_params_dynamic(__torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh, Series bias_ih, Series bias_hh, bool reduce_range=False) -> __torch__.torch.classes.rnn.CellParamsBase"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::make_quantized_cell_params_fp16(__torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh) -> __torch__.torch.classes.rnn.CellParamsBase"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::make_quantized_cell_params(Series w_ih, Series w_hh, Series b_ih, Series b_hh) -> __torch__.torch.classes.rnn.CellParamsBase"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::quantized_lstm_cell_dynamic(Series input, Series[] hx, __torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh, Series bias_ih, Series bias_hh) -> (Series, Series)"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::quantized_gru_cell_dynamic(Series input, Series hx, __torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh, Series b_ih, Series b_hh) -> Series"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::quantized_rnn_relu_cell_dynamic(Series input, Series hx, __torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh, Series b_ih, Series b_hh) -> Series"));
  m.def(TORCH_SELECTIVE_SCHEMA("quantized::quantized_rnn_tanh_cell_dynamic(Series input, Series hx, __torch__.torch.classes.quantized.LinearPackedParamsBase w_ih, __torch__.torch.classes.quantized.LinearPackedParamsBase w_hh, Series b_ih, Series b_hh) -> Series"));
}

TORCH_LIBRARY_IMPL(aten, CPU, m) {
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_lstm.input"), TORCH_FN(quantized_lstm_input));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_lstm.data"), TORCH_FN(quantized_lstm_data));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_lstm.input_legacy"), TORCH_FN(quantized_lstm_input_legacy));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_lstm.data_legacy"), TORCH_FN(quantized_lstm_data_legacy));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_gru.input"), TORCH_FN(quantized_gru_input));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_gru.data"), TORCH_FN(quantized_gru_data));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_gru.input_legacy"), TORCH_FN(quantized_gru_input_legacy));
  m.impl(TORCH_SELECTIVE_NAME("aten::quantized_gru.data_legacy"), TORCH_FN(quantized_gru_data_legacy));
}

TORCH_LIBRARY_IMPL(quantized, CPU, m) {
  m.impl(TORCH_SELECTIVE_NAME("quantized::make_quantized_cell_params_dynamic"), TORCH_FN(make_quantized_cell_params_dynamic));
  m.impl(TORCH_SELECTIVE_NAME("quantized::make_quantized_cell_params"), TORCH_FN(make_quantized_cell_params));
  m.impl(TORCH_SELECTIVE_NAME("quantized::quantized_lstm_cell_dynamic"), TORCH_FN(quantized_lstm_cell_dynamic));
  m.impl(TORCH_SELECTIVE_NAME("quantized::quantized_gru_cell_dynamic"), TORCH_FN(quantized_gru_cell_dynamic));
  m.impl(TORCH_SELECTIVE_NAME("quantized::quantized_rnn_relu_cell_dynamic"), TORCH_FN(quantized_rnn_relu_cell_dynamic));
  m.impl(TORCH_SELECTIVE_NAME("quantized::quantized_rnn_tanh_cell_dynamic"), TORCH_FN(quantized_rnn_tanh_cell_dynamic));
}

TORCH_LIBRARY_IMPL(quantized, CatchAll, m) {
  m.impl(TORCH_SELECTIVE_NAME("quantized::make_quantized_cell_params_fp16"), TORCH_FN(make_quantized_cell_params_fp16));
}

} // namespace
}  // namespace pluss::native
