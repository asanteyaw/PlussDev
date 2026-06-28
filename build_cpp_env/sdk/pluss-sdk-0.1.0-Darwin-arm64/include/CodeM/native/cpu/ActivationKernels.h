#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  void sigmoid(const Series& a, Series& out);
  void sigmoid_inplace(const Series& a);

  void softplus(const Series& a, Series& out, double beta=1.0, double threshold=20.0);
  void softplus_inplace(const Series& a, double beta=1.0, double threshold=20.0);

  void silu(const Series& a, Series& out);
  void silu_inplace(const Series& a);

  void gelu(const Series& a, Series& out, bool approximate=true);
  void gelu_inplace(const Series& a, bool approximate=true);

  void leaky_relu(const Series& a, Series& out, double negative_slope=0.01);
  void leaky_relu_inplace(const Series& a, double negative_slope=0.01);
}
