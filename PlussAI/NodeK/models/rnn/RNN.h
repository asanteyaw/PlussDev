#pragma once

#include "rnn_impl.h"

namespace pluss {
   using rnn = pluss::ai::rnn::RNN;  // alias so users can call pluss::rnn(...) instead of pluss::ai::rnn::RNN(...)
   using gru = pluss::ai::rnn::GRU;  // alias so users can call pluss::gru(...) instead of pluss::ai::rnn::GRU(...)
   using lstm = pluss::ai::rnn::LSTM; // alias so users can call pluss::lstm(...) instead of pluss::ai::rnn::LSTM(...)

} // namespace pluss
