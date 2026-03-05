#pragma once

#include <string>
#include <string_view>

namespace pluss::pl::plotism::spatex {

[[nodiscard]] std::string normalize_display_text(std::string_view text);
[[nodiscard]] std::string escape_pdf_text(std::string_view text);

} // namespace pluss::pl::plotism::spatex
