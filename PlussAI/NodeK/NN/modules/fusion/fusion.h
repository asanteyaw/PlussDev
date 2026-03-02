#include "fusion/Fusion.h"

// This file implements the Fusion module, which applies a linear transformation to the input data.
// It is to be used in a neural network architecture, not to be confused with pluss::fusion(...) which is a the functional-like version for users.
// It's usage will be pluss::nn::Fusion fc (args) as instantiation. It delegates to the functional pluss::fusion(...) in its implementation of the forward method.
// Example usage:
// pluss::nn::Fusion fc(pluss::nn::FusionOptions(input.size(1), input.size(1)));
// Series output = fc->forward(input);

namespace pluss::ai::nn { 
// implementation of the Fusion module

// aliases for convenience
class FusionOptions {
public:
    int64_t in_features;
    int64_t out_features;
    bool bias;
      FusionOptions(int64_t in_features, int64_t out_features, bool bias = true)
         : in_features(in_features), out_features(out_features), bias(bias) {}
};

class Fusion {
public:
    Fusion(const FusionOptions& options) : module_(options.in_features, options.out_features, options.bias) {}
      Fusion(int64_t in_features, int64_t out_features, bool bias = true) : module_(in_features, out_features, bias) {}

    
    Series forward(const Series& input) const {
        return pluss::fusion(input, module_.weight(), module_.with_bias() ? module_.bias() : Series());
    }
    
    const Series& weight() const {
        return module_.weight();
    }
    
    const Series& bias() const {
        return module_.bias();
    }

};
}  // namespace pluss::ai::nn  

namespace pluss::nn {
using Fusion = pluss::ai::nn::Fusion; // for convenience
using FusionOptions = pluss::ai::nn::FusionOptions;
}