#pragma once

// Lightweight functional DL surface (no module stack).
// Lives under pluss::ai::nn, alongside the module classes -- reachable
// publicly as pluss::nn::* (see <pluss/nn.h>) or fully qualified as
// pluss::ai::nn::*. Never re-exported into the bare pluss:: root
// namespace, which is reserved for things that are truly global across the
// whole codebase.

#include "pluss/core.h"
#include "pluss/ai/fusion.h"
#include "pluss/ai/conv.h"
#include "pluss/ai/rnn.h"
#include "pluss/ai/attention.h"

namespace pluss::ai::nn {

using ::pluss::ai::dropout;
using ::pluss::ai::cublas_fusion_ready;
using ::pluss::ai::cublas_fusion_error;
using ::pluss::ai::mps_fusion_ready;
using ::pluss::ai::rnn::gru;
using ::pluss::ai::rnn::lstm;
using ::pluss::ai::rnn::rnn;
using ::pluss::ai::cudnn_rnn_enabled;
using ::pluss::ai::cudnn_rnn_strict;
using ::pluss::ai::cudnn_rnn_ready;
using ::pluss::ai::Padding1d;
using ::pluss::ai::causal_padding1d;
using ::pluss::ai::conv;
using ::pluss::ai::conv1d;
using ::pluss::ai::conv2d;
using ::pluss::ai::conv3d;
using ::pluss::ai::cudnn_conv_enabled;
using ::pluss::ai::cudnn_conv_strict;
using ::pluss::ai::cudnn_conv_ready;
using ::pluss::ai::scaled_attention;
using ::pluss::ai::multihead_attention;
using ::pluss::ai::causal_attention_mask;
using ::pluss::ai::square_subsequent_mask;
using ::pluss::ai::cudnn_attention_enabled;
using ::pluss::ai::cudnn_attention_strict;
using ::pluss::ai::cudnn_attention_ready;
using ::pluss::ai::cudnn_transformer_ready;
using ::pluss::ai::cudnn_transformer_enabled;
using ::pluss::ai::cudnn_transformer_strict;
using ::pluss::ai::AttentionOptions;
using ::pluss::ai::MultiheadOptions;
using ::pluss::ai::MultiheadWeights;

// Type shim for concise functional RNN configuration:
//   pluss::nn::rnn::NetWeight, pluss::nn::rnn::NetConfig, pluss::nn::rnn::LSTMState
// while keeping callable pluss::nn::rnn(...) in the same surface.
struct rnn {
  using NetWeight = ::pluss::ai::rnn::NetWeight;
  using NetConfig = ::pluss::ai::rnn::NetConfig;
  using LSTMState = ::pluss::ai::rnn::LSTMState;
};

inline Series fusion(const Series& input, const Series& weight, const Series& bias = pluss::series::undefined()) {
  return ::pluss::ai::ff(input, weight, bias);
}

} // namespace pluss::ai::nn
