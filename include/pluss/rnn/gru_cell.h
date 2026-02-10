#pragma once

#include <PlussQit/core/SeriesBody.h>
#include <pluss/Types.h>
#include <pluss/series_factory.h>
#include <pluss/Fusion.h>
#include <pluss/Series.h>

namespace pluss {
namespace rnn {

/**
 * @brief Applies a single-layer GRU to an input sequence.
 */
class GRUCell {
public:
    /**
     * @brief Construct a new GRUCell
     * 
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param bias If false, then the layer does not use bias weights. Default: true
     */
    GRUCell(int64_t input_size, int64_t hidden_size, bool bias = true)
        : input_size_(input_size), hidden_size_(hidden_size), bias_(bias) {
        
        // Initialize weights and biases
        weight_ih_ = pluss::randn({3 * hidden_size, input_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        weight_hh_ = pluss::randn({3 * hidden_size, hidden_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        if (bias) {
            bias_ih_ = pluss::zeros({3 * hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            
            bias_hh_ = pluss::zeros({3 * hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
    }
    
    /**
     * @brief Forward pass
     * 
     * @param input Input series of shape (batch, input_size)
     * @param hx Previous hidden state of shape (batch, hidden_size)
     * @return Series New hidden state of shape (batch, hidden_size)
     */
    Series forward(const Series& input, const Series& hx = Series()) const {
        PLUSS_INTERNAL_DISPATCH({
            // Get batch size
            bool has_batch_dim = input.dim() > 1;
            int64_t batch_size = has_batch_dim ? input.sizes()[0] : 1;
            
            // Initialize hidden state if not provided
            Series h = hx;
            if (!h.defined() || h.numel() == 0) {
                h = pluss::zeros({batch_size, hidden_size_}, 
                               pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            }
            
            // Apply linear transformations
            Series gi = Fusion(input, weight_ih_, bias_ ? bias_ih_ : Series());
            Series gh = Fusion(h, weight_hh_, bias_ ? bias_hh_ : Series());
            
            // Split gates into chunks
            auto gi_chunks = gi.chunk(3, 1);
            auto gh_chunks = gh.chunk(3, 1);
            
            Series reset_gate = gi_chunks[0].add(gh_chunks[0]).sigmoid();
            Series update_gate = gi_chunks[1].add(gh_chunks[1]).sigmoid();
            Series new_gate = gi_chunks[2].add(reset_gate.mul(gh_chunks[2])).tanh();
            
            // Update hidden state
            Series h_next = update_gate.mul(h).add(update_gate.neg().add(1.0).mul(new_gate));
            
            return h_next;
        });
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
