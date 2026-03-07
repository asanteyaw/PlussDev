#include "plotism/render/export_bridge.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "CodeM/core/Errors.h"
#include "pluss/view/html_window.h"

namespace pluss::pl {

namespace {

std::string escape_html(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 16);
  for (const auto ch : text) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  return out;
}

std::string shell_quote_posix(std::string_view raw) {
  std::string out;
  out.reserve(raw.size() + 2);
  out.push_back('\'');
  for (const char ch : raw) {
    if (ch == '\'') {
      out.append("'\\''");
    } else {
      out.push_back(ch);
    }
  }
  out.push_back('\'');
  return out;
}

std::string shell_quote_windows(std::string_view raw) {
  std::string out;
  out.reserve(raw.size() + 2);
  out.push_back('"');
  for (const char ch : raw) {
    if (ch == '"') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  out.push_back('"');
  return out;
}

std::string lowercase_ascii(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

void ensure_extension_match_(const std::filesystem::path& path, std::string_view expected_ext, std::string_view where) {
  PLUSS_CHECK(!path.empty(), std::string(where) + "(): output path cannot be empty");
  const auto actual = lowercase_ascii(path.extension().string());
  const auto expected = lowercase_ascii(std::string(expected_ext));
  PLUSS_CHECK(actual == expected,
              std::string(where) + "(): filename extension '" + actual + "' does not match expected '" + expected +
                  "'");
}

std::string svg_embed_html(std::string_view svg_payload, std::string_view title, std::int64_t width, std::int64_t height) {
  const auto safe_width = std::max<std::int64_t>(320, width);
  const auto safe_height = std::max<std::int64_t>(240, height);
  const auto page_title = title.empty() ? std::string("PlussPl Plot") : std::string(title);
  const std::string svg(svg_payload);
  return "<!doctype html>\n"
         "<html lang=\"en\">\n"
         "<head>\n"
         "  <meta charset=\"utf-8\" />\n"
         "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
         "  <title>" + escape_html(page_title) + "</title>\n"
         "  <style>\n"
         "    html, body { margin: 0; padding: 0; background: #f7f7f7; min-height: 100%; }\n"
         "    body { display: grid; grid-template-rows: auto 1fr; min-height: 100vh; }\n"
         "    .toolbar { display: flex; align-items: center; justify-content: space-between; "
         "padding: 10px 12px; border-bottom: 1px solid #dddddd; background: #ffffff; }\n"
         "    .title { font: 600 14px/1.2 -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; "
         "color: #1f2937; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 50vw; }\n"
         "    .controls { display: flex; gap: 6px; }\n"
         "    button { border: 1px solid #cbd5e1; background: #ffffff; color: #1f2937; border-radius: 6px; "
         "padding: 6px 10px; font: 500 12px/1 -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; cursor: pointer; }\n"
         "    button:hover { background: #f3f4f6; }\n"
         "    .stage { display: grid; place-items: center; min-height: 0; padding: 16px; }\n"
         "    .frame { width: " + std::to_string(safe_width) + "px; max-width: 96vw; max-height: 96vh; "
         "background: #ffffff; border: 1px solid #d8d8d8; box-shadow: 0 10px 30px rgba(0,0,0,0.12); overflow: auto; }\n"
         "    #canvas { transform-origin: center top; transition: transform 120ms ease-out; }\n"
         "    svg { display: block; width: 100%; height: auto; min-height: " + std::to_string(safe_height) + "px; }\n"
         "    .dark body, body.dark { background: #0f172a; }\n"
         "    body.dark .toolbar { background: #111827; border-color: #374151; }\n"
         "    body.dark .title { color: #e5e7eb; }\n"
         "    body.dark button { background: #1f2937; border-color: #4b5563; color: #e5e7eb; }\n"
         "    body.dark button:hover { background: #374151; }\n"
         "    body.dark .frame { background: #111827; border-color: #374151; }\n"
         "  </style>\n"
         "</head>\n"
         "<body>\n"
         "  <div class=\"toolbar\">\n"
         "    <div class=\"title\">" + escape_html(page_title) + "</div>\n"
         "    <div class=\"controls\">\n"
         "      <button id=\"zoom-out\" type=\"button\">-</button>\n"
         "      <button id=\"zoom-reset\" type=\"button\">Reset</button>\n"
         "      <button id=\"zoom-in\" type=\"button\">+</button>\n"
         "      <button id=\"toggle-bg\" type=\"button\">Theme</button>\n"
         "      <button id=\"download-svg\" type=\"button\">Save SVG</button>\n"
         "    </div>\n"
         "  </div>\n"
         "  <div class=\"stage\">\n"
         "    <div class=\"frame\">\n"
         "      <div id=\"canvas\">\n" + svg + "\n      </div>\n"
         "    </div>\n"
         "  </div>\n"
         "  <script>\n"
         "    (function () {\n"
         "      const canvas = document.getElementById('canvas');\n"
         "      const zoomIn = document.getElementById('zoom-in');\n"
         "      const zoomOut = document.getElementById('zoom-out');\n"
         "      const zoomReset = document.getElementById('zoom-reset');\n"
         "      const toggleBg = document.getElementById('toggle-bg');\n"
         "      const downloadSvg = document.getElementById('download-svg');\n"
         "      let scale = 1.0;\n"
         "      function clamp(value, lo, hi) { return Math.min(hi, Math.max(lo, value)); }\n"
         "      function apply() { canvas.style.transform = 'scale(' + scale.toFixed(3) + ')'; }\n"
         "      function step(delta) { scale = clamp(scale + delta, 0.35, 3.0); apply(); }\n"
         "      zoomIn.addEventListener('click', function () { step(0.1); });\n"
         "      zoomOut.addEventListener('click', function () { step(-0.1); });\n"
         "      zoomReset.addEventListener('click', function () { scale = 1.0; apply(); });\n"
         "      toggleBg.addEventListener('click', function () { document.body.classList.toggle('dark'); });\n"
         "      downloadSvg.addEventListener('click', function () {\n"
         "        const svg = canvas.querySelector('svg');\n"
         "        if (!svg) { return; }\n"
         "        const serializer = new XMLSerializer();\n"
         "        let data = serializer.serializeToString(svg);\n"
         "        if (!data.startsWith('<?xml')) {\n"
         "          data = '<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\\n' + data;\n"
         "        }\n"
         "        const blob = new Blob([data], { type: 'image/svg+xml;charset=utf-8' });\n"
         "        const url = URL.createObjectURL(blob);\n"
         "        const a = document.createElement('a');\n"
         "        a.href = url; a.download = 'plusspl_plot.svg'; document.body.appendChild(a); a.click();\n"
         "        document.body.removeChild(a); URL.revokeObjectURL(url);\n"
         "      });\n"
         "      document.addEventListener('keydown', function (evt) {\n"
         "        if (evt.key === '+' || evt.key === '=') { step(0.1); }\n"
         "        else if (evt.key === '-') { step(-0.1); }\n"
         "        else if (evt.key === '0') { scale = 1.0; apply(); }\n"
         "        else if (evt.key === 'b' || evt.key === 'B') { document.body.classList.toggle('dark'); }\n"
         "        else if (evt.key === 's' || evt.key === 'S') { downloadSvg.click(); }\n"
         "      });\n"
         "      canvas.addEventListener('wheel', function (evt) {\n"
         "        if (!evt.ctrlKey) { return; }\n"
         "        evt.preventDefault();\n"
         "        step(evt.deltaY < 0 ? 0.08 : -0.08);\n"
         "      }, { passive: false });\n"
         "      apply();\n"
         "    })();\n"
         "  </script>\n"
         "</body>\n"
         "</html>\n";
}

std::string native_window_title_(std::string_view fallback) {
  return std::string(fallback);
}

pluss::view::BackendPreference plot_backend_preference_() {
  const char* raw = std::getenv("PLUSSVIEW_PLOT_BACKEND");
  if (raw == nullptr) {
    return pluss::view::BackendPreference::NativeWebView;
  }

  std::string token(raw);
  token = lowercase_ascii(token);
  token.erase(std::remove_if(token.begin(),
                             token.end(),
                             [](unsigned char ch) { return std::isspace(ch) != 0; }),
              token.end());

  if (token == "inhouse" || token == "inhousehtml" || token == "plusswebview" || token == "pluss-webview") {
    return pluss::view::BackendPreference::PlussWebView;
  }
  if (token == "inhouseonly" || token == "inhouse-only" || token == "plusswebviewonly" ||
      token == "plusswebview-only" || token == "pluss-webview-only") {
    return pluss::view::BackendPreference::PlussWebViewOnly;
  }
  if (token == "auto") {
    return pluss::view::BackendPreference::Auto;
  }
  return pluss::view::BackendPreference::NativeWebView;
}

bool run_command_(const std::string& command) {
  const int status = std::system(command.c_str());
  return status == 0;
}

bool command_available_(std::string_view command) {
#if defined(_WIN32)
  const std::string probe = "where " + std::string(command) + " >nul 2>nul";
#else
  const std::string probe = "command -v " + std::string(command) + " >/dev/null 2>&1";
#endif
  return run_command_(probe);
}

std::string quote_path_(const std::filesystem::path& path) {
#if defined(_WIN32)
  return shell_quote_windows(path.string());
#else
  return shell_quote_posix(path.string());
#endif
}

std::string quiet_redirect_() {
#if defined(_WIN32)
  return " >nul 2>nul";
#else
  return " >/dev/null 2>&1";
#endif
}

bool convert_svg_to_png_(const std::filesystem::path& svg_path,
                         const std::filesystem::path& png_path,
                         std::int64_t width_px,
                         std::int64_t height_px,
                         std::int64_t dpi) {
  std::error_code ec;
  if (png_path.has_parent_path()) {
    std::filesystem::create_directories(png_path.parent_path(), ec);
  }

  const auto svg_q = quote_path_(svg_path);
  const auto png_q = quote_path_(png_path);
  const auto quiet = quiet_redirect_();
  const auto width_s = std::to_string(std::max<std::int64_t>(width_px, 1));
  const auto height_s = std::to_string(std::max<std::int64_t>(height_px, 1));
  const auto dpi_s = std::to_string(std::max<std::int64_t>(dpi, 36));

#if defined(__APPLE__)
  if (command_available_("qlmanage")) {
    const auto ql_dir = png_path.parent_path();
    const std::string cmd = "qlmanage -t -s " + std::to_string(std::max(width_px, height_px)) + " -o " +
                            quote_path_(ql_dir) + " " + svg_q + quiet;
    if (run_command_(cmd)) {
      const auto generated = ql_dir / (svg_path.stem().string() + ".png");
      if (std::filesystem::exists(generated, ec)) {
        std::filesystem::copy_file(generated, png_path, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(generated, ec);
        if (!ec && std::filesystem::exists(png_path, ec)) {
          return true;
        }
      }
    }
  }
#endif

  if (command_available_("rsvg-convert")) {
    const std::string cmd = "rsvg-convert -w " + width_s + " -h " + height_s + " -o " + png_q + " " + svg_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(png_path, ec)) {
      return true;
    }
  }

  if (command_available_("inkscape")) {
    const std::string cmd = "inkscape " + svg_q + " --export-type=png --export-filename=" + png_q +
                            " --export-width=" + width_s + " --export-height=" + height_s + quiet;
    if (run_command_(cmd) && std::filesystem::exists(png_path, ec)) {
      return true;
    }
  }

  if (command_available_("magick")) {
    const std::string cmd = "magick -density " + dpi_s + " " + svg_q + " " + png_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(png_path, ec)) {
      return true;
    }
  }

  if (command_available_("convert")) {
    const std::string cmd = "convert -density " + dpi_s + " " + svg_q + " " + png_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(png_path, ec)) {
      return true;
    }
  }

  return false;
}

bool convert_svg_to_pdf_(const std::filesystem::path& svg_path, const std::filesystem::path& pdf_path) {
  std::error_code ec;
  if (pdf_path.has_parent_path()) {
    std::filesystem::create_directories(pdf_path.parent_path(), ec);
  }

  const auto svg_q = quote_path_(svg_path);
  const auto pdf_q = quote_path_(pdf_path);
  const auto quiet = quiet_redirect_();

  if (command_available_("rsvg-convert")) {
    const std::string cmd = "rsvg-convert -f pdf -o " + pdf_q + " " + svg_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(pdf_path, ec)) {
      return true;
    }
  }

  if (command_available_("inkscape")) {
    const std::string cmd = "inkscape " + svg_q + " --export-type=pdf --export-filename=" + pdf_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(pdf_path, ec)) {
      return true;
    }
  }

  if (command_available_("magick")) {
    const std::string cmd = "magick " + svg_q + " " + pdf_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(pdf_path, ec)) {
      return true;
    }
  }

  if (command_available_("convert")) {
    const std::string cmd = "convert " + svg_q + " " + pdf_q + quiet;
    if (run_command_(cmd) && std::filesystem::exists(pdf_path, ec)) {
      return true;
    }
  }

  return false;
}

bool show_html_window_(const std::string& html,
                       const std::string& svg_payload,
                       std::string_view title,
                       std::int64_t width,
                       std::int64_t height) {
  pluss::view::HtmlWindowOptions options{};
  options.title = native_window_title_(title);
  options.snapshot_name = "plot-view.png";
  options.svg_payload = svg_payload;
  options.svg_snapshot_name = "plot-view.svg";
  options.width = std::max<std::int64_t>(640, width);
  options.height = std::max<std::int64_t>(420, height);
  options.backend_preference = plot_backend_preference_();
  return pluss::view::show_html_window(html, options);
}

bool launch_html_in_default_viewer(const std::filesystem::path& html_path) {
  const auto native = html_path.string();
#if defined(_WIN32)
  const std::string command = "cmd /c start \"\" " + shell_quote_windows(native);
#elif defined(__APPLE__)
  const std::string command = "open " + shell_quote_posix(native) + " >/dev/null 2>&1";
#else
  const std::string command = "xdg-open " + shell_quote_posix(native) + " >/dev/null 2>&1";
#endif
  return std::system(command.c_str()) == 0;
}

std::filesystem::path fallback_viewer_temp_dir_() {
  const auto dir = std::filesystem::temp_directory_path() / "plusspl_show";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void cleanup_stale_fallback_html_(const std::filesystem::path& root) {
  std::error_code ec;
  if (!std::filesystem::exists(root, ec) || ec) {
    return;
  }

  const auto now = std::chrono::system_clock::now();
  for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec || !entry.is_regular_file()) {
      continue;
    }
    const auto path = entry.path();
    if (path.extension() != ".html") {
      continue;
    }
    const auto stamp = std::filesystem::last_write_time(path, ec);
    if (ec) {
      continue;
    }
    const auto age = now - std::chrono::file_clock::to_sys(stamp);
    if (age > std::chrono::hours(36)) {
      std::filesystem::remove(path, ec);
    }
  }
}

} // namespace

std::string Studio::to_svg() const {
  return plotism::render::render_svg(*this);
}

std::string Studio::to_pdf() const {
  return plotism::render::render_pdf(*this);
}

void Studio::save_svg(std::string_view path) const {
  ensure_extension_match_(std::filesystem::path(path), ".svg", "save_svg");
  plotism::render::save_svg(*this, path);
}

void Studio::save_pdf(std::string_view path, PdfEngine engine) const {
  ensure_extension_match_(std::filesystem::path(path), ".pdf", "save_pdf");
  if (engine != PdfEngine::SvgConvert) {
    plotism::render::save_pdf(*this, path);
    return;
  }

  const std::filesystem::path out_path(path);
  std::error_code ec;
  if (out_path.has_parent_path()) {
    std::filesystem::create_directories(out_path.parent_path(), ec);
  }

  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto stem = (name_.empty() ? std::string("plusspl_plot") : name_) + "_" + std::to_string(static_cast<long long>(now));
  const auto temp_root = fallback_viewer_temp_dir_();
  std::filesystem::create_directories(temp_root, ec);
  const auto temp_svg = temp_root / (stem + ".svg");

  plotism::render::save_svg(*this, temp_svg.string());
  const bool converted = convert_svg_to_pdf_(temp_svg, out_path);
  std::filesystem::remove(temp_svg, ec);
  if (converted) {
    return;
  }

  throw std::runtime_error(
      "save_pdf(): SvgConvert engine requested, but no SVG->PDF converter was available (rsvg-convert, inkscape, magick/convert).");
}

void Studio::save_png(std::string_view path, std::int64_t dpi) const {
  ensure_extension_match_(std::filesystem::path(path), ".png", "save_png");
  if (dpi <= 0) {
    throw std::invalid_argument("save_png(): dpi must be positive");
  }

  const std::filesystem::path out_path(path);
  std::error_code ec;
  if (out_path.has_parent_path()) {
    std::filesystem::create_directories(out_path.parent_path(), ec);
  }

  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto stem = (name_.empty() ? std::string("plusspl_plot") : name_) + "_" + std::to_string(static_cast<long long>(now));
  const auto temp_root = fallback_viewer_temp_dir_();
  std::filesystem::create_directories(temp_root, ec);
  const auto temp_svg = temp_root / (stem + ".svg");

  plotism::render::save_svg(*this, temp_svg.string());

  const double scale = static_cast<double>(dpi) / 96.0;
  const auto width_px = std::max<std::int64_t>(64, static_cast<std::int64_t>(std::llround(extent_.width * scale)));
  const auto height_px = std::max<std::int64_t>(64, static_cast<std::int64_t>(std::llround(extent_.height * scale)));

  const bool converted = convert_svg_to_png_(temp_svg, out_path, width_px, height_px, dpi);
  std::filesystem::remove(temp_svg, ec);

  if (!converted) {
    throw std::runtime_error(
        "save_png(): unable to rasterize SVG. Install one of: rsvg-convert, inkscape, or ImageMagick (magick).");
  }
}

bool Studio::show(std::string_view title, std::int64_t width, std::int64_t height, bool launch) const {
  const auto window_title = title.empty() ? std::string(name_.empty() ? "PlussPl Plot" : name_) : std::string(title);
  const auto svg_payload = to_svg();
  const auto html = svg_embed_html(svg_payload, window_title, width, height);
  if (launch && show_html_window_(html, svg_payload, window_title, width, height)) {
    return true;
  }

  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto name = name_.empty() ? std::string("plusspl_plot") : name_;
  const auto output_root = fallback_viewer_temp_dir_();
  cleanup_stale_fallback_html_(output_root);
  const auto output_path = output_root /
                           (name + "_" + std::to_string(static_cast<long long>(now)) + ".html");

  std::ofstream out(output_path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.is_open()) {
    throw std::runtime_error("show(): unable to create temporary html file");
  }
  out << html;
  out.close();
  if (!out) {
    throw std::runtime_error("show(): failed while writing html payload");
  }
  if (!launch) {
    return true;
  }
  return launch_html_in_default_viewer(output_path);
}

bool Studio::show(const pluss::view::HtmlWindowOptions& options, bool launch) const {
  const auto fallback_title = name_.empty() ? std::string("PlussPl Plot") : name_;
  const auto window_title = options.title.empty() ? fallback_title : options.title;
  const auto svg_payload = to_svg();
  const auto window_width = std::max<std::int64_t>(640, options.width);
  const auto window_height = std::max<std::int64_t>(420, options.height);
  const auto html = svg_embed_html(svg_payload, window_title, window_width, window_height);

  pluss::view::HtmlWindowOptions runtime_options = options;
  runtime_options.title = window_title;
  runtime_options.width = window_width;
  runtime_options.height = window_height;
  if (runtime_options.snapshot_name.empty()) {
    runtime_options.snapshot_name = "plot-view.png";
  }
  runtime_options.svg_payload = svg_payload;
  if (runtime_options.svg_snapshot_name.empty()) {
    runtime_options.svg_snapshot_name = "plot-view.svg";
  }

  if (launch && pluss::view::show_html_window(html, runtime_options)) {
    return true;
  }

  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto name = name_.empty() ? std::string("plusspl_plot") : name_;
  const auto output_root = fallback_viewer_temp_dir_();
  cleanup_stale_fallback_html_(output_root);
  const auto output_path = output_root /
                           (name + "_" + std::to_string(static_cast<long long>(now)) + ".html");

  std::ofstream out(output_path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.is_open()) {
    throw std::runtime_error("show(): unable to create temporary html file");
  }
  out << html;
  out.close();
  if (!out) {
    throw std::runtime_error("show(): failed while writing html payload");
  }
  if (!launch) {
    return true;
  }
  return launch_html_in_default_viewer(output_path);
}

} // namespace pluss::pl
