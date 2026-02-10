#pragma once

#include <MSharp/core/Device.h>
#include <MSharp/core/DeviceType.h>
#include <MSharp/core/Allocator.h>
#include <MSharp/util/UniqueVoidPtr.h>
#include <MSharp/macros/Export.h>
#include <MSharp/util/SymInt.h>

namespace MSharp {

struct C10_API StorageImpl {
  struct use_byte_size_t {};

  StorageImpl(use_byte_size_t, const SymInt& /*size_bytes*/, Allocator* /*allocator*/, bool /*resizable*/) {}
  StorageImpl(use_byte_size_t, size_t /*size_bytes*/, pluss::DataPtr /*data_ptr*/, Allocator* /*allocator*/, bool /*resizable*/) {}

  void set_nbytes(size_t /*nbytes*/) {}
  void set_nbytes(SymInt /*nbytes*/) {}
  bool resizable() const { return false; }
  size_t nbytes() const { return 0; }
  SymInt sym_nbytes() const { return SymInt(0); }

  const void* data() const { return nullptr; }
  void* mutable_data() const { return nullptr; }
  pluss::DataPtr& mutable_data_ptr() { return data_ptr_; }
  const pluss::DataPtr& data_ptr() const { return data_ptr_; }
  pluss::DataPtr set_data_ptr(pluss::DataPtr&& dp) { return std::move(dp); }
  void set_data_ptr_noswap(pluss::DataPtr&& /*dp*/) {}

  DeviceType device_type() const { return DeviceType::CPU; }
  pluss::Allocator* allocator() const { return nullptr; }
  pluss::Device device() const { return pluss::Device(DeviceType::CPU); }

  void UniqueStorageShareExternalPointer(void* /*src*/, size_t /*capacity*/, DeleterFnPtr /*d*/ = nullptr) {}
  void UniqueStorageShareExternalPointer(pluss::DataPtr&& /*data_ptr*/, size_t /*capacity*/) {}

 private:
  pluss::DataPtr data_ptr_;
};

} // namespace MSharp
