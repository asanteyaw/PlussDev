#include "PlatformHtmlViewer.h"

#if defined(PLUSSVIEW_HAVE_GTK_WEBKIT) && PLUSSVIEW_HAVE_GTK_WEBKIT

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace pluss::view::detail {

namespace {

struct SnapshotRequest {
  std::string path;
};

struct SavePngContext {
  WebKitWebView* webview{nullptr};
  const struct ViewerData* data{nullptr};
};

struct ViewerData {
  std::string html_payload;
  std::string svg_payload;
  std::string png_snapshot_name;
  std::string svg_snapshot_name;
};

bool runtime_gui_env_available_() {
  return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
}

std::optional<std::string> extract_svg_fragment_(std::string_view html) {
  const auto begin = html.find("<svg");
  if (begin == std::string_view::npos) {
    return std::nullopt;
  }
  const auto end_tag = html.find("</svg>", begin);
  if (end_tag == std::string_view::npos) {
    return std::nullopt;
  }
  const auto end = end_tag + std::string_view("</svg>").size();
  return std::string(html.substr(begin, end - begin));
}

bool write_svg_document_(std::string_view svg_payload, const std::filesystem::path& output_path) {
  std::ofstream out(output_path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.is_open()) {
    return false;
  }
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  out << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
         "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
  out << svg_payload;
  out.close();
  return static_cast<bool>(out);
}

} // namespace

bool gtk_html_viewer_available() {
  return true;
}

bool mac_html_viewer_available() {
  return false;
}

bool windows_html_viewer_available() {
  return false;
}

bool show_macos_html_window(const std::string& title,
                            const std::string& html,
                            std::int64_t width,
                            std::int64_t height,
                            const std::string& snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  return false;
}

bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  return false;
}

bool show_gtk_html_window(const std::string& title,
                          const std::string& html,
                          std::int64_t width,
                          std::int64_t height,
                          const std::string& snapshot_name,
                          const std::string& svg_payload,
                          const std::string& svg_snapshot_name) {
  if (!runtime_gui_env_available_()) {
    return false;
  }
  if (!gtk_init_check(nullptr, nullptr)) {
    return false;
  }

  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title.c_str());
  gtk_window_set_default_size(GTK_WINDOW(window),
                              static_cast<int>(std::max<std::int64_t>(640, width)),
                              static_cast<int>(std::max<std::int64_t>(420, height)));

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
  g_signal_connect(window,
                   "key-press-event",
                   G_CALLBACK(+[](GtkWidget*, GdkEventKey* event, gpointer) -> gboolean {
                     if (event->keyval == GDK_KEY_Escape) {
                       gtk_main_quit();
                       return TRUE;
                     }
                     return FALSE;
                   }),
                   nullptr);

  GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), root);

  GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_top(toolbar, 10);
  gtk_widget_set_margin_bottom(toolbar, 6);
  gtk_widget_set_margin_start(toolbar, 12);
  gtk_widget_set_margin_end(toolbar, 12);
  gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

  GtkWidget* save_png_button = gtk_button_new_with_label("Save PNG");
  gtk_box_pack_end(GTK_BOX(toolbar), save_png_button, FALSE, FALSE, 0);

  GtkWidget* scroller = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

  WebKitWebView* webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
  webkit_web_view_load_html(webview, html.c_str(), "file:///");
  gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(webview));

  ViewerData viewer_data{html,
                         svg_payload,
                         snapshot_name.empty() ? std::string("view.png") : snapshot_name,
                         svg_snapshot_name.empty() ? std::string("view.svg") : svg_snapshot_name};
  SavePngContext save_png_context{webview, &viewer_data};

  g_signal_connect(
      save_png_button,
      "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        const auto* context = static_cast<const SavePngContext*>(user_data);
        if (context == nullptr || context->webview == nullptr || context->data == nullptr) {
          return;
        }
        GtkWidget* dialog = gtk_file_chooser_dialog_new("Save Snapshot",
                                                        nullptr,
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel",
                                                        GTK_RESPONSE_CANCEL,
                                                        "_Save",
                                                        GTK_RESPONSE_ACCEPT,
                                                        nullptr);

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), context->data->png_snapshot_name.c_str());
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
          char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          auto* request = new SnapshotRequest{filename != nullptr ? filename : context->data->png_snapshot_name};
          if (filename != nullptr) {
            g_free(filename);
          }
          webkit_web_view_get_snapshot(
              context->webview,
              WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
              WEBKIT_SNAPSHOT_OPTIONS_NONE,
              nullptr,
              +[](GObject* source, GAsyncResult* result, gpointer user_data) {
                std::unique_ptr<SnapshotRequest> request(static_cast<SnapshotRequest*>(user_data));
                GError* error = nullptr;
                cairo_surface_t* surface =
                    webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(source), result, &error);
                if (error != nullptr) {
                  g_error_free(error);
                  return;
                }
                if (surface != nullptr) {
                  cairo_surface_write_to_png(surface, request->path.c_str());
                  cairo_surface_destroy(surface);
                }
              },
              request);
        }
        gtk_widget_destroy(dialog);
      }),
      &save_png_context);

  const bool can_save_svg = !viewer_data.svg_payload.empty() || extract_svg_fragment_(viewer_data.html_payload).has_value();
  if (can_save_svg) {
    GtkWidget* save_svg_button = gtk_button_new_with_label("Save SVG");
    gtk_box_pack_end(GTK_BOX(toolbar), save_svg_button, FALSE, FALSE, 0);

    g_signal_connect(
        save_svg_button,
        "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer user_data) {
          const auto* data = static_cast<const ViewerData*>(user_data);
          if (data == nullptr) {
            return;
          }

          GtkWidget* dialog = gtk_file_chooser_dialog_new("Save SVG",
                                                          nullptr,
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          "_Cancel",
                                                          GTK_RESPONSE_CANCEL,
                                                          "_Save",
                                                          GTK_RESPONSE_ACCEPT,
                                                          nullptr);
          gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), data->svg_snapshot_name.c_str());
          if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename != nullptr) {
              std::string payload = data->svg_payload;
              if (payload.empty()) {
                const auto extracted = extract_svg_fragment_(data->html_payload);
                if (extracted.has_value()) {
                  payload = *extracted;
                }
              }
              if (!payload.empty()) {
                (void)write_svg_document_(payload, std::filesystem::path(filename));
              }
              g_free(filename);
            }
          }
          gtk_widget_destroy(dialog);
        }),
        &viewer_data);
  }

  gtk_widget_show_all(window);
  gtk_main();
  return true;
}

} // namespace pluss::view::detail

#else

namespace pluss::view::detail {

bool gtk_html_viewer_available() {
  return false;
}

bool show_gtk_html_window(const std::string& title,
                          const std::string& html,
                          std::int64_t width,
                          std::int64_t height,
                          const std::string& snapshot_name,
                          const std::string& svg_payload,
                          const std::string& svg_snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  (void)svg_payload;
  (void)svg_snapshot_name;
  return false;
}

bool mac_html_viewer_available() {
  return false;
}

bool show_macos_html_window(const std::string& title,
                            const std::string& html,
                            std::int64_t width,
                            std::int64_t height,
                            const std::string& snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  return false;
}

bool windows_html_viewer_available() {
  return false;
}

bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  return false;
}

} // namespace pluss::view::detail

#endif
