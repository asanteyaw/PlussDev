#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {

void atan2(const Series& a, const Series& b, const Series& out);
void fmod(const Series& a, const Series& b, const Series& out);
void remainder(const Series& a, const Series& b, const Series& out);

} // namespace
