#include <string>

#include "pluss/tils/view/html_window.h"

namespace {

std::string showcase_html() {
  std::string html = R"HTML(
<!doctype html>
<html>
<head>
  <title>PlussView Scrolling Showcase</title>
  <style>
    body { margin: 0; background: #f5f7fa; color: #17202a; font-family: sans-serif; }
    h1 { margin: 12px 8px 6px; font-size: 24px; }
    p { margin: 8px; }
    .wide-banner {
      width: 1500px;
      margin: 8px;
      padding: 10px;
      background: #ffffff;
      border: 1px solid #ccd4dd;
      white-space: nowrap;
    }
    table {
      height: 230px;
      margin: 10px 8px;
      background: #ffffff;
      border: 1px solid #b8c2cc;
    }
    th, td { padding: 8px 12px; white-space: nowrap; }
  </style>
</head>
<body>
  <h1>PlussView two-axis scrolling</h1>
  <div class="wide-banner">
    This document band is intentionally 1500 pixels wide. Trackpad, Shift-wheel,
    keyboard navigation, and the bottom document scrollbar can move it horizontally.
  </div>
  <p>Scroll while the pointer is over a table to move that table independently.</p>
  <table id="orders">
    <tr>
)HTML";

  for (int column = 0; column < 12; ++column) {
    html += "<th>Order field " + std::to_string(column) + "</th>";
  }
  html += "</tr>";
  for (int row = 0; row < 42; ++row) {
    html += "<tr>";
    for (int column = 0; column < 12; ++column) {
      html += "<td>order " + std::to_string(row) + " / value " +
              std::to_string(column) + "</td>";
    }
    html += "</tr>";
  }
  html += R"HTML(
  </table>
  <p>The second table has a separate scroll position from the first.</p>
  <table id="inventory">
    <tr>
)HTML";
  for (int column = 0; column < 10; ++column) {
    html += "<th>Inventory field " + std::to_string(column) + "</th>";
  }
  html += "</tr>";
  for (int row = 0; row < 34; ++row) {
    html += "<tr>";
    for (int column = 0; column < 10; ++column) {
      html += "<td>stock " + std::to_string(row) + " / value " +
              std::to_string(column) + "</td>";
    }
    html += "</tr>";
  }
  html += "</table>";
  for (int row = 0; row < 24; ++row) {
    html += "<p>Document continuation row " + std::to_string(row) +
            " validates full-page vertical scrolling.</p>";
  }
  html += "</body></html>";
  return html;
}

} // namespace

int main() {
  pluss::view::HtmlWindowOptions options;
  options.title = "PlussView In-House Scrolling Showcase";
  options.width = 920;
  options.height = 680;
  options.snapshot_name = "/tmp/plussview_inhouse_scroll_showcase.png";
  options.backend_preference = pluss::view::BackendPreference::PlussWebViewOnly;
  return pluss::view::show_html_window(showcase_html(), options) ? 0 : 1;
}
