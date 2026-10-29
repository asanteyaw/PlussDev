#pragma once

#include "pluss/nn.h"
#include "pluss/ai/ml.h"
#include "pluss/ai/procedure.h"

// Compatibility umbrella.
// The active fusion/RNN surface now lives under <pluss/nn.h> plus the
// structured NodeK models/ and NN/modules/ headers that it exports.
//
// Public namespace families:
// - pluss::ai::classic -> NodeJ classical ML core
// - pluss::ai::nn      -> NodeK neural/module surface
// - pluss::ai::nlp     -> NodeB NLP surface
