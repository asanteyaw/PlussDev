#include "rnn/RNN.h"

// Implementation of RNN, LSTM, and GRU modules in neural network architectures. These modules apply the corresponding RNN cells (RNNCell, LSTMCell, GRUCell) across an input sequence, handling multiple layers, bidirectionality, and dropout as specified in the constructor parameters. The forward method processes the input sequence step by step, applying the cell computations and updating the hidden states accordingly. The implementation also supports packed sequences for variable-length inputs.
// It does delegations where appropriate to the functional-like versions of the cells (pluss::rnn_cell, pluss::lstm_cell, pluss::gru_cell).
// Intended usage:
// pluss::nn::RNN rnn(pluss::nn::RNNOptions(input.size(1), input.size(1), /*batch_first=*/true, ...));
// Series output = rnn->forward(input);
// pluss::nn::GRU and pluss::nn::LSTM have similar usage patterns.