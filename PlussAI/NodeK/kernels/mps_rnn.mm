#include "NodeK/kernels/mps_rnn.h"

#if defined(__APPLE__)

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesTape.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSRuntimeMM.h"

#import <Metal/Metal.h>
#import <MetalPerformanceShadersGraph/MPSGraph.h>
#import <MetalPerformanceShadersGraph/MPSGraphMemoryOps.h>
#import <MetalPerformanceShadersGraph/MPSGraphRNNOps.h>
#import <MetalPerformanceShadersGraph/MPSGraphTensorData.h>
#import <MetalPerformanceShadersGraph/MPSGraphTensorShapeOps.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

namespace pluss::ai::kernels::mps {
namespace {

struct Context {
  bool ready{false};
  bool has_single_gate_rnn{false};
  bool has_gru{false};
  bool has_lstm{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  std::string error{};
};

struct RnnBackwardResult {
  Series grad_source{};
  Series grad_recurrent{};
  Series grad_input{};
  Series grad_bias{};
  Series grad_hx0{};
};

struct LstmBackwardResult {
  Series grad_source{};
  Series grad_recurrent{};
  Series grad_bias{};
  Series grad_hx0{};
  Series grad_cx0{};
};

struct LstmLayerResult {
  Series output{};
  Series hidden{};
  Series cell{};
};

struct RnnLayerResult {
  Series output{};
  Series hidden{};
};

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) return false;
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* key, bool fallback) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return fallback;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return fallback;
}

Context& ctx_() {
  static Context ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = ::pluss::codem::native::mps::shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS LSTM init failed: ";
        ctx.error += ::pluss::codem::native::mps::runtime_error_cstr();
        return;
      }
      ctx.queue = ::pluss::codem::native::mps::shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS LSTM init failed: no shared Metal command queue";
        return;
      }
      ctx.has_single_gate_rnn = [MPSGraph instancesRespondToSelector:@selector(singleGateRNNWithSourceTensor:recurrentWeight:inputWeight:bias:initState:descriptor:name:)] &&
          [MPSGraph instancesRespondToSelector:@selector(singleGateRNNGradientsWithSourceTensor:recurrentWeight:sourceGradient:zState:stateGradient:inputWeight:bias:initState:mask:descriptor:name:)];
      ctx.has_gru = [MPSGraph instancesRespondToSelector:@selector(GRUWithSourceTensor:recurrentWeight:inputWeight:bias:initState:descriptor:name:)] &&
          [MPSGraph instancesRespondToSelector:@selector(GRUGradientsWithSourceTensor:recurrentWeight:sourceGradient:zState:outputFwd:stateGradient:inputWeight:bias:initState:mask:secondaryBias:descriptor:name:)];
      ctx.has_lstm = [MPSGraph instancesRespondToSelector:@selector(LSTMWithSourceTensor:recurrentWeight:inputWeight:bias:initState:initCell:descriptor:name:)] &&
          [MPSGraph instancesRespondToSelector:@selector(LSTMGradientsWithSourceTensor:recurrentWeight:sourceGradient:zState:cellOutputFwd:stateGradient:cellGradient:inputWeight:bias:initState:initCell:mask:peephole:descriptor:name:)];
      ctx.ready = true;
    }
  });
  return ctx;
}

void set_reason_(std::string* reason, std::string msg) noexcept {
  if (reason) *reason = std::move(msg);
}

std::string shape_string_(const Series& s) {
  std::string out = "[";
  for (std::size_t i = 0; i < s.sizes().size(); ++i) {
    if (i) out += ",";
    out += std::to_string(s.sizes()[i]);
  }
  out += "]";
  return out;
}

bool is_zero_offset_contiguous_f32_mps_(const Series& s) noexcept {
  return s.defined() &&
      s.platform() == Platform::MPS &&
      s.scalar_type() == ScalarType::Float32 &&
      s.is_contiguous() &&
      s.storage_offset() == 0;
}

bool validate_rnn_(const Series& input,
                   const Series& hx,
                   pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
                   const ::pluss::ai::rnn::NetConfig& cfg,
                   bool require_backward,
                   std::string* reason) noexcept {
  if (!rnn_route_enabled()) {
    set_reason_(reason, "MPS RNN route is disabled");
    return false;
  }
  auto& c = ctx_();
  if (!c.ready) {
    set_reason_(reason, c.error);
    return false;
  }
  if (!c.has_single_gate_rnn) {
    set_reason_(reason, "MPSGraph single-gate RNN operator is unavailable on this OS");
    return false;
  }
  (void)require_backward;
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t total_layers = cfg.num_layers * dirs;
  if (params.size() != static_cast<std::size_t>(total_layers)) {
    set_reason_(reason, "native MPS RNN route expects one parameter block per layer/direction");
    return false;
  }
  if (!is_zero_offset_contiguous_f32_mps_(input) || !is_zero_offset_contiguous_f32_mps_(hx)) {
    set_reason_(reason, "native MPS RNN route expects zero-offset contiguous float32 MPS input/state tensors");
    return false;
  }
  if (input.dim() != 3 || hx.dim() != 3) {
    set_reason_(reason, "native MPS RNN route expects input [time,batch,input], hx [layers,batch,hidden]");
    return false;
  }
  if (hx.sizes()[0] != total_layers || hx.sizes()[1] != input.sizes()[1]) {
    set_reason_(reason, "native MPS RNN route state shape mismatch: hx=" + shape_string_(hx));
    return false;
  }
  if (cfg.nonlinearity != "tanh" && cfg.nonlinearity != "relu") {
    set_reason_(reason, "native MPS RNN route supports tanh/relu nonlinearity only");
    return false;
  }
  std::int64_t expected_hidden = -1;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input.sizes()[2] : expected_hidden * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::size_t i = static_cast<std::size_t>(layer * dirs + dir);
      const auto& p = params[i];
      if (!is_zero_offset_contiguous_f32_mps_(p.weight_ih) ||
          !is_zero_offset_contiguous_f32_mps_(p.weight_hh)) {
        set_reason_(reason, "native MPS RNN route expects zero-offset contiguous float32 MPS weights");
        return false;
      }
      const std::int64_t hidden = p.weight_hh.sizes()[0];
      if (expected_hidden < 0) expected_hidden = hidden;
      if (hidden != expected_hidden ||
          p.weight_hh.sizes()[1] != hidden ||
          p.weight_ih.sizes()[0] != hidden ||
          p.weight_ih.sizes()[1] != layer_input ||
          hx.sizes()[2] != hidden) {
        set_reason_(reason, "native MPS RNN route shape mismatch at layer " + std::to_string(i));
        return false;
      }
      if (cfg.split_bias) {
        if (!is_zero_offset_contiguous_f32_mps_(p.bias_ih) || !is_zero_offset_contiguous_f32_mps_(p.bias_hh)) {
          set_reason_(reason, "native MPS RNN route expects zero-offset contiguous float32 split biases");
          return false;
        }
      } else if (cfg.use_bias) {
        if (!is_zero_offset_contiguous_f32_mps_(p.bias)) {
          set_reason_(reason, "native MPS RNN route expects a zero-offset contiguous float32 unified bias");
          return false;
        }
      }
      if (cfg.use_bias) {
        const bool bias_ok = cfg.split_bias
            ? (p.bias_ih.dim() == 1 && p.bias_ih.sizes()[0] == hidden && p.bias_hh.dim() == 1 && p.bias_hh.sizes()[0] == hidden)
            : (p.bias.dim() == 1 && p.bias.sizes()[0] == hidden);
        if (!bias_ok) {
          set_reason_(reason, "native MPS RNN route bias shape mismatch");
          return false;
        }
      }
    }
  }
  return true;
}

bool validate_gru_(const Series& input,
                   const Series& hx,
                   pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
                   const ::pluss::ai::rnn::NetConfig& cfg,
                   bool require_backward,
                   std::string* reason) noexcept {
  if (!gru_route_enabled()) {
    set_reason_(reason, "MPS GRU route is disabled");
    return false;
  }
  auto& c = ctx_();
  if (!c.ready) {
    set_reason_(reason, c.error);
    return false;
  }
  if (!c.has_gru) {
    set_reason_(reason, "MPSGraph GRU operator is unavailable on this OS");
    return false;
  }
  (void)require_backward;
  if (cfg.split_bias) {
    set_reason_(reason, "native MPS GRU route currently supports unified-bias layout only");
    return false;
  }
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t total_layers = cfg.num_layers * dirs;
  if (params.size() != static_cast<std::size_t>(total_layers)) {
    set_reason_(reason, "native MPS GRU route expects one parameter block per layer/direction");
    return false;
  }
  if (!is_zero_offset_contiguous_f32_mps_(input) || !is_zero_offset_contiguous_f32_mps_(hx)) {
    set_reason_(reason, "native MPS GRU route expects zero-offset contiguous float32 MPS input/state tensors");
    return false;
  }
  if (input.dim() != 3 || hx.dim() != 3) {
    set_reason_(reason, "native MPS GRU route expects input [time,batch,input], hx [layers,batch,hidden]");
    return false;
  }
  if (hx.sizes()[0] != total_layers || hx.sizes()[1] != input.sizes()[1]) {
    set_reason_(reason, "native MPS GRU route state shape mismatch: hx=" + shape_string_(hx));
    return false;
  }
  std::int64_t expected_hidden = -1;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input.sizes()[2] : expected_hidden * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::size_t i = static_cast<std::size_t>(layer * dirs + dir);
      const auto& p = params[i];
      if (!is_zero_offset_contiguous_f32_mps_(p.weight_ih) ||
          !is_zero_offset_contiguous_f32_mps_(p.weight_hh)) {
        set_reason_(reason, "native MPS GRU route expects zero-offset contiguous float32 MPS weights");
        return false;
      }
      const std::int64_t hidden = p.weight_hh.sizes()[1];
      const std::int64_t gate = hidden * 3;
      if (expected_hidden < 0) expected_hidden = hidden;
      if (hidden != expected_hidden ||
          p.weight_hh.sizes()[0] != gate ||
          p.weight_ih.sizes()[0] != gate ||
          p.weight_ih.sizes()[1] != layer_input ||
          hx.sizes()[2] != hidden) {
        set_reason_(reason, "native MPS GRU route shape mismatch at layer " + std::to_string(i));
        return false;
      }
      if (cfg.use_bias) {
        if (!is_zero_offset_contiguous_f32_mps_(p.bias) || p.bias.dim() != 1 || p.bias.sizes()[0] != gate) {
          set_reason_(reason, "native MPS GRU route expects zero-offset contiguous float32 unified bias");
          return false;
        }
      }
    }
  }
  return true;
}

bool validate_lstm_(const Series& input,
                    const Series& hx,
                    const Series& cx,
                    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
                    const ::pluss::ai::rnn::NetConfig& cfg,
                    bool require_backward,
                    std::string* reason) noexcept {
  if (!lstm_route_enabled()) {
    set_reason_(reason, "MPS LSTM route is disabled");
    return false;
  }
  if (!ctx_().ready) {
    set_reason_(reason, ctx_().error);
    return false;
  }
  if (!ctx_().has_lstm) {
    set_reason_(reason, "MPSGraph LSTM operator is unavailable on this OS");
    return false;
  }
  (void)require_backward;
  if (cfg.proj_size != 0) {
    set_reason_(reason, "native MPS LSTM route does not support projections");
    return false;
  }
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t total_layers = cfg.num_layers * dirs;
  if (params.size() != static_cast<std::size_t>(total_layers)) {
    set_reason_(reason, "native MPS LSTM route expects one parameter block per layer/direction");
    return false;
  }
  if (!is_zero_offset_contiguous_f32_mps_(input) ||
      !is_zero_offset_contiguous_f32_mps_(hx) ||
      !is_zero_offset_contiguous_f32_mps_(cx)) {
    set_reason_(reason, "native MPS LSTM route expects zero-offset contiguous float32 MPS input/state tensors");
    return false;
  }
  if (input.dim() != 3 || hx.dim() != 3 || cx.dim() != 3) {
    set_reason_(reason, "native MPS LSTM route expects input [time,batch,input], hx/cx [layers,batch,hidden]");
    return false;
  }
  if (hx.sizes()[0] != total_layers ||
      cx.sizes()[0] != total_layers ||
      hx.sizes()[1] != input.sizes()[1] ||
      cx.sizes()[1] != input.sizes()[1]) {
    set_reason_(reason, "native MPS LSTM route state shape mismatch: hx=" + shape_string_(hx));
    return false;
  }
  std::int64_t expected_hidden = -1;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input.sizes()[2] : expected_hidden * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::size_t i = static_cast<std::size_t>(layer * dirs + dir);
      const auto& p = params[i];
      if (!is_zero_offset_contiguous_f32_mps_(p.weight_ih) ||
          !is_zero_offset_contiguous_f32_mps_(p.weight_hh)) {
        set_reason_(reason, "native MPS LSTM route expects zero-offset contiguous float32 MPS weights");
        return false;
      }
      const std::int64_t hidden = p.weight_hh.sizes()[1];
      const std::int64_t gate = hidden * 4;
      if (expected_hidden < 0) expected_hidden = hidden;
      if (hidden != expected_hidden ||
          p.weight_hh.sizes()[0] != gate ||
          p.weight_ih.sizes()[0] != gate ||
          p.weight_ih.sizes()[1] != layer_input ||
          hx.sizes()[2] != hidden ||
          cx.sizes()[2] != hidden) {
        set_reason_(reason, "native MPS LSTM route shape mismatch at layer " + std::to_string(i));
        return false;
      }
      if (cfg.split_bias) {
        if (!is_zero_offset_contiguous_f32_mps_(p.bias_ih) || !is_zero_offset_contiguous_f32_mps_(p.bias_hh)) {
          set_reason_(reason, "native MPS LSTM route expects zero-offset contiguous float32 split biases");
          return false;
        }
      } else if (cfg.use_bias) {
        if (!is_zero_offset_contiguous_f32_mps_(p.bias)) {
          set_reason_(reason, "native MPS LSTM route expects a zero-offset contiguous float32 unified bias");
          return false;
        }
      }
      if (cfg.use_bias) {
        const Series& b = cfg.split_bias ? p.bias_ih : p.bias;
        if (b.dim() != 1 || b.sizes()[0] != gate) {
          set_reason_(reason, "native MPS LSTM route bias shape mismatch");
          return false;
        }
      }
    }
  }
  return true;
}

NSUInteger byte_count_(const Series& s) {
  return static_cast<NSUInteger>(s.numel() * static_cast<std::int64_t>(sizeof(float)));
}

MPSShape* mps_shape_(pluss::msharp::RangeView::Int sizes) {
  NSMutableArray<NSNumber*>* shape = [NSMutableArray arrayWithCapacity:sizes.size()];
  for (const auto d : sizes) {
    [shape addObject:@(static_cast<NSInteger>(d))];
  }
  return shape;
}

MPSGraphTensorData* tensor_data_for_read_(const Series& s, MPSShape* shape, id<MTLDevice> device) {
  id<MTLBuffer> buf = ::pluss::codem::native::mps::ensure_device_buffer_for_read_mm(s, device, byte_count_(s));
  if (buf == nil) {
    ::pluss::throw_error("mps::lstm: failed to acquire input Metal buffer");
  }
  return [[MPSGraphTensorData alloc] initWithMTLBuffer:buf shape:shape dataType:MPSDataTypeFloat32];
}

MPSGraphTensorData* tensor_data_for_write_(const Series& s, MPSShape* shape, id<MTLDevice> device) {
  id<MTLBuffer> buf = ::pluss::codem::native::mps::ensure_device_buffer_for_write_mm(s, device, byte_count_(s));
  if (buf == nil) {
    ::pluss::throw_error("mps::lstm: failed to acquire output Metal buffer");
  }
  return [[MPSGraphTensorData alloc] initWithMTLBuffer:buf shape:shape dataType:MPSDataTypeFloat32];
}

Series zeros_like_(const Series& s) {
  return Series::zeros(s.sizes(), s.scalar_type()).to(s.platform()).contiguous();
}

RnnBackwardResult run_single_gate_rnn_backward_(const Series& source,
                                                const Series& recurrent_weight,
                                                const Series& input_weight,
                                                const Series& bias,
                                                const Series& hx0,
                                                const Series& z_state,
                                                const Series& grad_y,
                                                const Series& grad_hy0,
                                                const ::pluss::ai::rnn::NetConfig& cfg) {
  auto& c = ctx_();
  if (!c.ready || !c.has_single_gate_rnn) {
    ::pluss::throw_error("mps::rnn_backward: backend unavailable: " + c.error);
  }

  RnnBackwardResult result{};
  result.grad_source = Series::empty(source.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_recurrent = Series::empty(recurrent_weight.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_input = Series::empty(input_weight.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  if (bias.defined()) {
    result.grad_bias = Series::empty(bias.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  }
  result.grad_hx0 = Series::empty(hx0.sizes(), ScalarType::Float32, Platform::MPS).contiguous();

  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* source_shape = mps_shape_(source.sizes());
    MPSShape* recurrent_shape = mps_shape_(recurrent_weight.sizes());
    MPSShape* input_weight_shape = mps_shape_(input_weight.sizes());
    MPSShape* state_shape = mps_shape_(hx0.sizes());
    MPSShape* z_shape = mps_shape_(z_state.sizes());
    MPSShape* bias_shape = bias.defined() ? mps_shape_(bias.sizes()) : nil;

    MPSGraphTensor* source_t = [graph placeholderWithShape:source_shape dataType:MPSDataTypeFloat32 name:@"source"];
    MPSGraphTensor* recurrent_t =
        [graph placeholderWithShape:recurrent_shape dataType:MPSDataTypeFloat32 name:@"recurrent_weight"];
    MPSGraphTensor* input_weight_t =
        [graph placeholderWithShape:input_weight_shape dataType:MPSDataTypeFloat32 name:@"input_weight"];
    MPSGraphTensor* grad_y_t = [graph placeholderWithShape:z_shape dataType:MPSDataTypeFloat32 name:@"grad_y"];
    MPSGraphTensor* z_t = [graph placeholderWithShape:z_shape dataType:MPSDataTypeFloat32 name:@"z_state"];
    MPSGraphTensor* grad_hy_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"grad_hy"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* bias_t = nil;
    if (bias.defined()) {
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphSingleGateRNNDescriptor* desc = [MPSGraphSingleGateRNNDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.reverse = false;
    desc.activation = (cfg.nonlinearity == "relu") ? MPSGraphRNNActivationRelu : MPSGraphRNNActivationTanh;

    NSArray<MPSGraphTensor*>* grads = [graph singleGateRNNGradientsWithSourceTensor:source_t
                                                                    recurrentWeight:recurrent_t
                                                                     sourceGradient:grad_y_t
                                                                             zState:z_t
                                                                      stateGradient:grad_hy_t
                                                                        inputWeight:input_weight_t
                                                                               bias:bias_t
                                                                          initState:hx_t
                                                                               mask:nil
                                                                         descriptor:desc
                                                                               name:@"pluss_rnn_backward"];
    if (grads == nil || [grads count] < (bias.defined() ? 5u : 4u)) {
      ::pluss::throw_error("mps::rnn_backward: MPSGraph returned an unexpected gradient set");
    }
    NSUInteger idx = 0;
    MPSGraphTensor* grad_source_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_recurrent_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_input_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_bias_t = nil;
    if (bias.defined()) {
      grad_bias_t = [grads objectAtIndex:idx++];
    }
    MPSGraphTensor* grad_hx_t = [grads objectAtIndex:idx++];

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      source_t : tensor_data_for_read_(source, source_shape, c.device),
      recurrent_t : tensor_data_for_read_(recurrent_weight, recurrent_shape, c.device),
      input_weight_t : tensor_data_for_read_(input_weight, input_weight_shape, c.device),
      grad_y_t : tensor_data_for_read_(grad_y, z_shape, c.device),
      z_t : tensor_data_for_read_(z_state, z_shape, c.device),
      grad_hy_t : tensor_data_for_read_(grad_hy0, state_shape, c.device),
      hx_t : tensor_data_for_read_(hx0, state_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [feeds setObject:tensor_data_for_read_(bias, bias_shape, c.device) forKey:bias_t];
    }

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* outputs = [@{
      grad_source_t : tensor_data_for_write_(result.grad_source, source_shape, c.device),
      grad_recurrent_t : tensor_data_for_write_(result.grad_recurrent, recurrent_shape, c.device),
      grad_input_t : tensor_data_for_write_(result.grad_input, input_weight_shape, c.device),
      grad_hx_t : tensor_data_for_write_(result.grad_hx0, state_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [outputs setObject:tensor_data_for_write_(result.grad_bias, bias_shape, c.device) forKey:grad_bias_t];
    }

    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:outputs];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_source, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_recurrent, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_input, nullptr);
  if (result.grad_bias.defined()) ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_bias, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_hx0, nullptr);
  return result;
}

RnnBackwardResult run_gru_backward_(const Series& source,
                                    const Series& recurrent_weight,
                                    const Series& input_weight,
                                    const Series& bias,
                                    const Series& hx0,
                                    const Series& z_state,
                                    const Series& output_fwd,
                                    const Series& grad_y,
                                    const Series& grad_hy0) {
  auto& c = ctx_();
  if (!c.ready || !c.has_gru) {
    ::pluss::throw_error("mps::gru_backward: backend unavailable: " + c.error);
  }

  RnnBackwardResult result{};
  result.grad_source = Series::empty(source.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_recurrent = Series::empty(recurrent_weight.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_input = Series::empty(input_weight.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  if (bias.defined()) {
    result.grad_bias = Series::empty(bias.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  }
  result.grad_hx0 = Series::empty(hx0.sizes(), ScalarType::Float32, Platform::MPS).contiguous();

  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* source_shape = mps_shape_(source.sizes());
    MPSShape* recurrent_shape = mps_shape_(recurrent_weight.sizes());
    MPSShape* input_weight_shape = mps_shape_(input_weight.sizes());
    MPSShape* state_shape = mps_shape_(hx0.sizes());
    MPSShape* z_shape = mps_shape_(z_state.sizes());
    MPSShape* output_shape = mps_shape_(output_fwd.sizes());
    MPSShape* bias_shape = bias.defined() ? mps_shape_(bias.sizes()) : nil;

    MPSGraphTensor* source_t = [graph placeholderWithShape:source_shape dataType:MPSDataTypeFloat32 name:@"source"];
    MPSGraphTensor* recurrent_t =
        [graph placeholderWithShape:recurrent_shape dataType:MPSDataTypeFloat32 name:@"recurrent_weight"];
    MPSGraphTensor* input_weight_t =
        [graph placeholderWithShape:input_weight_shape dataType:MPSDataTypeFloat32 name:@"input_weight"];
    MPSGraphTensor* grad_y_t = [graph placeholderWithShape:output_shape dataType:MPSDataTypeFloat32 name:@"grad_y"];
    MPSGraphTensor* z_t = [graph placeholderWithShape:z_shape dataType:MPSDataTypeFloat32 name:@"z_state"];
    MPSGraphTensor* out_t = [graph placeholderWithShape:output_shape dataType:MPSDataTypeFloat32 name:@"output_fwd"];
    MPSGraphTensor* grad_hy_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"grad_hy"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* bias_t = nil;
    if (bias.defined()) {
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphGRUDescriptor* desc = [MPSGraphGRUDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.reverse = false;
    desc.resetGateFirst = true;
    desc.resetAfter = false;
    desc.flipZ = false;

    NSArray<MPSGraphTensor*>* grads = [graph GRUGradientsWithSourceTensor:source_t
                                                           recurrentWeight:recurrent_t
                                                            sourceGradient:grad_y_t
                                                                    zState:z_t
                                                                 outputFwd:out_t
                                                             stateGradient:grad_hy_t
                                                               inputWeight:input_weight_t
                                                                      bias:bias_t
                                                                 initState:hx_t
                                                                      mask:nil
                                                             secondaryBias:nil
                                                                descriptor:desc
                                                                      name:@"pluss_gru_backward"];
    if (grads == nil || [grads count] < (bias.defined() ? 5u : 4u)) {
      ::pluss::throw_error("mps::gru_backward: MPSGraph returned an unexpected gradient set");
    }
    NSUInteger idx = 0;
    MPSGraphTensor* grad_source_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_recurrent_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_input_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_bias_t = nil;
    if (bias.defined()) {
      grad_bias_t = [grads objectAtIndex:idx++];
    }
    MPSGraphTensor* grad_hx_t = [grads objectAtIndex:idx++];

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      source_t : tensor_data_for_read_(source, source_shape, c.device),
      recurrent_t : tensor_data_for_read_(recurrent_weight, recurrent_shape, c.device),
      input_weight_t : tensor_data_for_read_(input_weight, input_weight_shape, c.device),
      grad_y_t : tensor_data_for_read_(grad_y, output_shape, c.device),
      z_t : tensor_data_for_read_(z_state, z_shape, c.device),
      out_t : tensor_data_for_read_(output_fwd, output_shape, c.device),
      grad_hy_t : tensor_data_for_read_(grad_hy0, state_shape, c.device),
      hx_t : tensor_data_for_read_(hx0, state_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [feeds setObject:tensor_data_for_read_(bias, bias_shape, c.device) forKey:bias_t];
    }

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* outputs = [@{
      grad_source_t : tensor_data_for_write_(result.grad_source, source_shape, c.device),
      grad_recurrent_t : tensor_data_for_write_(result.grad_recurrent, recurrent_shape, c.device),
      grad_input_t : tensor_data_for_write_(result.grad_input, input_weight_shape, c.device),
      grad_hx_t : tensor_data_for_write_(result.grad_hx0, state_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [outputs setObject:tensor_data_for_write_(result.grad_bias, bias_shape, c.device) forKey:grad_bias_t];
    }

    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:outputs];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_source, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_recurrent, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_input, nullptr);
  if (result.grad_bias.defined()) ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_bias, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_hx0, nullptr);
  return result;
}

LstmBackwardResult run_lstm_backward_(const Series& source,
                                      const Series& recurrent_weight,
                                      const Series& bias,
                                      const Series& hx0,
                                      const Series& cx0,
                                      const Series& z_state,
                                      const Series& cell_state_fwd,
                                      const Series& grad_y,
                                      const Series& grad_hy0,
                                      const Series& grad_cy0) {
  auto& c = ctx_();
  if (!c.ready) {
    ::pluss::throw_error("mps::lstm_backward: backend unavailable: " + c.error);
  }

  LstmBackwardResult result{};
  result.grad_source = Series::empty(source.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_recurrent = Series::empty(recurrent_weight.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  if (bias.defined()) {
    result.grad_bias = Series::empty(bias.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  }
  result.grad_hx0 = Series::empty(hx0.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  result.grad_cx0 = Series::empty(cx0.sizes(), ScalarType::Float32, Platform::MPS).contiguous();

  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* source_shape = mps_shape_(source.sizes());
    MPSShape* recurrent_shape = mps_shape_(recurrent_weight.sizes());
    MPSShape* state_shape = mps_shape_(hx0.sizes());
    MPSShape* z_shape = mps_shape_(z_state.sizes());
    MPSShape* cell_shape = mps_shape_(cell_state_fwd.sizes());
    MPSShape* bias_shape = bias.defined() ? mps_shape_(bias.sizes()) : nil;

    MPSGraphTensor* source_t = [graph placeholderWithShape:source_shape dataType:MPSDataTypeFloat32 name:@"source"];
    MPSGraphTensor* recurrent_t =
        [graph placeholderWithShape:recurrent_shape dataType:MPSDataTypeFloat32 name:@"recurrent_weight"];
    MPSGraphTensor* grad_y_t = [graph placeholderWithShape:cell_shape dataType:MPSDataTypeFloat32 name:@"grad_y"];
    MPSGraphTensor* z_t = [graph placeholderWithShape:z_shape dataType:MPSDataTypeFloat32 name:@"z_state"];
    MPSGraphTensor* cell_t =
        [graph placeholderWithShape:cell_shape dataType:MPSDataTypeFloat32 name:@"cell_state_fwd"];
    MPSGraphTensor* grad_hy_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"grad_hy"];
    MPSGraphTensor* grad_cy_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"grad_cy"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* cx_t = [graph placeholderWithShape:state_shape dataType:MPSDataTypeFloat32 name:@"cx"];
    MPSGraphTensor* bias_t = nil;
    if (bias.defined()) {
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphLSTMDescriptor* desc = [MPSGraphLSTMDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.produceCell = true;

    NSArray<MPSGraphTensor*>* grads = [graph LSTMGradientsWithSourceTensor:source_t
                                                           recurrentWeight:recurrent_t
                                                            sourceGradient:grad_y_t
                                                                    zState:z_t
                                                             cellOutputFwd:cell_t
                                                            stateGradient:grad_hy_t
                                                             cellGradient:grad_cy_t
                                                               inputWeight:nil
                                                                      bias:bias_t
                                                                 initState:hx_t
                                                                  initCell:cx_t
                                                                      mask:nil
                                                                  peephole:nil
                                                                descriptor:desc
                                                                      name:@"pluss_lstm_backward"];
    if (grads == nil || [grads count] < (bias.defined() ? 5u : 4u)) {
      ::pluss::throw_error("mps::lstm_backward: MPSGraph returned an unexpected gradient set");
    }
    NSUInteger idx = 0;
    MPSGraphTensor* grad_source_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_recurrent_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_bias_t = nil;
    if (bias.defined()) {
      grad_bias_t = [grads objectAtIndex:idx++];
    }
    MPSGraphTensor* grad_hx_t = [grads objectAtIndex:idx++];
    MPSGraphTensor* grad_cx_t = [grads objectAtIndex:idx++];

    MPSGraphTensorData* source_d = tensor_data_for_read_(source, source_shape, c.device);
    MPSGraphTensorData* recurrent_d = tensor_data_for_read_(recurrent_weight, recurrent_shape, c.device);
    MPSGraphTensorData* grad_y_d = tensor_data_for_read_(grad_y, cell_shape, c.device);
    MPSGraphTensorData* z_d = tensor_data_for_read_(z_state, z_shape, c.device);
    MPSGraphTensorData* cell_d = tensor_data_for_read_(cell_state_fwd, cell_shape, c.device);
    MPSGraphTensorData* grad_hy_d = tensor_data_for_read_(grad_hy0, state_shape, c.device);
    MPSGraphTensorData* grad_cy_d = tensor_data_for_read_(grad_cy0, state_shape, c.device);
    MPSGraphTensorData* hx_d = tensor_data_for_read_(hx0, state_shape, c.device);
    MPSGraphTensorData* cx_d = tensor_data_for_read_(cx0, state_shape, c.device);

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      source_t : source_d,
      recurrent_t : recurrent_d,
      grad_y_t : grad_y_d,
      z_t : z_d,
      cell_t : cell_d,
      grad_hy_t : grad_hy_d,
      grad_cy_t : grad_cy_d,
      hx_t : hx_d,
      cx_t : cx_d,
    } mutableCopy];
    if (bias.defined()) {
      MPSGraphTensorData* bias_d = tensor_data_for_read_(bias, bias_shape, c.device);
      [feeds setObject:bias_d forKey:bias_t];
    }

    MPSGraphTensorData* grad_source_d = tensor_data_for_write_(result.grad_source, source_shape, c.device);
    MPSGraphTensorData* grad_recurrent_d = tensor_data_for_write_(result.grad_recurrent, recurrent_shape, c.device);
    MPSGraphTensorData* grad_hx_d = tensor_data_for_write_(result.grad_hx0, state_shape, c.device);
    MPSGraphTensorData* grad_cx_d = tensor_data_for_write_(result.grad_cx0, state_shape, c.device);
    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* outputs = [@{
      grad_source_t : grad_source_d,
      grad_recurrent_t : grad_recurrent_d,
      grad_hx_t : grad_hx_d,
      grad_cx_t : grad_cx_d,
    } mutableCopy];
    if (bias.defined()) {
      MPSGraphTensorData* grad_bias_d = tensor_data_for_write_(result.grad_bias, bias_shape, c.device);
      [outputs setObject:grad_bias_d forKey:grad_bias_t];
    }

    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:outputs];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_source, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_recurrent, nullptr);
  if (result.grad_bias.defined()) ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_bias, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_hx0, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(result.grad_cx0, nullptr);
  return result;
}

RnnLayerResult run_single_gate_rnn_layer_(const Series& input,
                                          const Series& hx0,
                                          const ::pluss::ai::rnn::NetWeight& p,
                                          const ::pluss::ai::rnn::NetConfig& cfg) {
  Series bias = Series::undefined();
  if (cfg.use_bias) {
    bias = cfg.split_bias ? p.bias_ih.add(p.bias_hh).contiguous() : p.bias;
  }

  const std::int64_t hidden_size = p.weight_hh.sizes()[0];
  Series out = Series::empty({input.sizes()[0], input.sizes()[1], hidden_size}, ScalarType::Float32, Platform::MPS)
                   .contiguous();
  Series z_state = Series::empty(out.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  Series hy = Series::empty({1, hx0.sizes()[0], hx0.sizes()[1]}, ScalarType::Float32, Platform::MPS).contiguous();

  auto& c = ctx_();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* input_shape = mps_shape_(input.sizes());
    MPSShape* output_shape = mps_shape_(out.sizes());
    MPSShape* hx0_shape = mps_shape_(hx0.sizes());
    MPSShape* state_out_shape = mps_shape_(hy.sizes());
    MPSShape* weight_hh_shape = mps_shape_(p.weight_hh.sizes());
    MPSShape* weight_ih_shape = mps_shape_(p.weight_ih.sizes());

    MPSGraphTensor* input_t = [graph placeholderWithShape:input_shape dataType:MPSDataTypeFloat32 name:@"input"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:hx0_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* whh_t = [graph placeholderWithShape:weight_hh_shape dataType:MPSDataTypeFloat32 name:@"weight_hh"];
    MPSGraphTensor* wih_t = [graph placeholderWithShape:weight_ih_shape dataType:MPSDataTypeFloat32 name:@"weight_ih"];
    MPSGraphTensor* bias_t = nil;
    MPSShape* bias_shape = nil;
    if (bias.defined()) {
      bias_shape = mps_shape_(bias.sizes());
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphSingleGateRNNDescriptor* desc = [MPSGraphSingleGateRNNDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.reverse = false;
    desc.activation = (cfg.nonlinearity == "relu") ? MPSGraphRNNActivationRelu : MPSGraphRNNActivationTanh;

    NSArray<MPSGraphTensor*>* outputs = [graph singleGateRNNWithSourceTensor:input_t
                                                             recurrentWeight:whh_t
                                                                 inputWeight:wih_t
                                                                        bias:bias_t
                                                                   initState:hx_t
                                                                  descriptor:desc
                                                                        name:@"pluss_rnn"];
    if (outputs == nil || [outputs count] < 2) {
      ::pluss::throw_error("mps::rnn: MPSGraph did not return RNN value/training tensors");
    }
    MPSGraphTensor* out_t = [outputs objectAtIndex:0];
    MPSGraphTensor* z_t = [outputs objectAtIndex:1];
    MPSGraphTensor* hy_t = [graph sliceTensor:out_t dimension:0 start:-1 length:1 name:@"hy"];

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      input_t : tensor_data_for_read_(input, input_shape, c.device),
      hx_t : tensor_data_for_read_(hx0, hx0_shape, c.device),
      whh_t : tensor_data_for_read_(p.weight_hh, weight_hh_shape, c.device),
      wih_t : tensor_data_for_read_(p.weight_ih, weight_ih_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [feeds setObject:tensor_data_for_read_(bias, bias_shape, c.device) forKey:bias_t];
    }

    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      out_t : tensor_data_for_write_(out, output_shape, c.device),
      z_t : tensor_data_for_write_(z_state, output_shape, c.device),
      hy_t : tensor_data_for_write_(hy, state_out_shape, c.device),
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(out, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(z_state, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(hy, nullptr);

  const bool should_link =
      ::pluss::codem::series_tape::recording_enabled() &&
      (::pluss::codem::series_tape::tracks_slope(input) ||
       ::pluss::codem::series_tape::tracks_slope(p.weight_ih) ||
       ::pluss::codem::series_tape::tracks_slope(p.weight_hh) ||
       (bias.defined() && ::pluss::codem::series_tape::tracks_slope(bias)) ||
       ::pluss::codem::series_tape::tracks_slope(hx0));
  if (should_link) {
    auto bind = [&](const Series& target, const char* op_name, int lane) {
      auto link = std::make_shared<::pluss::codem::series_tape::SourceLink>();
      link->op_name = op_name;
      link->parents = {input, p.weight_hh, p.weight_ih, hx0};
      if (bias.defined()) link->parents.push_back(bias);
      link->propagate = [input, recurrent = p.weight_hh, input_weight = p.weight_ih, bias, hx0, z_state, out, cfg, lane](
                            const Series& sens) {
        Series grad_y = zeros_like_(out);
        Series grad_hy = zeros_like_(hx0);
        if (lane == 0) {
          grad_y = sens.contiguous();
        } else {
          grad_hy = sens.select(0, 0).contiguous();
        }
        const RnnBackwardResult grads =
            run_single_gate_rnn_backward_(input, recurrent, input_weight, bias, hx0, z_state, grad_y, grad_hy, cfg);
        ::pluss::codem::series_tape::accumulate_sensitivity(input, grads.grad_source);
        ::pluss::codem::series_tape::accumulate_sensitivity(recurrent, grads.grad_recurrent);
        ::pluss::codem::series_tape::accumulate_sensitivity(input_weight, grads.grad_input);
        if (bias.defined() && grads.grad_bias.defined()) {
          ::pluss::codem::series_tape::accumulate_sensitivity(bias, grads.grad_bias);
        }
        ::pluss::codem::series_tape::accumulate_sensitivity(hx0, grads.grad_hx0);
      };
      ::pluss::codem::series_tape::bind_source_link(target, std::move(link));
    };
    bind(out, "mps_rnn", 0);
    bind(hy, "mps_rnn_h", 1);
  }
  return {out, hy};
}

RnnLayerResult run_gru_layer_(const Series& input,
                              const Series& hx0,
                              const ::pluss::ai::rnn::NetWeight& p,
                              bool use_bias) {
  Series bias = use_bias ? p.bias : Series::undefined();

  const std::int64_t hidden_size = p.weight_hh.sizes()[1];
  Series out = Series::empty({input.sizes()[0], input.sizes()[1], hidden_size}, ScalarType::Float32, Platform::MPS)
                   .contiguous();
  Series z_state = Series::empty({input.sizes()[0], input.sizes()[1], 3 * hidden_size}, ScalarType::Float32, Platform::MPS)
                       .contiguous();
  Series hy = Series::empty({1, hx0.sizes()[0], hx0.sizes()[1]}, ScalarType::Float32, Platform::MPS).contiguous();

  auto& c = ctx_();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* input_shape = mps_shape_(input.sizes());
    MPSShape* output_shape = mps_shape_(out.sizes());
    MPSShape* z_shape = mps_shape_(z_state.sizes());
    MPSShape* hx0_shape = mps_shape_(hx0.sizes());
    MPSShape* state_out_shape = mps_shape_(hy.sizes());
    MPSShape* weight_hh_shape = mps_shape_(p.weight_hh.sizes());
    MPSShape* weight_ih_shape = mps_shape_(p.weight_ih.sizes());

    MPSGraphTensor* input_t = [graph placeholderWithShape:input_shape dataType:MPSDataTypeFloat32 name:@"input"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:hx0_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* whh_t = [graph placeholderWithShape:weight_hh_shape dataType:MPSDataTypeFloat32 name:@"weight_hh"];
    MPSGraphTensor* wih_t = [graph placeholderWithShape:weight_ih_shape dataType:MPSDataTypeFloat32 name:@"weight_ih"];
    MPSGraphTensor* bias_t = nil;
    MPSShape* bias_shape = nil;
    if (bias.defined()) {
      bias_shape = mps_shape_(bias.sizes());
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphGRUDescriptor* desc = [MPSGraphGRUDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.reverse = false;
    desc.resetGateFirst = true;
    desc.resetAfter = false;
    desc.flipZ = false;

    NSArray<MPSGraphTensor*>* outputs = [graph GRUWithSourceTensor:input_t
                                                   recurrentWeight:whh_t
                                                       inputWeight:wih_t
                                                              bias:bias_t
                                                         initState:hx_t
                                                        descriptor:desc
                                                              name:@"pluss_gru"];
    if (outputs == nil || [outputs count] < 2) {
      ::pluss::throw_error("mps::gru: MPSGraph did not return GRU value/training tensors");
    }
    MPSGraphTensor* out_t = [outputs objectAtIndex:0];
    MPSGraphTensor* z_t = [outputs objectAtIndex:1];
    MPSGraphTensor* hy_t = [graph sliceTensor:out_t dimension:0 start:-1 length:1 name:@"hy"];

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      input_t : tensor_data_for_read_(input, input_shape, c.device),
      hx_t : tensor_data_for_read_(hx0, hx0_shape, c.device),
      whh_t : tensor_data_for_read_(p.weight_hh, weight_hh_shape, c.device),
      wih_t : tensor_data_for_read_(p.weight_ih, weight_ih_shape, c.device),
    } mutableCopy];
    if (bias.defined()) {
      [feeds setObject:tensor_data_for_read_(bias, bias_shape, c.device) forKey:bias_t];
    }

    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      out_t : tensor_data_for_write_(out, output_shape, c.device),
      z_t : tensor_data_for_write_(z_state, z_shape, c.device),
      hy_t : tensor_data_for_write_(hy, state_out_shape, c.device),
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(out, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(z_state, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(hy, nullptr);

  const bool should_link =
      ::pluss::codem::series_tape::recording_enabled() &&
      (::pluss::codem::series_tape::tracks_slope(input) ||
       ::pluss::codem::series_tape::tracks_slope(p.weight_ih) ||
       ::pluss::codem::series_tape::tracks_slope(p.weight_hh) ||
       (bias.defined() && ::pluss::codem::series_tape::tracks_slope(bias)) ||
       ::pluss::codem::series_tape::tracks_slope(hx0));
  if (should_link) {
    auto bind = [&](const Series& target, const char* op_name, int lane) {
      auto link = std::make_shared<::pluss::codem::series_tape::SourceLink>();
      link->op_name = op_name;
      link->parents = {input, p.weight_hh, p.weight_ih, hx0};
      if (bias.defined()) link->parents.push_back(bias);
      link->propagate = [input, recurrent = p.weight_hh, input_weight = p.weight_ih, bias, hx0, z_state, out, lane](
                            const Series& sens) {
        Series grad_y = zeros_like_(out);
        Series grad_hy = zeros_like_(hx0);
        if (lane == 0) {
          grad_y = sens.contiguous();
        } else {
          grad_hy = sens.select(0, 0).contiguous();
        }
        const RnnBackwardResult grads =
            run_gru_backward_(input, recurrent, input_weight, bias, hx0, z_state, out, grad_y, grad_hy);
        ::pluss::codem::series_tape::accumulate_sensitivity(input, grads.grad_source);
        ::pluss::codem::series_tape::accumulate_sensitivity(recurrent, grads.grad_recurrent);
        ::pluss::codem::series_tape::accumulate_sensitivity(input_weight, grads.grad_input);
        if (bias.defined() && grads.grad_bias.defined()) {
          ::pluss::codem::series_tape::accumulate_sensitivity(bias, grads.grad_bias);
        }
        ::pluss::codem::series_tape::accumulate_sensitivity(hx0, grads.grad_hx0);
      };
      ::pluss::codem::series_tape::bind_source_link(target, std::move(link));
    };
    bind(out, "mps_gru", 0);
    bind(hy, "mps_gru_h", 1);
  }
  return {out, hy};
}

LstmLayerResult run_lstm_layer_(const Series& input,
                                const Series& hx0,
                                const Series& cx0,
                                const ::pluss::ai::rnn::NetWeight& p,
                                bool use_bias,
                                bool split_bias) {
  Series bias = Series::undefined();
  if (use_bias) {
    bias = split_bias ? p.bias_ih.add(p.bias_hh).contiguous() : p.bias;
  }

  const std::int64_t hidden_size = p.weight_hh.sizes()[1];
  Series input_gates = input.reshape({input.sizes()[0] * input.sizes()[1], input.sizes()[2]})
                           .matmul(p.weight_ih.transpose(0, 1).contiguous())
                           .reshape({input.sizes()[0], input.sizes()[1], 4 * hidden_size})
                           .contiguous();
  Series out = Series::empty({input.sizes()[0], input.sizes()[1], hidden_size}, ScalarType::Float32, Platform::MPS)
                   .contiguous();
  Series cell_state_fwd = Series::empty(out.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  Series z_state = Series::empty(input_gates.sizes(), ScalarType::Float32, Platform::MPS).contiguous();
  Series hy = Series::empty({1, hx0.sizes()[0], hx0.sizes()[1]}, ScalarType::Float32, Platform::MPS).contiguous();
  Series cy = Series::empty(hy.sizes(), ScalarType::Float32, Platform::MPS).contiguous();

  auto& c = ctx_();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* input_shape = mps_shape_(input_gates.sizes());
    MPSShape* output_shape = mps_shape_(out.sizes());
    MPSShape* z_shape = mps_shape_(z_state.sizes());
    MPSShape* hx0_shape = mps_shape_(hx0.sizes());
    MPSShape* cx0_shape = mps_shape_(cx0.sizes());
    MPSShape* state_out_shape = mps_shape_(hy.sizes());
    MPSShape* weight_hh_shape = mps_shape_(p.weight_hh.sizes());

    MPSGraphTensor* input_t = [graph placeholderWithShape:input_shape dataType:MPSDataTypeFloat32 name:@"input"];
    MPSGraphTensor* hx_t = [graph placeholderWithShape:hx0_shape dataType:MPSDataTypeFloat32 name:@"hx"];
    MPSGraphTensor* cx_t = [graph placeholderWithShape:cx0_shape dataType:MPSDataTypeFloat32 name:@"cx"];
    MPSGraphTensor* whh_t = [graph placeholderWithShape:weight_hh_shape dataType:MPSDataTypeFloat32 name:@"weight_hh"];
    MPSGraphTensor* bias_t = nil;
    MPSShape* bias_shape = nil;
    if (use_bias) {
      bias_shape = mps_shape_(bias.sizes());
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
    }

    MPSGraphLSTMDescriptor* desc = [MPSGraphLSTMDescriptor descriptor];
    desc.training = true;
    desc.bidirectional = false;
    desc.produceCell = true;

    NSArray<MPSGraphTensor*>* outputs = [graph LSTMWithSourceTensor:input_t
                                                    recurrentWeight:whh_t
                                                        inputWeight:nil
                                                               bias:bias_t
                                                          initState:hx_t
                                                           initCell:cx_t
                                                         descriptor:desc
                                                               name:@"pluss_lstm"];
    if (outputs == nil || [outputs count] < 3) {
      ::pluss::throw_error("mps::lstm: MPSGraph did not return LSTM value/cell/training tensors");
    }
    MPSGraphTensor* out_t = [outputs objectAtIndex:0];
    MPSGraphTensor* cell_seq_t = [outputs objectAtIndex:1];
    MPSGraphTensor* z_t = [outputs objectAtIndex:2];
    MPSGraphTensor* hy_t = [graph sliceTensor:out_t dimension:0 start:-1 length:1 name:@"hy"];
    MPSGraphTensor* cy_t = [graph sliceTensor:cell_seq_t dimension:0 start:-1 length:1 name:@"cy"];

    MPSGraphTensorData* input_d = tensor_data_for_read_(input_gates, input_shape, c.device);
    MPSGraphTensorData* hx_d = tensor_data_for_read_(hx0, hx0_shape, c.device);
    MPSGraphTensorData* cx_d = tensor_data_for_read_(cx0, cx0_shape, c.device);
    MPSGraphTensorData* whh_d = tensor_data_for_read_(p.weight_hh, weight_hh_shape, c.device);
    MPSGraphTensorData* out_d = tensor_data_for_write_(out, output_shape, c.device);
    MPSGraphTensorData* cell_state_fwd_d = tensor_data_for_write_(cell_state_fwd, output_shape, c.device);
    MPSGraphTensorData* z_d = tensor_data_for_write_(z_state, z_shape, c.device);
    MPSGraphTensorData* hy_d = tensor_data_for_write_(hy, state_out_shape, c.device);
    MPSGraphTensorData* cy_d = tensor_data_for_write_(cy, state_out_shape, c.device);

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      input_t : input_d,
      hx_t : hx_d,
      cx_t : cx_d,
      whh_t : whh_d,
    } mutableCopy];
    if (use_bias) {
      MPSGraphTensorData* bias_d = tensor_data_for_read_(bias, bias_shape, c.device);
      [feeds setObject:bias_d forKey:bias_t];
    }

    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      out_t : out_d,
      cell_seq_t : cell_state_fwd_d,
      z_t : z_d,
      hy_t : hy_d,
      cy_t : cy_d,
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }

  ::pluss::codem::native::mps::mark_device_data_dirty(out, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(cell_state_fwd, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(z_state, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(hy, nullptr);
  ::pluss::codem::native::mps::mark_device_data_dirty(cy, nullptr);

  const bool should_link =
      ::pluss::codem::series_tape::recording_enabled() &&
      (::pluss::codem::series_tape::tracks_slope(input_gates) ||
       ::pluss::codem::series_tape::tracks_slope(p.weight_hh) ||
       (bias.defined() && ::pluss::codem::series_tape::tracks_slope(bias)) ||
       ::pluss::codem::series_tape::tracks_slope(hx0) ||
       ::pluss::codem::series_tape::tracks_slope(cx0));
  if (should_link) {
    auto bind = [&](const Series& target, const char* op_name, int lane) {
      auto link = std::make_shared<::pluss::codem::series_tape::SourceLink>();
      link->op_name = op_name;
      link->parents = {input_gates, p.weight_hh, hx0, cx0};
      if (bias.defined()) link->parents.push_back(bias);
      link->propagate = [input_gates, recurrent = p.weight_hh, bias, hx0, cx0, z_state, cell_state_fwd, out, hy, cy, lane](
                            const Series& sens) {
        Series grad_y = zeros_like_(out);
        Series grad_hy = zeros_like_(hx0);
        Series grad_cy = zeros_like_(cx0);
        if (lane == 0) {
          grad_y = sens.contiguous();
        } else if (lane == 1) {
          grad_hy = sens.select(0, 0).contiguous();
        } else {
          grad_cy = sens.select(0, 0).contiguous();
        }
        const LstmBackwardResult grads =
            run_lstm_backward_(input_gates, recurrent, bias, hx0, cx0, z_state, cell_state_fwd, grad_y, grad_hy, grad_cy);
        ::pluss::codem::series_tape::accumulate_sensitivity(input_gates, grads.grad_source);
        ::pluss::codem::series_tape::accumulate_sensitivity(recurrent, grads.grad_recurrent);
        if (bias.defined() && grads.grad_bias.defined()) {
          ::pluss::codem::series_tape::accumulate_sensitivity(bias, grads.grad_bias);
        }
        ::pluss::codem::series_tape::accumulate_sensitivity(hx0, grads.grad_hx0);
        ::pluss::codem::series_tape::accumulate_sensitivity(cx0, grads.grad_cx0);
      };
      ::pluss::codem::series_tape::bind_source_link(target, std::move(link));
    };
    bind(out, "mps_lstm", 0);
    bind(hy, "mps_lstm_h", 1);
    bind(cy, "mps_lstm_c", 2);
  }
  return {out, hy, cy};
}

} // namespace

bool lstm_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_LSTM_ROUTE", true);
}

bool lstm_route_strict() noexcept {
  return parse_bool_env_("PLUSS_MPS_LSTM_ROUTE_STRICT", false);
}

bool lstm_route_ready() noexcept {
  auto& c = ctx_();
  return c.ready && c.has_lstm;
}

bool rnn_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_RNN_ROUTE", true);
}

bool rnn_route_strict() noexcept {
  return parse_bool_env_("PLUSS_MPS_RNN_ROUTE_STRICT", false);
}

bool rnn_route_ready() noexcept {
  auto& c = ctx_();
  return c.ready && c.has_single_gate_rnn;
}

bool gru_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_GRU_ROUTE", true);
}

bool gru_route_strict() noexcept {
  return parse_bool_env_("PLUSS_MPS_GRU_ROUTE_STRICT", false);
}

bool gru_route_ready() noexcept {
  auto& c = ctx_();
  return c.ready && c.has_gru;
}

bool supports_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
  return validate_rnn_(input, hx, params, cfg, require_backward, reason);
}

std::optional<std::tuple<Series, Series>> try_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  std::string reason;
  if (!validate_rnn_(input, hx, params, cfg, require_backward, &reason)) {
    if (rnn_route_strict() && input.defined() && input.platform() == Platform::MPS) {
      ::pluss::throw_error("mps::rnn: native route unavailable: " + reason);
    }
    return std::nullopt;
  }

  Series layer_input = input;
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  std::vector<Series> hidden_layers;
  hidden_layers.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      const Series hx_fwd = hx.select(0, base).contiguous();
      const Series hx_bwd = hx.select(0, base + 1).contiguous();
      const RnnLayerResult fwd =
          run_single_gate_rnn_layer_(layer_input, hx_fwd, params[static_cast<std::size_t>(base)], cfg);
      const RnnLayerResult bwd = run_single_gate_rnn_layer_(
          ::pluss::detail::nn::reverse_time_(layer_input),
          hx_bwd,
          params[static_cast<std::size_t>(base + 1)],
          cfg);
      hidden_layers.push_back(fwd.hidden);
      hidden_layers.push_back(bwd.hidden);
      layer_input = fwd.output.unite(std::vector<Series>{::pluss::detail::nn::reverse_time_(bwd.output)}, 2);
    } else {
      const Series hx0 = hx.select(0, layer).contiguous();
      RnnLayerResult layer_out =
          run_single_gate_rnn_layer_(layer_input, hx0, params[static_cast<std::size_t>(layer)], cfg);
      layer_input = layer_out.output;
      hidden_layers.push_back(layer_out.hidden);
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = ::pluss::detail::nn::dropout_(layer_input, cfg.dropout, cfg.training, cfg.seed + static_cast<std::uint64_t>(layer));
    }
  }

  Series hidden = hidden_layers.front();
  if (hidden_layers.size() > 1) {
    std::vector<Series> tail(hidden_layers.begin() + 1, hidden_layers.end());
    hidden = hidden.unite(pluss::msharp::RangeView::Of<Series>(tail), 0);
  }
  return std::make_tuple(layer_input, hidden);
}

bool supports_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
  return validate_gru_(input, hx, params, cfg, require_backward, reason);
}

std::optional<std::tuple<Series, Series>> try_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  std::string reason;
  if (!validate_gru_(input, hx, params, cfg, require_backward, &reason)) {
    if (gru_route_strict() && input.defined() && input.platform() == Platform::MPS) {
      ::pluss::throw_error("mps::gru: native route unavailable: " + reason);
    }
    return std::nullopt;
  }

  Series layer_input = input;
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  std::vector<Series> hidden_layers;
  hidden_layers.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      const Series hx_fwd = hx.select(0, base).contiguous();
      const Series hx_bwd = hx.select(0, base + 1).contiguous();
      const RnnLayerResult fwd =
          run_gru_layer_(layer_input, hx_fwd, params[static_cast<std::size_t>(base)], cfg.use_bias);
      const RnnLayerResult bwd = run_gru_layer_(
          ::pluss::detail::nn::reverse_time_(layer_input),
          hx_bwd,
          params[static_cast<std::size_t>(base + 1)],
          cfg.use_bias);
      hidden_layers.push_back(fwd.hidden);
      hidden_layers.push_back(bwd.hidden);
      layer_input = fwd.output.unite(std::vector<Series>{::pluss::detail::nn::reverse_time_(bwd.output)}, 2);
    } else {
      const Series hx0 = hx.select(0, layer).contiguous();
      RnnLayerResult layer_out =
          run_gru_layer_(layer_input, hx0, params[static_cast<std::size_t>(layer)], cfg.use_bias);
      layer_input = layer_out.output;
      hidden_layers.push_back(layer_out.hidden);
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = ::pluss::detail::nn::dropout_(layer_input, cfg.dropout, cfg.training, cfg.seed + 101ULL + static_cast<std::uint64_t>(layer));
    }
  }

  Series hidden = hidden_layers.front();
  if (hidden_layers.size() > 1) {
    std::vector<Series> tail(hidden_layers.begin() + 1, hidden_layers.end());
    hidden = hidden.unite(pluss::msharp::RangeView::Of<Series>(tail), 0);
  }
  return std::make_tuple(layer_input, hidden);
}

bool supports_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
  return validate_lstm_(input, hx, cx, params, cfg, require_backward, reason);
}

std::optional<std::tuple<Series, std::tuple<Series, Series>>> try_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  std::string reason;
  if (!validate_lstm_(input, hx, cx, params, cfg, require_backward, &reason)) {
    if (lstm_route_strict() && input.defined() && input.platform() == Platform::MPS) {
      ::pluss::throw_error("mps::lstm: native route unavailable: " + reason);
    }
    return std::nullopt;
  }

  Series layer_input = input;
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  std::vector<Series> hidden_layers;
  std::vector<Series> cell_layers;
  hidden_layers.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  cell_layers.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      const Series hx_fwd = hx.select(0, base).contiguous();
      const Series cx_fwd = cx.select(0, base).contiguous();
      const Series hx_bwd = hx.select(0, base + 1).contiguous();
      const Series cx_bwd = cx.select(0, base + 1).contiguous();
      const LstmLayerResult fwd =
          run_lstm_layer_(layer_input, hx_fwd, cx_fwd, params[static_cast<std::size_t>(base)], cfg.use_bias, cfg.split_bias);
      const LstmLayerResult bwd = run_lstm_layer_(
          ::pluss::detail::nn::reverse_time_(layer_input),
          hx_bwd,
          cx_bwd,
          params[static_cast<std::size_t>(base + 1)],
          cfg.use_bias,
          cfg.split_bias);
      hidden_layers.push_back(fwd.hidden);
      hidden_layers.push_back(bwd.hidden);
      cell_layers.push_back(fwd.cell);
      cell_layers.push_back(bwd.cell);
      layer_input = fwd.output.unite(std::vector<Series>{::pluss::detail::nn::reverse_time_(bwd.output)}, 2);
    } else {
      const Series hx0 = hx.select(0, layer).contiguous();
      const Series cx0 = cx.select(0, layer).contiguous();
      LstmLayerResult layer_out =
          run_lstm_layer_(layer_input, hx0, cx0, params[static_cast<std::size_t>(layer)], cfg.use_bias, cfg.split_bias);
      layer_input = layer_out.output;
      hidden_layers.push_back(layer_out.hidden);
      cell_layers.push_back(layer_out.cell);
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = ::pluss::detail::nn::dropout_(layer_input, cfg.dropout, cfg.training, cfg.seed + 211ULL + static_cast<std::uint64_t>(layer));
    }
  }

  Series hidden = hidden_layers.front();
  Series cell = cell_layers.front();
  if (hidden_layers.size() > 1) {
    std::vector<Series> hidden_tail(hidden_layers.begin() + 1, hidden_layers.end());
    std::vector<Series> cell_tail(cell_layers.begin() + 1, cell_layers.end());
    hidden = hidden.unite(pluss::msharp::RangeView::Of<Series>(hidden_tail), 0);
    cell = cell.unite(pluss::msharp::RangeView::Of<Series>(cell_tail), 0);
  }
  return std::make_tuple(layer_input, std::make_tuple(hidden, cell));
}

} // namespace pluss::ai::kernels::mps

#endif
