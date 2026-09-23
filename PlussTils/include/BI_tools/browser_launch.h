#pragma once

#include <string>
#include <string_view>

// Cross-platform "open this in a real web browser" helper for PlussBI.
//
// This is deliberately independent of PlussTilsView's native WebKit/GTK/
// WebView2 window machinery (`pluss::view::show_html_window`): it never
// creates an OS window, never links a browser-engine framework, and never
// blocks on a modal event loop. It just hands a file to the user's default
// browser the same way a terminal `open`/`xdg-open`/`start` command would,
// so a dashboard opens as an ordinary, shareable browser tab.

namespace pluss::bi_tools {

// Writes `html` to a fresh temporary `.html` file (named from
// `suggested_filename`, sanitized for filesystem safety) and opens that
// file in the OS default browser. Returns the path written to on success,
// or an empty string if the file could not be written or the browser could
// not be launched.
[[nodiscard]] std::string open_html_in_browser(
    const std::string& html, std::string_view suggested_filename = "dashboard");

// Opens an existing local file path or URL directly in the OS default
// browser. Returns true if the OS accepted the open request -- this does
// not guarantee the page loaded or rendered correctly, only that a browser
// process was launched.
[[nodiscard]] bool open_path_in_browser(std::string_view path_or_url);

} // namespace pluss::bi_tools
