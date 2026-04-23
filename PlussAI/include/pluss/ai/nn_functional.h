#pragma once

// Lightweight functional neural surface (no module stack, no NodeJ umbrella).
//
// Exposes top-level functional operators such as:
// - pluss::fusion
// - pluss::conv / conv1d / conv2d
// - pluss::rnn / gru / lstm
// - pluss::scaled_attention / multihead_attention
//
// This header is intentionally narrower than <pluss/ai.h>.

#include "pluss/nn_func.h"

