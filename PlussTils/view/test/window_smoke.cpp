// Opens a real PlussView window, shows a table, and has the viewer write what Save As would: a PNG
// and a PDF with only the table in them. It then checks both files. Run it where a display exists
// (a desktop session, or xvfb on Linux): plussview_window_smoke <output directory>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "pluss/tils/view/html_window.h"

namespace {

[[nodiscard]] std::string page() {
  std::string html =
      "<html><head><style>table{width:100%;max-height:28.6em;margin:0;} th,td{padding:8px;}"
      "</style></head><body><div><table><thead><tr>";
  for (int c = 0; c < 3; ++c) html += "<th>column_" + std::to_string(c) + "</th>";
  html += "</tr></thead><tbody>";
  for (int r = 0; r < 4; ++r) {
    html += "<tr>";
    for (int c = 0; c < 3; ++c) html += "<td>value " + std::to_string(r) + "-" + std::to_string(c) + "</td>";
    html += "</tr>";
  }
  html += "</tbody></table></div></body></html>";
  return html;
}

[[nodiscard]] std::string slurp(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void set_script(const std::string& script) {
#if defined(_WIN32)
  _putenv_s("PLUSSTILS_VIEW_SCRIPT", script.c_str());
#else
  setenv("PLUSSTILS_VIEW_SCRIPT", script.c_str(), 1);
#endif
}

[[nodiscard]] std::uint32_t be32(const std::string& data, const std::size_t at) {
  return (static_cast<std::uint32_t>(static_cast<unsigned char>(data[at])) << 24U) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(data[at + 1])) << 16U) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(data[at + 2])) << 8U) |
         static_cast<std::uint32_t>(static_cast<unsigned char>(data[at + 3]));
}

}  // namespace

int main(int argc, char** argv) {
  const std::filesystem::path directory = argc > 1 ? argv[1] : "plussview-smoke";
  std::filesystem::create_directories(directory);
  const std::string png = (directory / "table.png").string();
  const std::string pdf = (directory / "table.pdf").string();
  std::filesystem::remove(png);
  std::filesystem::remove(pdf);
  // Let the first frames render, write both exports, keep the window up long enough to look at,
  // then close it.
  set_script("sleep:1500;note:export-png-to:" + png + ";note:export-pdf-to:" + pdf + ";sleep:4000;close");

  pluss::view::HtmlWindowOptions options{};
  options.title = "PlussView smoke";
  options.width = 1000;
  options.height = 600;
  (void)pluss::view::show_html_window(page(), options);

  const std::string png_data = slurp(png);
  const std::string pdf_data = slurp(pdf);
  if (png_data.size() < 33 || png_data.compare(1, 3, "PNG") != 0) {
    std::cerr << "[FAIL] the PNG export was not written\n";
    return 1;
  }
  const std::uint32_t width = be32(png_data, 16);
  const std::uint32_t height = be32(png_data, 20);
  if (width < 100 || height < 40) {
    std::cerr << "[FAIL] the PNG export is implausibly small: " << width << "x" << height << "\n";
    return 1;
  }
  // The window is 1000x600 (more pixels on a high-density display); the export holds only the
  // table, which is far shorter than the window.
  if (height * 100U >= 600U * 100U * 2U) {
    std::cerr << "[FAIL] the PNG export is as tall as the window: " << width << "x" << height << "\n";
    return 1;
  }
  if (pdf_data.rfind("%PDF-", 0) != 0 || pdf_data.size() < 200) {
    std::cerr << "[FAIL] the PDF export was not written\n";
    return 1;
  }
  std::cout << "[PASS] plussview_window_smoke png=" << width << "x" << height << " pdf=" << pdf_data.size()
            << " bytes\n";
  return 0;
}
