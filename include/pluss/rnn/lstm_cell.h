#pragma once

#include "pluss/Fusion.h"
#include <PlussQit/core/SeriesBody.h>
#include <pluss/Types.h>
#include <pluss/series_factory.h>
#include <pluss/Fusion.h>
#include <pluss/rnn/dispatch_macros.h>
#include <tuple>

namespace pluss {
namespace rnn {

/**
 * @brief Applies a single-layer LSTM to an input sequence.
 */
class LSTMCell {
public:
    /**
     * @brief Construct a new LSTMCell
     * 
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param bias If false, then the layer does not use bias weights. Default: true
     */
    LSTMCell(int64_t input_size, int64_t hidden_size, bool bias = true)
        : input_size_(input_size), hidden_size_(hidden_size), bias_(bias) {
        
        // Initialize weights and biases
        weight_ih_ = pluss::randn({4 * hidden_size, input_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        weight_hh_ = pluss::randn({4 * hidden_size, hidden_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        if (bias) {
            bias_ih_ = pluss::zeros({4 * hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            
            bias_hh_ = pluss::zeros({4 * hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
    }
    
    /**
     * @brief Forward pass
     * 
     * @param input Input series of shape (batch, input_size)
     * @param hidden Previous hidden state tuple (h_0, c_0) of shapes (batch, hidden_size)
     * @return std::tuple<Series, Series> New hidden state tuple (h_1, c_1)
     */
    std::tuple<Series, Series> forward(const Series& input, 
                                      const std::tuple<Series, Series>& hidden = std::tuple<Series, Series>()) const {
        // Get batch size
        bool has_batch_dim = input.dim() > 1;
        int64_t batch_size = has_batch_dim ? input.sizes()[0] : 1;
        
        // Initialize hidden state if not provided
        Series h, c;
        if (std::get<0>(hidden).defined() && std::get<0>(hidden).numel() > 0) {
            h = std::get<0>(hidden);
            c = std::get<1>(hidden);
        } else {
            h = pluss::zeros({batch_size, hidden_size_}, 
                           pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            c = pluss::zeros({batch_size, hidden_size_}, 
                           pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
        
        // Apply linear transformations
        Series gates = Fusion(input, weight_ih_, bias_ ? bias_ih_ : Series())
                      .add(Fusion(h, weight_hh_, bias_ ? bias_hh_ : Series()));
        
        // Split gates into chunks
        auto chunks = gates.chunk(4, 1);
        Series in_gate = chunks[0].sigmoid();
        Series forget_gate = chunks[1].sigmoid();
        Series cell_gate = chunks[2].tanh();
        Series out_gate = chunks[3].sigmoid();
        
        // Update cell state and hidden state
        Series c_next = forget_gate.mul(c).add(in_gate.mul(cell_gate));
        Series h_next = out_gate.mul(c_next.tanh());
        
        return std::make_tuple(h_next, c_next);
    }
    
    /**
     * @brief Get the input size
     * 
     * @return int64_t Input size
     */
    int64_t input_size() const {
        return input_size_;
    }
    
    /**
     * @brief Get the hidden size
     * 
     * @return int64_t Hidden size
     */
    int64_t hidden_size() const {
        return hidden_size_;
    }
    
    /**
     * @brief Check if the cell has bias
     * 
     * @return bool True if the cell has bias, false otherwise
     */
    bool bias() const {
        return bias_;
    }
    
private:
    int64_t input_size_;
    int64_t hidden_size_;
    bool bias_;
    
    Series weight_ih_;
    Series weight_hh_;
    Series bias_ih_;
    Series bias_hh_;
};

} // namespace rnn
} // namespace pluss
