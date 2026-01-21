#pragma once

#include <pluss/SeriesBody.h>
#include <pluss/Types.h>
#include <pluss/series_factory.h>

namespace pluss {

/**
 * @brief Applies a linear transformation to the incoming data: y = xA^T + b
 * 
 * This is similar to torch::Linear but named Fusion to avoid confusion.
 */
class Fusion {
public:
    /**
     * @brief Construct a new Fusion module
     * 
     * @param in_features Size of each input sample
     * @param out_features Size of each output sample
     * @param bias If set to false, the layer will not learn an additive bias
     */
   Fusion(int64_t in_features, int64_t out_features, bool bias = true)
        : in_features_(in_features), out_features_(out_features), with_bias_(bias) {
        
        // Initialize weight
        weight_ = pluss::randn({out_features, in_features}, 
                              pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        // Initialize bias if needed
        if (with_bias_) {
            bias_ = pluss::zeros({out_features}, 
                                pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
    }
    
    /**
     * @brief Construct a new Fusion module with pre-initialized weights and biases
     * 
     * @param weight Weight tensor of shape (out_features, in_features)
     * @param bias Bias tensor of shape (out_features) or empty tensor
     */
   Fusion(const Series& weight, const Series& bias = Series())
        : weight_(weight), bias_(bias), 
          in_features_(weight.sizes()[1]), out_features_(weight.sizes()[0]),
          with_bias_(bias.defined() && bias.numel() > 0) {
        
        // Validate dimensions
        PLUSS_INTERNAL_CHECK(weight.dim() == 2, 
                            "Weight tensor should be 2D, got " + std::to_string(weight.dim()) + "D");
        
        if (with_bias_) {
            PLUSS_INTERNAL_CHECK(bias.dim() == 1, 
                                "Bias tensor should be 1D, got " + std::to_string(bias.dim()) + "D");
            PLUSS_INTERNAL_CHECK(bias.sizes()[0] == out_features_, 
                                "Bias tensor size should match out_features");
        }
    }
    
    /**
     * @brief Apply the linear transformation
     * 
     * @param input Input tensor of shape (*, in_features) where * means any number of dimensions
     * @return Output tensor of shape (*, out_features)
     */
    Series forward(const Series& input) const {
        PLUSS_INTERNAL_DISPATCH({
            // Validate input
            PLUSS_INTERNAL_CHECK(input.sizes()[input.dim()-1] == in_features_, 
                                "Last dimension of input should be " + std::to_string(in_features_) + 
                                ", got " + std::to_string(input.sizes()[input.dim()-1]));
            
            // Apply linear transformation: y = xA^T + b
            Series output = input.matmul(weight_.t());
            
            // Add bias if needed
            if (with_bias_) {
                output.add_(bias_);
            }
            
            return output;
        });
    }
    
    /**
     * @brief Get the weight tensor
     * 
     * @return Weight tensor
     */
    const Series& weight() const {
        return weight_;
    }
    
    /**
     * @brief Get the bias tensor
     * 
     * @return Bias tensor or empty tensor if no bias
     */
    const Series& bias() const {
        return bias_;
    }
    
    /**
     * @brief Check if the module has bias
     * 
     * @return true if the module has bias, false otherwise
     */
    bool with_bias() const {
        return with_bias_;
    }
    
    /**
     * @brief Get the number of input features
     * 
     * @return Number of input features
     */
    int64_t in_features() const {
        return in_features_;
    }
    
    /**
     * @brief Get the number of output features
     * 
     * @return Number of output features
     */
    int64_t out_features() const {
        return out_features_;
    }
    
private:
    Series weight_;
    Series bias_;
    int64_t in_features_;
    int64_t out_features_;
    bool with_bias_;
};

/**
 * @brief Apply a linear transformation to the input
 * 
 * @param input Input tensor of shape (*, in_features)
 * @param weight Weight tensor of shape (out_features, in_features)
 * @param bias Bias tensor of shape (out_features) or empty tensor
 * @return Output tensor of shape (*, out_features)
 */
inline Series Fusion(const Series& input, const Series& weight, const Series& bias = Series()) {
    PLUSS_INTERNAL_DISPATCH({
        // Validate dimensions
        PLUSS_INTERNAL_CHECK(weight.dim() == 2, 
                            "Weight tensor should be 2D, got " + std::to_string(weight.dim()) + "D");
        PLUSS_INTERNAL_CHECK(input.sizes()[input.dim()-1] == weight.sizes()[1], 
                            "Last dimension of input should match second dimension of weight");
        
        // Apply linear transformation: y = xA^T + b
        Series output = input.matmul(weight.t());
        
        // Add bias if provided
        if (bias.defined() && bias.numel() > 0) {
            PLUSS_INTERNAL_CHECK(bias.dim() == 1, 
                                "Bias tensor should be 1D, got " + std::to_string(bias.dim()) + "D");
            PLUSS_INTERNAL_CHECK(bias.sizes()[0] == weight.sizes()[0], 
                                "Bias tensor size should match first dimension of weight");
            
            output.add_(bias);
        }
        
        return output;
    });
}

} // namespace pluss
