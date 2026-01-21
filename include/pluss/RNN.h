#pragma once

#include <pluss/rnn/rnn_cell.h>
#include <pluss/rnn/lstm_cell.h>
#include <pluss/rnn/gru_cell.h>

namespace pluss {
namespace rnn {

// Re-export the RNN cell classes for easier access

using RNNCell = pluss::rnn::RNNCell;
using LSTMCell = pluss::rnn::LSTMCell;
using GRUCell = pluss::rnn::GRUCell;

} // namespace rnn
} // namespace pluss
