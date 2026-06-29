#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "platform/PlatformHtmlViewer.h"

namespace {

void expect(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

int main() {
  try {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "plussview_html_export_test.xlsx";
    const std::string html = R"HTML(
<!doctype html>
<html><body>
  <table>
    <thead><tr><th>case</th><th>city</th><th>revenue</th></tr></thead>
    <tbody>
      <tr><td>0</td><td>Accra</td><td>1250.5</td></tr>
      <tr><td>1</td><td>Tema &amp; Port</td><td>980</td></tr>
    </tbody>
  </table>
</body></html>
)HTML";

    expect(pluss::view::detail::export_inhouse_table_xlsx(html, output.string()),
           "real XLSX export should succeed");
    expect(std::filesystem::exists(output) && std::filesystem::file_size(output) > 500,
           "real XLSX export should create a non-empty workbook package");

    std::ifstream stream(output, std::ios::binary);
    char signature[2]{};
    stream.read(signature, 2);
    expect(stream.good() && signature[0] == 'P' && signature[1] == 'K',
           "XLSX output should be an Office Open XML ZIP package");

    std::filesystem::remove(output);
    std::cout << "[PASS] real_xlsx_export\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
