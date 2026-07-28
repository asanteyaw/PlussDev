#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "pluss/tils/view/html_window.h"
#include "platform/PlatformHtmlViewer.h"

namespace {

void expect(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class ExportTableModel final : public pluss::view::VirtualTableModel {
 public:
  std::int64_t row_count() const noexcept override { return 2; }
  std::int64_t column_count() const noexcept override { return 2; }
  std::string row_header() const override { return "case"; }
  std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
  std::string column_name(const std::int64_t column) const override {
    return column == 0 ? "city" : "revenue";
  }
  bool column_numeric(const std::int64_t column) const noexcept override { return column == 1; }
  pluss::view::VirtualTableCell cell(const std::int64_t row,
                                     const std::int64_t column) const override {
    if (column == 0) {
      return pluss::view::VirtualTableCell{
          .text = row == 0 ? "Accra" : "Tema & Port",
          .missing = false,
      };
    }
    return pluss::view::VirtualTableCell{
        .text = row == 0 ? "1250.5" : "980",
        .missing = false,
    };
  }
  bool row_matches(const std::int64_t row,
                   const std::string_view query) const override {
    return row_label(row).find(query) != std::string::npos;
  }
};

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

    const std::filesystem::path virtual_output =
        std::filesystem::temp_directory_path() / "plussview_virtual_export_test.xlsx";
    auto model = std::make_shared<ExportTableModel>();
    expect(pluss::view::detail::export_inhouse_table_xlsx(model, virtual_output.string()),
           "virtual-table XLSX export should succeed");
    expect(std::filesystem::exists(virtual_output) && std::filesystem::file_size(virtual_output) > 500,
           "virtual-table XLSX export should create a non-empty workbook package");

    std::ifstream virtual_stream(virtual_output, std::ios::binary);
    char virtual_signature[2]{};
    virtual_stream.read(virtual_signature, 2);
    expect(virtual_stream.good() && virtual_signature[0] == 'P' && virtual_signature[1] == 'K',
           "virtual-table XLSX output should be an Office Open XML ZIP package");

    std::filesystem::remove(virtual_output);
    std::cout << "[PASS] real_xlsx_export\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
