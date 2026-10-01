#pragma once

#include "pluss/ai/nn_grad.h"
#include "pluss/ai/nn_functional.h"

// The DataLoader stack (Dataset/SeriesDataset/DataLoader/split_fields/
// split_loaders/temporal_split_*, reachable as pluss::loader::*) physically
// lives in PlussTils, not PlussAI -- it is purpose-built to pair with nn
// training and documented as the nn/data family, so it belongs behind this
// same single include.
#include "pluss/tils/aux/data_loader.h"

#include "NodeK/NN/modules/Module.h"
#include "NodeK/NN/modules/activation/Activation.h"
#include "NodeK/NN/modules/container/Schedule.h"
#include "NodeK/NN/modules/embedding/Embedding.h"
#include "NodeK/NN/modules/loss/Loss.h"
#include "NodeK/NN/modules/fusion/fusion.h"
#include "NodeK/NN/modules/normalization/LayerNorm.h"
#include "NodeK/NN/modules/rnn/RNNBase.h"
#include "NodeK/NN/modules/rnn/RNNImpl.h"
#include "NodeK/NN/modules/rnn/GRUImpl.h"
#include "NodeK/NN/modules/rnn/LSTMImpl.h"
#include "NodeK/NN/modules/attention/multihead.h"
#include "NodeK/NN/modules/attention/transformer.h"
#include "NodeK/NN/modules/cnn/NNconv1d.h"
#include "NodeK/NN/modules/cnn/NNconvNd.h"
#include "NodeK/NN/modules/cnn/NNconv3d.h"
#include "NodeK/NN/optim/optim.h"
