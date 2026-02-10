#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <PlussQit/core/Series.h>
#include <PlussQit/core/dispatch/Dispatcher.h>
#include <PlussQit/core/ivalue.h>
#include <MSharp/core/CPUAllocator.h>
#include <MSharp/util/irange.h>

template <class... Inputs>
inline std::vector<MSharp::IValue> makeStack(Inputs&&... inputs) {
  return {std::forward<Inputs>(inputs)...};
}

inline pluss::Series dummyTensor(
    MSharp::DispatchKeySet ks,
    bool requires_grad = false) {
  auto* allocator = MSharp::GetCPUAllocator();
  int64_t nelements = 1;
  auto dtype = caffe2::TypeMeta::Make<float>();
  int64_t size_bytes = nelements * dtype.itemsize();
  auto storage_impl = MSharp::make_intrusive<MSharp::StorageImpl>(
      MSharp::StorageImpl::use_byte_size_t(),
      size_bytes,
      allocator->allocate(size_bytes),
      allocator,
      /*resizable=*/true);
  pluss::Series t =
      pluss::detail::make_tensor<MSharp::TensorImpl>(storage_impl, ks, dtype);
  // TODO: We add this to simulate the ideal case where we only have Autograd
  // backend keys
  //       on Series when it requires grad. But currently Autograd keys are
  //       added in TensorImpl constructor by default.
  if (!requires_grad) {
    t.unsafeGetTensorImpl()->remove_autograd_key();
  }
  return t;
}

inline pluss::Series dummyTensor(
    MSharp::DispatchKey dispatch_key,
    bool requires_grad = false) {
  return dummyTensor(MSharp::DispatchKeySet(dispatch_key), requires_grad);
}

template <class... Args>
inline std::vector<MSharp::IValue> callOp(
    const MSharp::OperatorHandle& op,
    Args... args) {
  auto stack = makeStack(std::forward<Args>(args)...);
  op.callBoxed(&stack);
  return stack;
}

template <class Result, class... Args>
inline Result callOpUnboxed(const MSharp::OperatorHandle& op, Args... args) {
  return op.typed<Result(Args...)>().call(std::forward<Args>(args)...);
}

template <class Result, class... Args>
inline Result callOpUnboxedWithDispatchKey(
    const MSharp::OperatorHandle& op,
    MSharp::DispatchKey dispatchKey,
    Args... args) {
  return op.typed<Result(Args...)>().callWithDispatchKey(
      dispatchKey, std::forward<Args>(args)...);
}

template <class Result, class... Args>
inline Result callOpUnboxedWithPrecomputedDispatchKeySet(
    const MSharp::OperatorHandle& op,
    MSharp::DispatchKeySet ks,
    Args... args) {
  return op.typed<Result(Args...)>().redispatch(
      ks, std::forward<Args>(args)...);
}

inline void expectDoesntFindKernel(
    const char* op_name,
    MSharp::DispatchKey dispatch_key) {
  auto op = MSharp::Dispatcher::singleton().findSchema({op_name, ""});
  EXPECT_ANY_THROW(callOp(*op, dummyTensor(dispatch_key), 5););
}

inline void expectDoesntFindOperator(const char* op_name) {
  auto op = MSharp::Dispatcher::singleton().findSchema({op_name, ""});
  EXPECT_FALSE(op.has_value());
}

template <class Exception, class Functor>
inline void expectThrows(Functor&& functor, const char* expectMessageContains) {
  try {
    std::forward<Functor>(functor)();
  } catch (const Exception& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr(expectMessageContains));
    return;
  }
  ADD_FAILURE() << "Expected to throw exception containing \""
                << expectMessageContains << "\" but didn't throw";
}

template <class T, size_t N>
void expectListEquals(MSharp::ArrayRef<T> expected, std::array<T, N> actual) {
  EXPECT_EQ(expected.size(), actual.size());
  for (const auto i : MSharp::irange(expected.size())) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}

template <class T>
void expectListEquals(MSharp::ArrayRef<T> expected, MSharp::ArrayRef<T> actual) {
  EXPECT_EQ(expected.size(), actual.size());
  for (const auto i : MSharp::irange(expected.size())) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}

template <class T>
void expectListEquals(MSharp::ArrayRef<T> expected, MSharp::List<T> actual) {
  EXPECT_EQ(expected.size(), actual.size());
  for (const auto i : MSharp::irange(expected.size())) {
    EXPECT_EQ(expected[i], actual.get(i));
  }
}

template <class T>
void expectListEquals(MSharp::ArrayRef<T> expected, std::vector<T> actual) {
  EXPECT_EQ(expected.size(), actual.size());
  for (const auto i : MSharp::irange(expected.size())) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}

// NB: This is not really sound, but all of the type sets constructed here
// are singletons so it's fine
static inline MSharp::DispatchKey extractDispatchKey(const pluss::Series& t) {
  return legacyExtractDispatchKey(t.key_set());
}
