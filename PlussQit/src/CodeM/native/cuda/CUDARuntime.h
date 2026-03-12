#pragma once

#include <cstddef>
#include <cstdint>

namespace pluss::codem::native::cuda {

enum class TransferPolicy : std::uint8_t {
  Strict = 0,
  AllowCpuBridge = 1,
};

bool runtime_available() noexcept;
const char* runtime_error_cstr() noexcept;
std::int32_t device_count() noexcept;
std::int32_t runtime_version() noexcept;
std::int32_t driver_version() noexcept;
const char* primary_device_name() noexcept;
std::int32_t primary_device_cc_major() noexcept;
std::int32_t primary_device_cc_minor() noexcept;
std::size_t primary_device_global_memory_bytes() noexcept;

bool cublas_provider_available() noexcept;
const char* cublas_provider_error_cstr() noexcept;
bool cusolver_provider_available() noexcept;
const char* cusolver_provider_error_cstr() noexcept;
bool cudnn_provider_available() noexcept;
const char* cudnn_provider_error_cstr() noexcept;

TransferPolicy transfer_policy() noexcept;
bool cpu_bridge_enabled() noexcept;
const char* transfer_policy_name(TransferPolicy policy) noexcept;

bool copy_host_to_device(void* dst, const void* src, std::size_t nbytes) noexcept;
bool copy_device_to_host(void* dst, const void* src, std::size_t nbytes) noexcept;

} // namespace pluss::codem::native::cuda
