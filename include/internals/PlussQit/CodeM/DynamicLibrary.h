#pragma once

#include <PlussQit/Utils.h>
#include <MSharp/macros/Export.h>
#include <MSharp/util/Exception.h>

namespace MSharp {

class DynamicLibraryError : public Error {
  using Error::Error;
};

} // namespace MSharp

namespace pluss {

struct DynamicLibrary {
  AT_DISALLOW_COPY_AND_ASSIGN(DynamicLibrary);
  DynamicLibrary(DynamicLibrary&& other) = delete;
  DynamicLibrary& operator=(DynamicLibrary&&) = delete;

  PLUSS_API DynamicLibrary(
      const char* name,
      const char* alt_name = nullptr,
      bool leak_handle = false);

  PLUSS_API void* sym(const char* name);

  PLUSS_API ~DynamicLibrary();

 private:
  bool leak_handle;
  void* handle = nullptr;
};

} // namespace pluss
