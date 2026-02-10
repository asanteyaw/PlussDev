#pragma once

#include <PlussQit/core/SeriesBody.h>
#include <pluss/Types.h>
#include <pluss/series_factory.h>
#include <pluss/Fusion.h>
#include <pluss/Series.h>

namespace pluss {
namespace rnn {

/**
 * @brief Applies a single-layer Elman RNN with tanh or ReLU non-linearity to an input sequence.
 */
class RNNCell {
public:
    /**
     * @brief Construct a new RNNCell
     * 
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param nonlinearity The non-linearity to use. Can be 'tanh' or 'relu'. Default: 'tanh'
     * @param bias If false, then the layer does not use bias weights. Default: true
     */
    RNNCell(int64_t input_size, int64_t hidden_size, 
            const std::string& nonlinearity = "tanh", bool bias = true)
        : input_size_(input_size), hidden_size_(hidden_size),
          nonlinearity_(nonlinearity), bias_(bias) {
        
        // Initialize weights and biases
        weight_ih_ = pluss::randn({hidden_size, input_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        weight_hh_ = pluss::randn({hidden_size, hidden_size}, 
                                 pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        if (bias) {
            bias_ih_ = pluss::zeros({hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            
            bias_hh_ = pluss::zeros({hidden_size}, 
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
            Series igates = Fusion(input, weight_ih_, bias_ ? bias_ih_ : Series());
            Series hgates = Fusion(h, weight_hh_, bias_ ? bias_hh_ : Series());
            
            // Apply non-linearity
            Series output = igates + hgates;
            if (nonlinearity_ == "tanh") {
                output = output.tanh();
            } else if (nonlinearity_ == "relu") {
                output = output.relu();
            }
            
            return output;
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
     * @brief Get the non-linearity
     * 
     * @return std::string Non-linearity
     */
    std::string nonlinearity() const {
        return nonlinearity_;
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
    std::string nonlinearity_;
    bool bias_;
    
    Series weight_ih_;
    Series weight_hh_;
    Series bias_ih_;
    Series bias_hh_;
};

} // namespace rnn
} // namespace pluss
