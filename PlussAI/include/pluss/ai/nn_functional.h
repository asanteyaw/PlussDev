#pragma once

// Lightweight functional neural surface (no module stack, no NodeJ umbrella).
//
// Exposes functional operators under pluss::ai::nn (reachable publicly as
// pluss::nn::*, or fully qualified as pluss::ai::nn::*), such as:
// - pluss::nn::fusion
// - pluss::nn::conv / conv1d / conv2d
// - pluss::nn::rnn / gru / lstm
// - pluss::nn::scaled_attention / multihead_attention
//
// This header is intentionally narrower than <pluss/ai.h>.

#include "pluss/nn_func.h"

