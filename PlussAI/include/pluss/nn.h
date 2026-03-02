#pragma once

#include "pluss/core.h"
#include "pluss/ai/fusion.h"
#include "pluss/ai/conv.h"
#include "pluss/ai/rnn.h"
#include "pluss/ai/nn.h"

namespace pluss {

using ::pluss::ai::dropout;
using ::pluss::ai::rnn::gru;
using ::pluss::ai::rnn::lstm;
using ::pluss::ai::rnn::rnn;
using ::pluss::ai::causal_padding1d;
using ::pluss::ai::conv;
using ::pluss::ai::conv1d;
using ::pluss::ai::conv2d;

inline Series fusion(const Series& input, const Series& weight, const Series& bias = Series()) {
  return ::pluss::ai::ff(input, weight, bias);
}

namespace nn = ::pluss::ai::nn;

} // namespace pluss
