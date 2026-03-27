#if defined(_WIN32) && defined(PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2) && PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2

#include "PlatformHtmlViewer.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <shlwapi.h>
#include <wrl.h>
#include <WebView2.h>

namespace pluss::view::detail {

namespace {

using Microsoft::WRL::ComPtr;

constexpr wchar_t kWindowClassName[] = L"PlussTilsViewHtmlViewerWindow";
constexpr int kToolbarHeight = 44;
constexpr int kButtonWidth = 92;
constexpr int kButtonHeight = 28;
constexpr int kButtonMargin = 12;

std::wstring utf8_to_wstring_(const std::string& text) {
  if (text.empty()) {
    return std::wstring();
  }
  const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (count <= 0) {
    return std::wstring();
  }
  std::wstring out(static_cast<std::size_t>(count - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), count);
  return out;
}

struct ScopedCoInit {
  HRESULT hr{E_FAIL};
  bool initialized{false};

  ScopedCoInit() {
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    initialized = hr == S_OK || hr == S_FALSE;
  }

  ~ScopedCoInit() {
    if (initialized) {
      CoUninitialize();
    }
  }
};

struct ViewerState {
  std::wstring title;
  std::wstring html;
  std::wstring snapshot_name;
  HWND hwnd{nullptr};
  HWND save_button{nullptr};
  ComPtr<ICoreWebView2Environment> environment;
  ComPtr<ICoreWebView2Controller> controller;
  ComPtr<ICoreWebView2> webview;
  bool created{false};
  bool success{false};
};

void layout_window_(ViewerState* state) {
  if (state == nullptr || state->hwnd == nullptr) {
    return;
  }

  RECT client{};
  GetClientRect(state->hwnd, &client);

  if (state->save_button != nullptr) {
    MoveWindow(state->save_button,
               std::max(0L, client.right - kButtonWidth - kButtonMargin),
               (kToolbarHeight - kButtonHeight) / 2,
               kButtonWidth,
               kButtonHeight,
               TRUE);
  }

  if (state->controller) {
    RECT bounds{};
    bounds.left = 0;
    bounds.top = kToolbarHeight;
    bounds.right = client.right;
    bounds.bottom = client.bottom;
    state->controller->put_Bounds(bounds);
  }
}

void save_png_(ViewerState* state) {
  if (state == nullptr || !state->webview) {
    return;
  }

  const std::wstring default_name = state->snapshot_name.empty() ? std::wstring(L"view.png") : state->snapshot_name;
  wchar_t path[MAX_PATH] = {};
  wcsncpy(path, default_name.c_str(), MAX_PATH - 1);

  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = state->hwnd;
  ofn.lpstrFilter = L"PNG Files\0*.png\0All Files\0*.*\0";
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = L"png";
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

  if (!GetSaveFileNameW(&ofn)) {
    return;
  }

  ComPtr<IStream> stream;
  if (FAILED(SHCreateStreamOnFileEx(path,
                                    STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_NONE,
                                    FILE_ATTRIBUTE_NORMAL,
                                    TRUE,
                                    nullptr,
                                    &stream))) {
    return;
  }

  state->webview->CapturePreview(
      COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG,
      stream.Get(),
      Microsoft::WRL::Callback<ICoreWebView2CapturePreviewCompletedHandler>(
          [stream](HRESULT result) -> HRESULT {
            if (SUCCEEDED(result) && stream) {
              stream->Commit(STGC_DEFAULT);
            }
            return S_OK;
          })
          .Get());
}

LRESULT CALLBACK viewer_wnd_proc_(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  auto* state = reinterpret_cast<ViewerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  if (message == WM_NCCREATE) {
    auto* create = reinterpret_cast<LPCREATESTRUCTW>(lparam);
    state = static_cast<ViewerState*>(create->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    if (state != nullptr) {
      state->hwnd = hwnd;
    }
  }

  switch (message) {
    case WM_CREATE: {
      if (state == nullptr) {
        return -1;
      }

      state->save_button = CreateWindowW(L"BUTTON",
                                         L"Save PNG",
                                         WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         0,
                                         0,
                                         kButtonWidth,
                                         kButtonHeight,
                                         hwnd,
                                         reinterpret_cast<HMENU>(1001),
                                         GetModuleHandleW(nullptr),
                                         nullptr);

      const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
          nullptr,
          nullptr,
          nullptr,
          Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
              [state](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result) || environment == nullptr) {
                  state->created = true;
                  state->success = false;
                  DestroyWindow(state->hwnd);
                  return S_OK;
                }

                state->environment = environment;
                environment->CreateCoreWebView2Controller(
                    state->hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [state](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                          if (FAILED(result) || controller == nullptr) {
                            state->created = true;
                            state->success = false;
                            DestroyWindow(state->hwnd);
                            return S_OK;
                          }

                          state->controller = controller;
                          controller->get_CoreWebView2(&state->webview);
                          if (!state->webview) {
                            state->created = true;
                            state->success = false;
                            DestroyWindow(state->hwnd);
                            return S_OK;
                          }

                          state->webview->NavigateToString(state->html.c_str());
                          layout_window_(state);
                          ShowWindow(state->hwnd, SW_SHOW);
                          UpdateWindow(state->hwnd);
                          state->created = true;
                          state->success = true;
                          return S_OK;
                        })
                        .Get());
                return S_OK;
              })
              .Get());

      if (FAILED(hr)) {
        state->created = true;
        state->success = false;
        return -1;
      }
      return 0;
    }
    case WM_COMMAND:
      if (LOWORD(wparam) == 1001 && HIWORD(wparam) == BN_CLICKED) {
        save_png_(state);
        return 0;
      }
      break;
    case WM_SIZE:
      layout_window_(state);
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }

  return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool ensure_window_class_registered_() {
  static bool registered = false;
  if (registered) {
    return true;
  }

  WNDCLASSW wc{};
  wc.lpfnWndProc = viewer_wnd_proc_;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = kWindowClassName;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

  if (RegisterClassW(&wc) == 0) {
    return false;
  }
  registered = true;
  return true;
}

} // namespace

bool windows_html_viewer_available() {
  LPWSTR version = nullptr;
  const HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
  if (version != nullptr) {
    CoTaskMemFree(version);
  }
  return SUCCEEDED(hr);
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

bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name) {
  ScopedCoInit coinit;
  if (!coinit.initialized && coinit.hr != RPC_E_CHANGED_MODE) {
    return false;
  }
  if (!windows_html_viewer_available()) {
    return false;
  }
  if (!ensure_window_class_registered_()) {
    return false;
  }

  ViewerState state;
  state.title = utf8_to_wstring_(title);
  state.html = utf8_to_wstring_(html);
  state.snapshot_name = utf8_to_wstring_(snapshot_name);
  if (state.title.empty() || state.html.empty()) {
    return false;
  }

  const int viewer_width = static_cast<int>(std::max<std::int64_t>(640, width));
  const int viewer_height = static_cast<int>(std::max<std::int64_t>(420, height));

  HWND hwnd = CreateWindowExW(0,
                              kWindowClassName,
                              state.title.c_str(),
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              viewer_width,
                              viewer_height,
                              nullptr,
                              nullptr,
                              GetModuleHandleW(nullptr),
                              &state);
  if (hwnd == nullptr) {
    return false;
  }

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return state.created && state.success;
}

} // namespace pluss::view::detail

#endif
