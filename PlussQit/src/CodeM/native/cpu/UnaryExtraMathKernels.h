#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {

void rsqrt(const Series& a, const Series& out);
void log2(const Series& a, const Series& out);
void log10(const Series& a, const Series& out);
void exp2(const Series& a, const Series& out);
void cbrt(const Series& a, const Series& out);
void square(const Series& a, const Series& out);

} // namespace
