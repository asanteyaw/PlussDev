#include <catch2/catch_test_macros.hpp>

#include <pluss/validate.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#if defined(_WIN32)
  #include <io.h>
#else
  #include <unistd.h>
#endif

namespace {

#if defined(_WIN32)
int stderr_fd_() { return _fileno(stderr); }
int dup_fd_(int fd) { return _dup(fd); }
int dup2_fd_(int from, int to) { return _dup2(from, to); }
int close_fd_(int fd) { return _close(fd); }
#else
int stderr_fd_() { return fileno(stderr); }
int dup_fd_(int fd) { return ::dup(fd); }
int dup2_fd_(int from, int to) { return ::dup2(from, to); }
int close_fd_(int fd) { return ::close(fd); }
#endif

class ScopedStderrRedirect final {
 public:
  explicit ScopedStderrRedirect(std::filesystem::path path) : path_(std::move(path)) {
    saved_ = dup_fd_(stderr_fd_());
    REQUIRE(saved_ >= 0);
    file_ = std::freopen(path_.string().c_str(), "w", stderr);
    REQUIRE(file_ != nullptr);
  }

  ~ScopedStderrRedirect() {
    std::fflush(stderr);
    if (saved_ >= 0) {
      dup2_fd_(saved_, stderr_fd_());
      close_fd_(saved_);
    }
  }

  std::string read() const {
    std::fflush(stderr);
    std::ifstream in(path_);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }

 private:
  std::filesystem::path path_;
  int saved_ = -1;
  FILE* file_ = nullptr;
};

} // namespace

TEST_CASE("validate warn mirrors check condition semantics without throwing", "[validate][warn]") {
  const auto path = std::filesystem::temp_directory_path() / "pluss_validate_warn_test.stderr";
  ScopedStderrRedirect redirect(path);

  pluss::validate::warn(true, "this warning must not be emitted");
  REQUIRE(redirect.read().find("this warning must not be emitted") == std::string::npos);

  pluss::validate::warn(false, "conditional warning emitted");
  REQUIRE(redirect.read().find("conditional warning emitted") != std::string::npos);

  pluss::validate::warn("unconditional warning emitted");
  REQUIRE(redirect.read().find("unconditional warning emitted") != std::string::npos);
}

TEST_CASE("validate warn supports batched condition-message checks", "[validate][warn]") {
  const auto path = std::filesystem::temp_directory_path() / "pluss_validate_warn_batch_test.stderr";
  ScopedStderrRedirect redirect(path);

  pluss::validate::warn({
      {true, "batch warning must not be emitted"},
      {false, "first batch warning emitted"},
      {false, "second batch warning emitted"},
  });

  const auto output = redirect.read();
  REQUIRE(output.find("batch warning must not be emitted") == std::string::npos);
  REQUIRE(output.find("first batch warning emitted") != std::string::npos);
  REQUIRE(output.find("second batch warning emitted") != std::string::npos);
}

TEST_CASE("validate check still throws when condition is violated", "[validate][check]") {
  REQUIRE_NOTHROW(pluss::validate::check(true, "ok"));
  REQUIRE_THROWS_AS(pluss::validate::check(false, "must throw"), pluss::Error);
}
