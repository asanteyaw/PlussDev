#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void exp(const Series& a, Series& out);
  void log(const Series& a, Series& out);
  void sqrt(const Series& a, Series& out);
  void sin(const Series& a, Series& out);
  void cos(const Series& a, Series& out);
  void tanh(const Series& a, Series& out);
}
