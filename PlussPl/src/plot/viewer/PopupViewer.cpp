#include "plot/viewer/PlatformPopupViewer.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "plot/modules/core.h"
#include "plot/modules/detail/show_helpers.h"

#if defined(PLUSSPL_HAVE_GTK_WEBKIT) && PLUSSPL_HAVE_GTK_WEBKIT
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

namespace pluss::plotism::detail {
namespace {

bool runtime_gui_env_available_() {
#if defined(__APPLE__)
  return true;
#elif defined(_WIN32)
  return true;
#else
  const char* display = std::getenv("DISPLAY");
  const char* wayland = std::getenv("WAYLAND_DISPLAY");
  return (display != nullptr && *display != '\0') || (wayland != nullptr && *wayland != '\0');
#endif
}

std::string native_window_title_(const std::string& title) {
  return title.empty() ? std::string("PlussPl Viewer") : title;
}

bool copy_file_to_path_(const std::string& from, const std::string& to) {
  try {
    const std::filesystem::path source(from);
    const std::filesystem::path output(to);
    if (output.has_parent_path()) {
      std::filesystem::create_directories(output.parent_path());
    }
    std::filesystem::copy_file(source, output, std::filesystem::copy_options::overwrite_existing);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace

#if !(defined(PLUSSPL_HAVE_MAC_WEBKIT) && PLUSSPL_HAVE_MAC_WEBKIT)
bool mac_plot_viewer_available() {
  return false;
}

bool show_macos_plot_window(const std::string& title,
                            const std::string& svg_path,
                            std::int64_t width,
                            std::int64_t height) {
  (void)title;
  (void)svg_path;
  (void)width;
  (void)height;
  return false;
}
#endif

bool popup_plot_viewer_available() {
#if defined(PLUSSPL_HAVE_GTK_WEBKIT) && PLUSSPL_HAVE_GTK_WEBKIT
  return runtime_gui_env_available_();
#elif defined(PLUSSPL_HAVE_MAC_WEBKIT) && PLUSSPL_HAVE_MAC_WEBKIT
  return mac_plot_viewer_available();
#else
  return false;
#endif
}

bool show_plot_popup_window(const std::string& title,
                            const std::string& svg_path,
                            std::int64_t width,
                            std::int64_t height) {
#if defined(PLUSSPL_HAVE_GTK_WEBKIT) && PLUSSPL_HAVE_GTK_WEBKIT
  struct SnapshotRequest {
    std::string path;
  };

  if (!runtime_gui_env_available_()) {
    return false;
  }
  if (!gtk_init_check(nullptr, nullptr)) {
    return false;
  }

  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), native_window_title_(title).c_str());
  gtk_window_set_default_size(GTK_WINDOW(window),
                              static_cast<int>(std::max<std::int64_t>(700, width)),
                              static_cast<int>(std::max<std::int64_t>(500, height)));

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

  GtkWidget* save_svg_button = gtk_button_new_with_label("Save SVG");
  gtk_box_pack_end(GTK_BOX(toolbar), save_svg_button, FALSE, FALSE, 0);
  GtkWidget* save_png_button = gtk_button_new_with_label("Save PNG");
  gtk_box_pack_end(GTK_BOX(toolbar), save_png_button, FALSE, FALSE, 0);

  GtkWidget* scroller = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

  WebKitWebView* webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
  gchar* uri = g_filename_to_uri(svg_path.c_str(), nullptr, nullptr);
  if (uri != nullptr) {
    webkit_web_view_load_uri(webview, uri);
    g_free(uri);
  } else {
    const std::string fallback = std::string("<html><body><pre>Failed to open SVG:\n") + escape_xml(svg_path) +
                                 "</pre></body></html>";
    webkit_web_view_load_html(webview, fallback.c_str(), "file:///");
  }
  gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(webview));

  g_signal_connect(
      save_svg_button,
      "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        const auto* source_path = static_cast<const std::string*>(user_data);
        GtkWidget* dialog = gtk_file_chooser_dialog_new("Save SVG",
                                                        nullptr,
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel",
                                                        GTK_RESPONSE_CANCEL,
                                                        "_Save",
                                                        GTK_RESPONSE_ACCEPT,
                                                        nullptr);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "plot.svg");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
          char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          if (filename != nullptr) {
            copy_file_to_path_(*source_path, filename);
            g_free(filename);
          }
        }
        gtk_widget_destroy(dialog);
      }),
      const_cast<std::string*>(&svg_path));

  g_signal_connect(
      save_png_button,
      "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* webview = WEBKIT_WEB_VIEW(user_data);
        GtkWidget* dialog = gtk_file_chooser_dialog_new("Save PNG",
                                                        nullptr,
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel",
                                                        GTK_RESPONSE_CANCEL,
                                                        "_Save",
                                                        GTK_RESPONSE_ACCEPT,
                                                        nullptr);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "plot.png");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
          char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          auto* request = new SnapshotRequest{filename != nullptr ? filename : "plot.png"};
          if (filename != nullptr) {
            g_free(filename);
          }
          webkit_web_view_get_snapshot(
              webview,
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
      webview);

  gtk_widget_show_all(window);
  gtk_main();
  return true;
#elif defined(PLUSSPL_HAVE_MAC_WEBKIT) && PLUSSPL_HAVE_MAC_WEBKIT
  if (!mac_plot_viewer_available()) {
    return false;
  }
  return show_macos_plot_window(native_window_title_(title),
                                svg_path,
                                std::max<std::int64_t>(700, width),
                                std::max<std::int64_t>(500, height));
#else
  (void)title;
  (void)svg_path;
  (void)width;
  (void)height;
  return false;
#endif
}

bool open_plot_viewer(std::string_view title, std::string_view path, bool block, std::int64_t width, std::int64_t height) {
  if (block && popup_plot_viewer_available()) {
    return show_plot_popup_window(std::string(title), std::string(path), width, height);
  }
  return open_in_default_viewer(path, block);
}

} // namespace pluss::plotism::detail
