#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void relu(const Series& a, Series& out);
  void exp(const Series& a, Series& out);
  void log(const Series& a, Series& out);
}
