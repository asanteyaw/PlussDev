#pragma once

#include <string>
#include <string_view>

namespace pluss::loader::detail {

class ResolvedSource {
public:
  explicit ResolvedSource(std::string local_path);
  ResolvedSource(std::string local_path, std::string cleanup_path);
  ResolvedSource(const ResolvedSource&) = delete;
  ResolvedSource& operator=(const ResolvedSource&) = delete;
  ResolvedSource(ResolvedSource&& other) noexcept;
  ResolvedSource& operator=(ResolvedSource&& other) noexcept;
  ~ResolvedSource();

  [[nodiscard]] const std::string& local_path() const noexcept;

private:
  std::string local_path_{};
  std::string cleanup_path_{};
};

[[nodiscard]] bool is_http_url(std::string_view filepath);
[[nodiscard]] ResolvedSource resolve_local_or_http_source(std::string_view filepath,
                                                          std::string_view where);

} // namespace pluss::loader::detail
