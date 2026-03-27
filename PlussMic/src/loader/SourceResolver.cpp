#include "loader/SourceResolver.h"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"

#ifndef PLUSSMIC_HAS_CURL
#define PLUSSMIC_HAS_CURL 0
#endif

#if PLUSSMIC_HAS_CURL
#include <curl/curl.h>
#endif

namespace pluss::loader::detail {

namespace {

std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) --e;
  return std::string(s.substr(b, e - b));
}

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::filesystem::path make_download_temp_path() {
  std::error_code ec;
  auto base = std::filesystem::temp_directory_path(ec);
  if (ec || base.empty()) {
    ec.clear();
    base = std::filesystem::current_path(ec);
  }
  if (base.empty()) {
    base = ".";
  }

  const auto seed = static_cast<std::uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  for (std::size_t i = 0; i < 512; ++i) {
    const auto candidate = base / fmt::format("pluss_loader_http_{}_{}.tmp", seed, i);
    std::error_code exists_ec;
    const bool exists = std::filesystem::exists(candidate, exists_ec);
    if (!exists && !exists_ec) return candidate;
  }
  PLUSS_FAIL("loader URL resolver: failed to allocate temporary file path");
  return {};
}

#if PLUSSMIC_HAS_CURL
std::string format_curl_error(CURLcode code, const char* error_buffer) {
  if (error_buffer != nullptr && error_buffer[0] != '\0') {
    return std::string(error_buffer);
  }
  return std::string(curl_easy_strerror(code));
}

std::size_t curl_write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
  if (ptr == nullptr || userdata == nullptr) return 0;
  auto* out = static_cast<std::ofstream*>(userdata);
  const auto total = size * nmemb;
  out->write(ptr, static_cast<std::streamsize>(total));
  return out->good() ? total : 0;
}

void ensure_curl_global_initialized() {
  static const bool initialized = []() {
    const auto rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    return rc == CURLE_OK;
  }();
  PLUSS_CHECK(initialized, "loader URL resolver: curl global initialization failed");
}

void download_to_file(std::string_view url, const std::filesystem::path& target, std::string_view where) {
  ensure_curl_global_initialized();

  std::ofstream out(target, std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(), std::string(where) + ": failed to create temp file '" + target.string() + "'");

  std::array<char, CURL_ERROR_SIZE> error_buffer{};
  CURL* curl = curl_easy_init();
  PLUSS_CHECK(curl != nullptr, std::string(where) + ": curl easy init failed");

  const std::string url_str(url);
  curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer.data());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "pluss-loader/1.0");

  const auto curl_rc = curl_easy_perform(curl);
  long http_status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
  curl_easy_cleanup(curl);
  out.flush();

  if (curl_rc != CURLE_OK) {
    out.close();
    std::error_code rm_ec;
    std::filesystem::remove(target, rm_ec);
    PLUSS_FAIL(std::string(where) + ": failed to download URL '" + url_str + "' (" +
               format_curl_error(curl_rc, error_buffer.data()) + ")");
  }
  PLUSS_CHECK(static_cast<bool>(out),
              std::string(where) + ": failed to persist downloaded bytes for URL '" + url_str + "'");
  if (http_status >= 400) {
    out.close();
    std::error_code rm_ec;
    std::filesystem::remove(target, rm_ec);
    PLUSS_FAIL(std::string(where) + ": URL request failed with HTTP status " + std::to_string(http_status) +
               " for '" + url_str + "'");
  }
}
#endif

} // namespace

ResolvedSource::ResolvedSource(std::string local_path)
    : local_path_(std::move(local_path)) {}

ResolvedSource::ResolvedSource(std::string local_path, std::string cleanup_path)
    : local_path_(std::move(local_path)), cleanup_path_(std::move(cleanup_path)) {}

ResolvedSource::ResolvedSource(ResolvedSource&& other) noexcept = default;

ResolvedSource& ResolvedSource::operator=(ResolvedSource&& other) noexcept = default;

ResolvedSource::~ResolvedSource() {
  if (!cleanup_path_.empty()) {
    std::error_code ec;
    std::filesystem::remove(cleanup_path_, ec);
  }
}

const std::string& ResolvedSource::local_path() const noexcept {
  return local_path_;
}

bool is_http_url(std::string_view filepath) {
  const auto normalized = lower_copy(trim_ascii(filepath));
  return normalized.rfind("http://", 0) == 0 || normalized.rfind("https://", 0) == 0;
}

ResolvedSource resolve_local_or_http_source(std::string_view filepath, std::string_view where) {
  const auto normalized = trim_ascii(filepath);
  PLUSS_CHECK(!normalized.empty(), std::string(where) + ": filepath is empty");
  if (!is_http_url(normalized)) {
    return ResolvedSource(normalized);
  }

#if PLUSSMIC_HAS_CURL
  const auto temp_path = make_download_temp_path();
  download_to_file(normalized, temp_path, where);
  return ResolvedSource(temp_path.string(), temp_path.string());
#else
  PLUSS_FAIL(std::string(where) + ": URL input requires libcurl support (rebuild with CURL available)");
  return ResolvedSource(std::string(normalized));
#endif
}

} // namespace pluss::loader::detail
