#include "pluss/subsp/actuarial/loss.h"

#include <cmath>
#include <string>
#include <string_view>

namespace pluss::subsp::actuarial {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

void nonnegative_(double value, std::string_view name) {
  PLUSS_CHECK(std::isfinite(value), std::string(name) + " must be finite");
  PLUSS_CHECK(value >= 0.0, std::string(name) + " must be >= 0");
}

Series losses_(const Series& losses, std::string_view operation) {
  PLUSS_CHECK(losses.defined(), std::string(operation) + ": losses must be defined");
  PLUSS_CHECK(losses.dim() == 1, std::string(operation) + ": losses must be 1D");
  const Series out = losses.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(),
              std::string(operation) + ": losses must be finite");
  PLUSS_CHECK(out.ge(0.0).all().item<bool>(),
              std::string(operation) + ": losses must be >= 0");
  return out;
}

} // namespace

namespace operators {

LossTransform deductible(double amount) {
  nonnegative_(amount, "operators::deductible: amount");
  return LossTransform("deductible", [amount](const Series& input) {
    return losses_(input, "deductible").sub(amount).maximum(0.0);
  });
}

LossTransform limit(double amount) {
  nonnegative_(amount, "operators::limit: amount");
  return LossTransform("limit", [amount](const Series& input) {
    return losses_(input, "limit").minimum(amount);
  });
}

LossTransform coinsurance(double share) {
  nonnegative_(share, "operators::coinsurance: share");
  PLUSS_CHECK(share <= 1.0, "operators::coinsurance: share must be <= 1");
  return LossTransform("coinsurance", [share](const Series& input) {
    return losses_(input, "coinsurance").mul(share);
  });
}

LossTransform layer(double attachment, double layer_limit, double share) {
  nonnegative_(attachment, "operators::layer: attachment");
  nonnegative_(layer_limit, "operators::layer: limit");
  nonnegative_(share, "operators::layer: share");
  PLUSS_CHECK(share <= 1.0, "operators::layer: share must be <= 1");
  return LossTransform(
      "layer", [attachment, layer_limit, share](const Series& input) {
        return losses_(input, "layer")
            .sub(attachment)
            .maximum(0.0)
            .minimum(layer_limit)
            .mul(share);
      });
}

LossMeasure mean() {
  return LossMeasure("mean", [](const Series& input) {
    const Series losses = losses_(input, "mean");
    if (losses.numel() == 0) {
      return Series::zeros({}, ScalarType::Float64).to(losses.platform());
    }
    return losses.mean();
  });
}

} // namespace operators
} // namespace pluss::subsp::actuarial
