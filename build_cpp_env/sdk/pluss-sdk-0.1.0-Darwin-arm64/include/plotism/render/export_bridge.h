#pragma once

#include "pluss/pl/plusspl.h"

namespace pluss::pl::plotism::render {

std::string render_svg(const Studio& studio);
std::string render_pdf(const Studio& studio);
void save_svg(const Studio& studio, std::string_view path);
void save_pdf(const Studio& studio, std::string_view path);

} // namespace pluss::pl::plotism::render
