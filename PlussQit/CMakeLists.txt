cmake_minimum_required(VERSION 3.25)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
  include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PlussQit LANGUAGES CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

option(PLUSS_ENABLE_TORCH "Enable LibTorch interop" OFF)
if(PLUSS_ENABLE_TORCH)
  if(NOT Torch_DIR)
    set(_pluss_torch_candidates "")

    if(DEFINED ENV{TORCH_SITE} AND NOT "$ENV{TORCH_SITE}" STREQUAL "")
      list(APPEND _pluss_torch_candidates "$ENV{TORCH_SITE}/share/cmake/Torch")
    endif()
    if(DEFINED ENV{TORCH_DIR} AND NOT "$ENV{TORCH_DIR}" STREQUAL "")
      list(APPEND _pluss_torch_candidates "$ENV{TORCH_DIR}")
    endif()

    if(DEFINED ENV{CONDA_PREFIX} AND NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
      file(GLOB _pluss_torch_from_conda "$ENV{CONDA_PREFIX}/lib/python*/site-packages/torch/share/cmake/Torch")
      list(FILTER _pluss_torch_from_conda INCLUDE REGEX ".*/python[0-9]+\\.[0-9]+/site-packages/torch/share/cmake/Torch$")
      list(APPEND _pluss_torch_candidates ${_pluss_torch_from_conda})
    endif()

    if(Python3_EXECUTABLE)
      get_filename_component(_pluss_py_bin "${Python3_EXECUTABLE}" DIRECTORY)
      get_filename_component(_pluss_py_prefix "${_pluss_py_bin}" DIRECTORY)
      execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"
        RESULT_VARIABLE _pluss_py_ver_rc
        OUTPUT_VARIABLE _pluss_py_ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
      if(_pluss_py_ver_rc EQUAL 0 AND _pluss_py_ver)
        list(APPEND _pluss_torch_candidates
          "${_pluss_py_prefix}/lib/python${_pluss_py_ver}/site-packages/torch/share/cmake/Torch")
      endif()

      file(GLOB _pluss_torch_from_py_prefix "${_pluss_py_prefix}/lib/python*/site-packages/torch/share/cmake/Torch")
      list(FILTER _pluss_torch_from_py_prefix INCLUDE REGEX ".*/python[0-9]+\\.[0-9]+/site-packages/torch/share/cmake/Torch$")
      list(APPEND _pluss_torch_candidates ${_pluss_torch_from_py_prefix})
    endif()

    list(REMOVE_DUPLICATES _pluss_torch_candidates)
    foreach(_pluss_torch_cand IN LISTS _pluss_torch_candidates)
      if(EXISTS "${_pluss_torch_cand}/TorchConfig.cmake")
        set(Torch_DIR "${_pluss_torch_cand}" CACHE PATH "Path to Torch CMake package" FORCE)
        message(STATUS "Auto-detected Torch_DIR: ${Torch_DIR}")
        break()
      endif()
    endforeach()

    unset(_pluss_torch_candidates)
    unset(_pluss_torch_from_conda)
    unset(_pluss_torch_from_py_prefix)
    unset(_pluss_torch_cand)
    unset(_pluss_py_bin)
    unset(_pluss_py_prefix)
    unset(_pluss_py_ver)
    unset(_pluss_py_ver_rc)
  endif()

  find_package(Torch REQUIRED)
  add_compile_definitions(PLUSS_ENABLE_TORCH)
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLUSSQIT_BUILD_TESTS "Build tests" ON)
option(PLUSSQIT_BUILD_EXAMPLES "Build examples" ON)
option(PLUSSQIT_USE_FMT "Use fmt for messages" ON)
option(PLUSSQIT_FETCH_DEPS "Allow CMake to FetchContent dependencies (fmt/Catch2)" OFF)
option(PLUSSQIT_BUILD_CODEM "Build CodeM (Series/Tensor)" OFF)
option(PLUSSQIT_BUILD_CODEH "Build CodeH (Timeline/Duration/TimeIndex)" ${PLUSSQIT_BUILD_CODEM})
option(PLUSSQIT_BUILD_CODEM_SMOKE_TESTS "Build ad-hoc CodeM smoke executables" OFF)
option(PLUSSQIT_CODEN_ENABLE_SERIES_INTEROP "Enable CodeN helpers that return CodeM::Series" OFF)
option(PLUSSQIT_ENABLE_CUDA "Enable CUDA runtime probing for CodeM (kernels still WIP)" OFF)
option(BUILD_BENCHMARKS "Build benchmark executables" OFF)
set(PLUSSQIT_CPU_BLAS_BACKEND "AUTO" CACHE STRING
    "CodeM CPU BLAS backend for dense matmul: AUTO|ACCELERATE|OPENBLAS|ONEMKL|NONE")
set_property(CACHE PLUSSQIT_CPU_BLAS_BACKEND PROPERTY STRINGS
             AUTO ACCELERATE OPENBLAS ONEMKL NONE)

set(PLUSSQIT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PLUSSQIT_GEN_DIR})

set(PLUSSMA_INCLUDE_DIR "")
if (EXISTS "${CMAKE_SOURCE_DIR}/PlussMa/include")
  set(PLUSSMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/PlussMa/include")
elseif (EXISTS "${CMAKE_SOURCE_DIR}/../PlussMa/include")
  set(PLUSSMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../PlussMa/include")
endif()

set(PLUSSTILS_INCLUDE_DIR "")
if (EXISTS "${CMAKE_SOURCE_DIR}/PlussTils/include")
  set(PLUSSTILS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/PlussTils/include")
elseif (EXISTS "${CMAKE_SOURCE_DIR}/../PlussTils/include")
  set(PLUSSTILS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../PlussTils/include")
endif()

include(FetchContent)

set(PLUSSQIT_FMT_INCLUDE_DIR "" CACHE INTERNAL "Resolved fmt include directory for PlussQit SDK staging")

if (PLUSSQIT_USE_FMT)
  set(_plussqit_fmt_candidates
    "${CMAKE_CURRENT_LIST_DIR}/../ExLibs/fmt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ExLibs/fmt/include"
    "${CMAKE_SOURCE_DIR}/ExLibs/fmt/include"
    "${CMAKE_SOURCE_DIR}/../ExLibs/fmt/include"
  )
  foreach(_plussqit_fmt_candidate IN LISTS _plussqit_fmt_candidates)
    if(EXISTS "${_plussqit_fmt_candidate}/fmt/format.h")
      set(PLUSSQIT_FMT_INCLUDE_DIR "${_plussqit_fmt_candidate}" CACHE INTERNAL "Resolved fmt include directory for PlussQit SDK staging" FORCE)
      break()
    endif()
  endforeach()

  if (NOT PLUSSQIT_FMT_INCLUDE_DIR)
    find_package(fmt QUIET)
    if (NOT fmt_FOUND AND PLUSSQIT_FETCH_DEPS)
      FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
      )
      FetchContent_MakeAvailable(fmt)
    endif()

    if (TARGET fmt::fmt-header-only)
      get_target_property(_plussqit_fmt_target_includes fmt::fmt-header-only INTERFACE_INCLUDE_DIRECTORIES)
    elseif (TARGET fmt::fmt)
      get_target_property(_plussqit_fmt_target_includes fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    endif()

    if (_plussqit_fmt_target_includes)
      foreach(_plussqit_fmt_target_include IN LISTS _plussqit_fmt_target_includes)
        if (EXISTS "${_plussqit_fmt_target_include}/fmt/format.h")
          set(PLUSSQIT_FMT_INCLUDE_DIR "${_plussqit_fmt_target_include}" CACHE INTERNAL "Resolved fmt include directory for PlussQit SDK staging" FORCE)
          break()
        endif()
      endforeach()
    endif()
  endif()

  if (NOT PLUSSQIT_FMT_INCLUDE_DIR)
    message(STATUS "fmt headers not found; disabling fmt support (install fmt, vendor ExLibs/fmt, or set PLUSSQIT_FETCH_DEPS=ON).")
    set(PLUSSQIT_USE_FMT OFF)
  endif()
endif()


# ---- CodeM (ATen-like) ----
if (PLUSSQIT_BUILD_CODEM)
if(APPLE)
  enable_language(OBJCXX)
endif()
set(PLUSSQIT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(SERIES_GEN_DIR ${PLUSSQIT_GEN_DIR})
file(MAKE_DIRECTORY ${PLUSSQIT_GEN_DIR})

set(CODEM_NATIVE_YAML ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/native/native_functions.yaml)
set(CODEM_GEN_SCRIPT  ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ops.py)
set(SERIES_METHODS_GEN_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/generate_series_methods_gen_h.py)
set(SERIES_METHODS_SEED_LIST ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/series_methods_seed.txt)
set(SERIES_METHODS_TYPED_YAML ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/series_methods_typed.yaml)


set(SERIES_OPSINTERNAL_GEN_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/generate_series_methods_from_opsinternal.py)
set(SERIES_OPSINTERNAL_H ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/OpsInternal.h)
set(SERIES_OPSINTERNAL_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/OpsInternal.cpp)

set(SERIES_OPSINTERNAL_DECLS ${PLUSSQIT_GEN_DIR}/CodeM/Series/SeriesOpsInternalMethods.inc)
set(SERIES_OPSINTERNAL_DEFS  ${PLUSSQIT_GEN_DIR}/CodeM/Series/SeriesOpsInternalMethods.cpp.inc)

add_custom_command(
  OUTPUT ${SERIES_OPSINTERNAL_DECLS} ${SERIES_OPSINTERNAL_DEFS}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUSSQIT_GEN_DIR}/CodeM/Series
  COMMAND ${Python3_EXECUTABLE} ${SERIES_OPSINTERNAL_GEN_SCRIPT}
          --opsinternal ${SERIES_OPSINTERNAL_H}
          --series-header ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/Series.h
          --out-decls ${SERIES_OPSINTERNAL_DECLS}
          --out-defs  ${SERIES_OPSINTERNAL_DEFS}
  DEPENDS ${SERIES_OPSINTERNAL_H}
          ${SERIES_OPSINTERNAL_CPP}
          ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/Series.h
          ${SERIES_OPSINTERNAL_GEN_SCRIPT}
  COMMENT "Generating Series method wrappers from OpsInternal.h"
)

set(SERIES_METHODS_GEN_HEADER ${PLUSSQIT_GEN_DIR}/CodeM/generated/SeriesMethodsGen.h)
add_custom_command(
  OUTPUT ${SERIES_METHODS_GEN_HEADER}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUSSQIT_GEN_DIR}/CodeM/generated
  COMMAND ${Python3_EXECUTABLE} ${SERIES_METHODS_GEN_SCRIPT}
          --yaml ${CODEM_NATIVE_YAML}
          --opsinternal ${SERIES_OPSINTERNAL_H}
          --seed-list ${SERIES_METHODS_SEED_LIST}
          --typed-yaml ${SERIES_METHODS_TYPED_YAML}
          --out ${SERIES_METHODS_GEN_HEADER}
  DEPENDS ${CODEM_NATIVE_YAML} ${SERIES_OPSINTERNAL_H} ${SERIES_METHODS_SEED_LIST} ${SERIES_METHODS_TYPED_YAML} ${SERIES_METHODS_GEN_SCRIPT}
  COMMENT "Generating SeriesMethodsGen.h from native_functions.yaml"
)

add_custom_command(
  OUTPUT
    ${SERIES_GEN_DIR}/CodeM/ops/add_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sub_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/mul_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/div_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/neg_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/abs_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/exp_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/log_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sqrt_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sin_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/cos_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/tanh_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sum_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/mean_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/amax_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/amin_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/argmax_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/argmin_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/index_select_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/masked_select_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/matmul_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/gather_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/scatter_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/scatter_add_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/where_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/masked_fill_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/nonzero_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/take_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/put_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/index_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/index_put_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/cat_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/stack_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/clamp_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/any_all_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/all_all_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sum_all_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/sign_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/pow_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/maximum_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/minimum_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/ge_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/gt_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/le_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/lt_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/ne_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/eq_ops.h
    ${SERIES_GEN_DIR}/CodeM/ops/ops_generated.cpp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUSSQIT_GEN_DIR}/CodeM/ops
  COMMAND ${Python3_EXECUTABLE} ${CODEM_GEN_SCRIPT} --in ${CODEM_NATIVE_YAML} --out ${PLUSSQIT_GEN_DIR}
  DEPENDS ${CODEM_NATIVE_YAML} ${CODEM_GEN_SCRIPT}
  COMMENT "Generating CodeM ops wrappers"
  VERBATIM
)

# CodeM sources (CPU-only). We glob within our tree to avoid a 1000-line CMake list.
# This is acceptable here because CodeM is a large, evolving numerical library and the
# source layout is intentionally regular.
file(GLOB_RECURSE PLUSSQIT_CODEM_CORE_CPP CONFIGURE_DEPENDS
  "src/CodeM/core/*.cpp"
  "src/CodeM/Traverse/*.cpp"
  "src/CodeM/core/ops_impl/*.cpp"
  "src/CodeM/compat/*.cpp"
  "src/CodeM/native/cpu/*.cpp"
  "src/CodeM/native/cuda/*.cpp"
)
list(REMOVE_ITEM PLUSSQIT_CODEM_CORE_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/SeriesTorchAlignedStubs.cpp") # optional stub file remains explicit below
# Duplicate compare implementation; keep CompareOps.cpp as canonical kernel entry.
list(REMOVE_ITEM PLUSSQIT_CODEM_CORE_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/native/cpu/Compare.cpp")

if(APPLE)
  set(PLUSSQIT_CODEM_MPS_SRC
    src/CodeM/native/mps/BufferCache.mm
    src/CodeM/native/mps/MPSAllocator.mm
    src/CodeM/native/mps/MPSGraphCache.mm
    src/CodeM/native/mps/MPSRuntime.mm
    src/CodeM/native/mps/Cast.mm
    src/CodeM/native/mps/Cum.mm
    src/CodeM/native/mps/Elementwise.mm
    src/CodeM/native/mps/Gather.mm
    src/CodeM/native/mps/Linalg.mm
    src/CodeM/native/mps/Matmul.mm
    src/CodeM/native/mps/Nonzero.mm
    src/CodeM/native/mps/Reductions.mm
    src/CodeM/native/mps/Scatter.mm
    src/CodeM/native/mps/Sorting.mm
    src/CodeM/native/mps/Stochastic.mm
  )
else()
  set(PLUSSQIT_CODEM_MPS_SRC
    src/CodeM/native/mps/BufferCacheStub.cpp
    src/CodeM/native/mps/MPSAllocatorStub.cpp
    src/CodeM/native/mps/MPSGraphCacheStub.cpp
    src/CodeM/native/mps/MPSRuntimeStub.cpp
    src/CodeM/native/mps/CastStub.cpp
    src/CodeM/native/mps/CumStub.cpp
    src/CodeM/native/mps/ElementwiseStub.cpp
    src/CodeM/native/mps/GatherStub.cpp
    src/CodeM/native/mps/LinalgStub.cpp
    src/CodeM/native/mps/MatmulStub.cpp
    src/CodeM/native/mps/NonzeroStub.cpp
    src/CodeM/native/mps/ReductionsStub.cpp
    src/CodeM/native/mps/ScatterStub.cpp
    src/CodeM/native/mps/SortingStub.cpp
    src/CodeM/native/mps/StochasticStub.cpp
  )
endif()

set(PLUSSQIT_CODEM_CUDA_KERNEL_SRC)
set(PLUSSQIT_CODEM_CUDA_KERNELS_ENABLED OFF)
if(PLUSSQIT_ENABLE_CUDA)
  find_package(CUDAToolkit QUIET)
  if(CUDAToolkit_FOUND)
    enable_language(CUDA)
    set(PLUSSQIT_CODEM_CUDA_KERNEL_SRC
      src/CodeM/native/cuda/ElementwiseKernels.cu
      src/CodeM/native/cuda/FactoryKernels.cu
      src/CodeM/native/cuda/ReductionKernels.cu
    )
    set(PLUSSQIT_CODEM_CUDA_KERNELS_ENABLED ON)
  endif()
endif()

add_library(plussqit_codem
  src/MSharp/core/MSharpMacros.h
  src/CodeM/core/Errors.h
  src/CodeM/core/ScalarType.h
  src/CodeM/core/Storage.h
  src/CodeM/core/SeriesImpl.h
  src/CodeM/core/SeriesBase.h
  src/CodeM/core/Series.h
  src/CodeM/core/SeriesIterator.h
  src/CodeM/core/SeriesIterator.inl
  # NOTE: SeriesTorchAlignedStubs.cpp contains Python/dunder compatibility stubs that
  # are not part of the current public Series API surface. Compiling it currently
  # introduces definition/decl mismatches and prevents a clean build.
  # When we decide to support these APIs, regenerate both declarations and stubs from
  # the canonical operator schema.
  # src/CodeM/core/SeriesTorchAlignedStubs.cpp
  ${SERIES_OPSINTERNAL_DECLS}
  ${SERIES_OPSINTERNAL_DEFS}
  ${SERIES_METHODS_GEN_HEADER}
  ${PLUSSQIT_CODEM_CORE_CPP}
  ${PLUSSQIT_CODEM_MPS_SRC}
  ${PLUSSQIT_CODEM_CUDA_KERNEL_SRC}
  ${PLUSSQIT_GEN_DIR}/CodeM/ops/ops_generated.cpp
)

add_library(PlussQit::CodeM ALIAS plussqit_codem)
add_library(pluss::plussqit_codem ALIAS plussqit_codem)

target_compile_definitions(plussqit_codem PRIVATE PLUSS_CODEM_GEN_AVAILABLE)

if(PLUSSQIT_CODEM_CUDA_KERNELS_ENABLED)
  set_target_properties(plussqit_codem PROPERTIES
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION OFF
  )
endif()

target_include_directories(plussqit_codem
  PUBLIC
    $<BUILD_INTERFACE:${PLUSSQIT_GEN_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(PLUSSTILS_INCLUDE_DIR)
  target_include_directories(plussqit_codem PUBLIC
    $<BUILD_INTERFACE:${PLUSSTILS_INCLUDE_DIR}>
  )
endif()

if(PLUSS_ENABLE_TORCH)
  if(TORCH_INCLUDE_DIRS)
    target_include_directories(plussqit_codem PUBLIC ${TORCH_INCLUDE_DIRS})
  endif()
  # Export Torch interop gate to downstream consumers so <pluss/codem/codem.h>
  # exposes from_torch/to_torch when they link PlussQit::CodeM / Pluss::SDK.
  target_compile_definitions(plussqit_codem PUBLIC PLUSS_ENABLE_TORCH=1)
  if(TORCH_CXX_FLAGS)
    separate_arguments(_PLUSS_TORCH_CXX_FLAGS NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    target_compile_options(plussqit_codem PUBLIC ${_PLUSS_TORCH_CXX_FLAGS})
  endif()
  target_link_libraries(plussqit_codem PUBLIC ${TORCH_LIBRARIES})
endif()

target_compile_features(plussqit_codem PUBLIC cxx_std_20)

if(APPLE)
  target_compile_options(plussqit_codem PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  target_compile_options(plussqit_codem PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (PLUSSQIT_USE_FMT)
  target_include_directories(plussqit_codem PUBLIC
    $<BUILD_INTERFACE:${PLUSSQIT_FMT_INCLUDE_DIR}>
  )
  target_compile_definitions(plussqit_codem PUBLIC PLUSSQIT_HAS_FMT=1 FMT_HEADER_ONLY=1)
else()
  target_compile_definitions(plussqit_codem PUBLIC PLUSSQIT_HAS_FMT=0)
endif()

if (PLUSSQIT_BUILD_CODEM AND PLUSSQIT_BUILD_CODEM_SMOKE_TESTS)
  add_executable(plussqit_addcmul_test src/CodeM/tools/addcmul_test.cpp)
  target_link_libraries(plussqit_addcmul_test PRIVATE plussqit_codem)
  target_include_directories(plussqit_addcmul_test PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
  add_test(NAME addcmul_test COMMAND plussqit_addcmul_test)
endif()

if (PLUSSQIT_BUILD_CODEM AND PLUSSQIT_BUILD_EXAMPLES)
  add_executable(plussqit_codem_series_usage_demo examples/CodeM/series_usage_demo.cpp)
  target_link_libraries(plussqit_codem_series_usage_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_data_reader_parity_demo examples/CodeM/data_reader_parity_demo.cpp)
  target_link_libraries(plussqit_codem_data_reader_parity_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_aux_let_transmit_demo examples/CodeM/aux_let_transmit_demo.cpp)
  target_link_libraries(plussqit_codem_aux_let_transmit_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_output_engine_demo examples/CodeM/output_engine_demo.cpp)
  target_link_libraries(plussqit_codem_output_engine_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_data_loader_demo examples/CodeM/data_loader_demo.cpp)
  target_link_libraries(plussqit_codem_data_loader_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_transmit_demo examples/CodeM/transmit_demo.cpp)
  target_link_libraries(plussqit_codem_transmit_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_suppose_demo examples/CodeM/suppose_demo.cpp)
  target_link_libraries(plussqit_codem_suppose_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_methods_demo examples/CodeM/series_methods_demo.cpp)
  target_link_libraries(plussqit_codem_series_methods_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_shape_reference_demo examples/CodeM/series_shape_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_shape_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_read_index_reference_demo examples/CodeM/series_read_index_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_read_index_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_combine_reference_demo examples/CodeM/series_combine_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_combine_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_extract_index_take_reference_demo examples/CodeM/series_extract_index_take_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_extract_index_take_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_write_reference_demo examples/CodeM/series_write_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_write_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_selector_write_reference_demo examples/CodeM/series_selector_write_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_selector_write_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_bounds_reference_demo examples/CodeM/series_bounds_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_bounds_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_copy_gather_reference_demo examples/CodeM/series_copy_gather_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_copy_gather_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_reduction_reference_demo examples/CodeM/series_reduction_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_reduction_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_max_reference_demo examples/CodeM/series_max_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_max_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_ordered_extrema_reference_demo examples/CodeM/series_ordered_extrema_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_ordered_extrema_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_factory_core_reference_demo examples/CodeM/series_factory_core_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_factory_core_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_stats_reference_demo examples/CodeM/series_stats_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_stats_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_arithmetic_reference_demo examples/CodeM/series_arithmetic_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_arithmetic_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_comparison_reference_demo examples/CodeM/series_comparison_reference_demo.cpp)
  target_link_libraries(plussqit_codem_series_comparison_reference_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_linear_alg_demo examples/CodeM/linear_alg_demo.cpp)
  target_link_libraries(plussqit_codem_linear_alg_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_traverse_demo examples/CodeM/traverse_demo.cpp)
  target_link_libraries(plussqit_codem_traverse_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_stats_demo examples/CodeM/stats_demo.cpp)
  target_link_libraries(plussqit_codem_stats_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_signal_demo examples/CodeM/signal_demo.cpp)
  target_link_libraries(plussqit_codem_signal_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_autograd_optim_demo examples/CodeM/autograd_optim_demo.cpp)
  target_link_libraries(plussqit_codem_autograd_optim_demo PRIVATE PlussQit::CodeM)
  if (TARGET pluss)
    target_link_libraries(plussqit_codem_autograd_optim_demo PRIVATE pluss)
  endif()
  if (PLUSSMA_INCLUDE_DIR)
    target_include_directories(plussqit_codem_autograd_optim_demo PRIVATE ${PLUSSMA_INCLUDE_DIR})
  endif()

  add_executable(plussqit_codem_autodiff_derivatives_demo examples/CodeM/autodiff_derivatives_demo.cpp)
  target_link_libraries(plussqit_codem_autodiff_derivatives_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_autodiff_minimal_demo examples/CodeM/autodiff_minimal_demo.cpp)
  target_link_libraries(plussqit_codem_autodiff_minimal_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_mps_vs_cpu_demo examples/CodeM/mps_vs_cpu_demo.cpp)
  target_link_libraries(plussqit_codem_mps_vs_cpu_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_mps_bulk_bench_demo examples/CodeM/mps_bulk_bench_demo.cpp)
  target_link_libraries(plussqit_codem_mps_bulk_bench_demo PRIVATE PlussQit::CodeM)

endif()

# Close optional CodeM block before defining CodeN.
endif() # PLUSSQIT_BUILD_CODEM

# ---- CodeN (text, Note/Chord) ----
set(CODEN_GEN_DIR ${CMAKE_BINARY_DIR}/generated/CodeN/generated)
set(CODEN_DISPATCH_FALLBACKS_GEN_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/gen_coden_decl_dispatch_fallbacks.py)
set(CODEN_NOTE_DECLS_GEN ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeN/generated/NoteMethodDeclsGen.h)
set(CODEN_CHORD_DECLS_GEN ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeN/generated/ChordMethodDeclsGen.h)
set(CODEN_DISPATCH_FALLBACKS_INC ${CODEN_GEN_DIR}/DispatchDeclFallbacks.inc)

add_custom_command(
  OUTPUT ${CODEN_DISPATCH_FALLBACKS_INC}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CODEN_GEN_DIR}
  COMMAND ${Python3_EXECUTABLE} ${CODEN_DISPATCH_FALLBACKS_GEN_SCRIPT}
          --note-decls ${CODEN_NOTE_DECLS_GEN}
          --chord-decls ${CODEN_CHORD_DECLS_GEN}
          --out ${CODEN_DISPATCH_FALLBACKS_INC}
  DEPENDS
    ${CODEN_DISPATCH_FALLBACKS_GEN_SCRIPT}
    ${CODEN_NOTE_DECLS_GEN}
    ${CODEN_CHORD_DECLS_GEN}
  COMMENT "Generating CodeN declared-dispatch fallback registry"
)

file(GLOB_RECURSE PLUSSQIT_CODEN_CPP CONFIGURE_DEPENDS
  "src/CodeN/core/*.cpp"
  "src/CodeN/native/cpu/*.cpp"
  "src/CodeN/compat/*.cpp"
)

add_library(plussqit_coden
  ${CODEN_DISPATCH_FALLBACKS_INC}
  ${PLUSSQIT_CODEN_CPP}
  src/Flame/Flame.h
  src/CodeN/core/Errors.h
  src/CodeN/core/TextArena.h
  src/CodeN/core/ChordImpl.h
  src/CodeN/core/ChordBase.h
  src/CodeN/core/ChordSlim.h
  src/CodeN/core/ChordSurface.h
  src/CodeN/core/Chord.h
  src/CodeN/core/NoteImpl.h
  src/CodeN/core/NoteBase.h
  src/CodeN/core/NoteSlim.h
  src/CodeN/core/NoteSurface.h
  src/CodeN/core/TokenBatch.h
  src/CodeN/core/Note.h
  src/CodeN/core/OpsInternal.h
  src/CodeN/compat/Dispatch.h
  src/CodeN/compat/Dispatch.cpp
)

add_library(PlussQit::CodeN ALIAS plussqit_coden)
add_library(pluss::plussqit_coden ALIAS plussqit_coden)

target_include_directories(plussqit_coden
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(plussqit_coden PUBLIC cxx_std_20)
target_compile_definitions(plussqit_coden PRIVATE CODEN_BUILD_MAIN_LIB)

# Unicode normalization is enabled when GNU libunistring is available.  The
# public Note contract remains stable on every platform; builds without the
# backend fail explicitly for normalization instead of silently returning
# unnormalized text.
find_path(PLUSSQIT_UNISTRING_INCLUDE_DIR NAMES uninorm.h)
find_library(PLUSSQIT_UNISTRING_LIBRARY NAMES unistring libunistring)
if(PLUSSQIT_UNISTRING_INCLUDE_DIR AND PLUSSQIT_UNISTRING_LIBRARY)
  target_include_directories(
    plussqit_coden PRIVATE ${PLUSSQIT_UNISTRING_INCLUDE_DIR})
  target_compile_definitions(
    plussqit_coden PRIVATE PLUSSQIT_HAS_LIBUNISTRING=1)
  target_link_libraries(
    plussqit_coden PUBLIC ${PLUSSQIT_UNISTRING_LIBRARY})
endif()

# Optional: CodeN <-> CodeM interop (guards the tiny surface in Note.h)
if (PLUSSQIT_CODEN_ENABLE_SERIES_INTEROP AND PLUSSQIT_BUILD_CODEM)
  target_compile_definitions(plussqit_coden PUBLIC CODEN_ENABLE_SERIES_INTEROP)
  target_link_libraries(plussqit_coden PUBLIC PlussQit::CodeM)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  target_compile_options(plussqit_coden PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (PLUSSQIT_BUILD_EXAMPLES)
  add_executable(plussqit_coden_note_cleanup_demo examples/CodeN/note_cleanup_demo.cpp)
  target_link_libraries(plussqit_coden_note_cleanup_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_cleanup_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_note_method_families_demo examples/CodeN/note_method_families_demo.cpp)
  target_link_libraries(plussqit_coden_note_method_families_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_method_families_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_note_printing_demo examples/CodeN/note_printing_demo.cpp)
  target_link_libraries(plussqit_coden_note_printing_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_printing_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_note_factory_demo examples/CodeN/note_factory_demo.cpp)
  target_link_libraries(plussqit_coden_note_factory_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_factory_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_note_base_methods_demo examples/CodeN/note_base_methods_demo.cpp)
  target_link_libraries(plussqit_coden_note_base_methods_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_base_methods_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_note_docverify_demo examples/CodeN/note_docverify_demo.cpp)
  target_link_libraries(plussqit_coden_note_docverify_demo PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_note_docverify_demo PRIVATE PlussQit::CodeM)
  endif()

  add_executable(plussqit_coden_cpu_perf_bench examples/CodeN/coden_cpu_perf_bench.cpp)
  target_link_libraries(plussqit_coden_cpu_perf_bench PRIVATE PlussQit::CodeN)
  if (TARGET plussqit_codem)
    target_link_libraries(plussqit_coden_cpu_perf_bench PRIVATE PlussQit::CodeM)
  endif()
endif()

# ---- CodeH (time, Timeline/Duration/TimeIndex) ----
if (PLUSSQIT_BUILD_CODEH)
  if (NOT PLUSSQIT_BUILD_CODEM)
    message(FATAL_ERROR "PLUSSQIT_BUILD_CODEH requires PLUSSQIT_BUILD_CODEM=ON")
  endif()

  set(CODEH_GEN_DIR ${PLUSSQIT_GEN_DIR}/CodeH/generated)
  set(CODEH_TIMELINE_METHODS_GEN ${CODEH_GEN_DIR}/TimelineMethodsGen.h)
  set(CODEH_DURATION_METHODS_GEN ${CODEH_GEN_DIR}/DurationMethodsGen.h)
  set(CODEH_TIMEINDEX_METHODS_GEN ${CODEH_GEN_DIR}/TimeIndexMethodsGen.h)
  set(CODEH_METHODS_GEN_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/gen_codeh_methods.py)
  set(CODEH_TIMELINE_DT_FIELDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/codeh_timeline_dt_fields.tsv)
  set(CODEH_TIMELINE_PREDICATES ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/codeh_timeline_predicates.tsv)
  set(CODEH_TIMELINE_STRFTIME ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/codeh_timeline_strftime.tsv)

  add_custom_command(
    OUTPUT
      ${CODEH_TIMELINE_METHODS_GEN}
      ${CODEH_DURATION_METHODS_GEN}
      ${CODEH_TIMEINDEX_METHODS_GEN}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CODEH_GEN_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CODEH_METHODS_GEN_SCRIPT}
            --timeline-dt-fields ${CODEH_TIMELINE_DT_FIELDS}
            --timeline-predicates ${CODEH_TIMELINE_PREDICATES}
            --timeline-strftime ${CODEH_TIMELINE_STRFTIME}
            --out-dir ${CODEH_GEN_DIR}
    DEPENDS
      ${CODEH_METHODS_GEN_SCRIPT}
      ${CODEH_TIMELINE_DT_FIELDS}
      ${CODEH_TIMELINE_PREDICATES}
      ${CODEH_TIMELINE_STRFTIME}
    COMMENT "Generating CodeH method-surface headers"
  )

  file(GLOB_RECURSE PLUSSQIT_CODEH_CPP CONFIGURE_DEPENDS
    "src/CodeH/core/*.cpp"
    "src/CodeH/compat/*.cpp"
  )

  add_library(plussqit_codeh
    ${CODEH_TIMELINE_METHODS_GEN}
    ${CODEH_DURATION_METHODS_GEN}
    ${CODEH_TIMEINDEX_METHODS_GEN}
    ${PLUSSQIT_CODEH_CPP}
    src/CodeH/compat/Access.h
    src/CodeH/compat/InvokeFwd.h
    src/CodeH/core/Errors.h
  )

  add_library(PlussQit::CodeH ALIAS plussqit_codeh)
  add_library(pluss::plussqit_codeh ALIAS plussqit_codeh)

  target_include_directories(plussqit_codeh
    PUBLIC
      $<BUILD_INTERFACE:${PLUSSQIT_GEN_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_features(plussqit_codeh PUBLIC cxx_std_20)
  target_compile_definitions(plussqit_codeh
    PRIVATE CODEH_BUILD_MAIN_LIB
    PUBLIC
      CODEH_ENABLE_TIMELINE_SURFACE
      CODEH_ENABLE_TIMEINDEX_SURFACE
      CODEH_ENABLE_DURATION_SURFACE
  )
  target_link_libraries(plussqit_codeh PUBLIC PlussQit::CodeM PlussQit::CodeN)

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(plussqit_codeh PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  if (PLUSSQIT_BUILD_EXAMPLES)
    add_executable(plussqit_codeh_timeline_printing_2d_3d_demo
      examples/CodeH/timeline_printing_2d_3d_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeline_printing_2d_3d_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_timeline_dataframe_timeindex_methods_demo
      examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeline_dataframe_timeindex_methods_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_timeline_non_generated_and_factory_methods_demo
      examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeline_non_generated_and_factory_methods_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_timeindex_docverify_demo
      examples/CodeH/timeindex_docverify_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeindex_docverify_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_timeline_docverify_demo
      examples/CodeH/timeline_docverify_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeline_docverify_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_duration_docverify_demo
      examples/CodeH/duration_docverify_demo.cpp
    )
    target_link_libraries(plussqit_codeh_duration_docverify_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_codeh_timeline_duration_interop_docverify_demo
      examples/CodeH/timeline_duration_interop_docverify_demo.cpp
    )
    target_link_libraries(plussqit_codeh_timeline_duration_interop_docverify_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_dispense_demo
      examples/dispense_demo.cpp
    )
    target_link_libraries(plussqit_dispense_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_translate_demo
      examples/translate_demo.cpp
    )
    target_link_libraries(plussqit_translate_demo PRIVATE PlussQit::CodeH)

    add_executable(plussqit_queue_demo
      examples/queue_demo.cpp
    )
    target_link_libraries(plussqit_queue_demo PRIVATE PlussQit::CodeH)
  endif()
endif()


if (PLUSSQIT_BUILD_TESTS)
  enable_testing()

  file(GLOB CODEM_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/CodeM/*.cpp"
  )
  list(FILTER CODEM_TEST_SOURCES EXCLUDE REGEX ".*/test_codem_nocatch_smoke\\.cpp$")

  file(GLOB CODEN_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/CodeN/*.cpp"
  )
  list(FILTER CODEN_TEST_SOURCES EXCLUDE REGEX ".*/test_coden_nocatch_smoke\\.cpp$")

  if (PLUSSQIT_BUILD_CODEH)
    file(GLOB CODEH_TEST_SOURCES CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/CodeH/*.cpp"
    )
    list(FILTER CODEH_TEST_SOURCES EXCLUDE REGEX ".*/test_codeh_nocatch_smoke\\.cpp$")
  endif()

  find_package(Catch2 3 QUIET)
  if (NOT Catch2_FOUND AND PLUSSQIT_FETCH_DEPS)
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
  endif()

  if (NOT Catch2_FOUND)
    message(STATUS "Catch2 v3 not found; building no-catch CodeM/CodeN smoke tests instead.")
    if (PLUSSQIT_BUILD_CODEM)
      add_executable(plussqit_codem_nocatch_tests
        tests/CodeM/test_codem_nocatch_smoke.cpp
      )
      target_link_libraries(plussqit_codem_nocatch_tests PRIVATE PlussQit::CodeM)
      target_compile_features(plussqit_codem_nocatch_tests PRIVATE cxx_std_20)
      add_test(NAME codem_nocatch_smoke COMMAND plussqit_codem_nocatch_tests)
    endif()
    add_executable(plussqit_coden_nocatch_tests
      tests/CodeN/test_coden_nocatch_smoke.cpp
    )
    target_link_libraries(plussqit_coden_nocatch_tests PRIVATE PlussQit::CodeN)
    if (TARGET plussqit_codem)
      target_link_libraries(plussqit_coden_nocatch_tests PRIVATE PlussQit::CodeM)
    endif()
    target_compile_features(plussqit_coden_nocatch_tests PRIVATE cxx_std_20)
    add_test(NAME coden_nocatch_smoke COMMAND plussqit_coden_nocatch_tests)

    if (PLUSSQIT_BUILD_CODEH)
      add_executable(plussqit_codeh_nocatch_tests
        tests/CodeH/test_codeh_nocatch_smoke.cpp
      )
      target_link_libraries(plussqit_codeh_nocatch_tests PRIVATE PlussQit::CodeH)
      target_compile_features(plussqit_codeh_nocatch_tests PRIVATE cxx_std_20)
      add_test(NAME codeh_nocatch_smoke COMMAND plussqit_codeh_nocatch_tests)
    endif()
  else()
    add_executable(plussqit_tests ${CODEM_TEST_SOURCES})
    target_link_libraries(plussqit_tests PRIVATE PlussQit::CodeM Catch2::Catch2WithMain)
    if(TARGET PlussQit::CodeH)
      target_link_libraries(plussqit_tests PRIVATE PlussQit::CodeH)
    elseif(TARGET PlussQit::CodeN)
      target_link_libraries(plussqit_tests PRIVATE PlussQit::CodeN)
    endif()
    target_compile_features(plussqit_tests PRIVATE cxx_std_20)

    add_executable(plussqit_codem_autodiff_tests
      tests/CodeM/test_patch67_autodiff_transforms.cpp
      tests/CodeM/test_patch167_cpu_autodiff_parity.cpp
    )
    target_link_libraries(plussqit_codem_autodiff_tests PRIVATE PlussQit::CodeM Catch2::Catch2WithMain)
    target_compile_features(plussqit_codem_autodiff_tests PRIVATE cxx_std_20)

    set(CODEM_MPS_GATE_TEST_SOURCES
      tests/CodeM/test_patch93_mps_sorting_selection_stress.cpp
      tests/CodeM/test_patch94_mps_take_put_stress.cpp
      tests/CodeM/test_patch95_mps_nonzero_stress.cpp
      tests/CodeM/test_patch97_linalg_backend_info.cpp
      tests/CodeM/test_patch98_mps_linalg_bridge_stress.cpp
      tests/CodeM/test_patch99_linalg_bridge_counters.cpp
      tests/CodeM/test_patch100_mps_qr_svd_bridge_stress.cpp
      tests/CodeM/test_patch101_mps_linalg_solver_residuals.cpp
      tests/CodeM/test_patch102_mps_native_solver_dispatch.cpp
      tests/CodeM/test_patch104_mps_native_inverse_lu_dispatch.cpp
      tests/CodeM/test_patch105_mps_native_det_dispatch.cpp
      tests/CodeM/test_patch106_mps_native_pinverse_dispatch.cpp
      tests/CodeM/test_patch107_mps_native_qr_dispatch.cpp
      tests/CodeM/test_patch108_mps_native_svd_dispatch.cpp
      tests/CodeM/test_patch111_mps_cast_coverage.cpp
      tests/CodeM/test_patch114_mps_linalg_policy_runtime.cpp
      tests/CodeM/test_patch118_mps_cache_strict_regression.cpp
      tests/CodeM/test_patch121_mps_batched_qr_svd_bridge.cpp
      tests/CodeM/test_patch122_mps_batched_unary_linalg_bridge.cpp
      tests/CodeM/test_patch123_mps_batched_solver_bridge.cpp
      tests/CodeM/test_patch124_mps_batched_solver_broadcast.cpp
      tests/CodeM/test_patch125_mps_solver_route_policy.cpp
      tests/CodeM/test_patch130_mps_native_device_telemetry.cpp
      tests/CodeM/test_patch131_mps_native_sort_selection_strict.cpp
      tests/CodeM/test_patch132_mps_native_svd_strict_graduation.cpp
      tests/CodeM/test_patch144_mps_native_take_put_graduation.cpp
      tests/CodeM/test_patch149_mps_torch_family_parity.cpp
      tests/CodeM/test_patch150_mps_strict_indexing_scatter_lanes.cpp
      tests/CodeM/test_patch151_mps_selection_strict_lanes_and_soak.cpp
      tests/CodeM/test_patch167_mps_nonzero_int32_native_tranche.cpp
      tests/CodeM/test_patch169_mps_masking_counting_int32_strict.cpp
      tests/CodeM/test_patch170_mps_torchwide_series_strict.cpp
      tests/CodeM/test_patch171_mps_torchwide_series_extended_strict.cpp
      tests/CodeM/test_patch175_mps_stochastic_dropout_parity.cpp
    )
    set(CODEM_CUDA_GATE_TEST_SOURCES
      tests/CodeM/test_patch97_linalg_backend_info.cpp
      tests/CodeM/test_patch133_cuda_bootstrap_contract.cpp
      tests/CodeM/test_patch137_cuda_selection_baseline_contract.cpp
      tests/CodeM/test_patch138_cuda_linalg_baseline_contract.cpp
      tests/CodeM/test_patch139_cuda_bridge_bootstrap_contract.cpp
      tests/CodeM/test_patch140_cuda_indexing_baseline_contract.cpp
      tests/CodeM/test_patch141_cuda_indexing_bridge_contract.cpp
      tests/CodeM/test_patch142_cuda_matmul_contract.cpp
      tests/CodeM/test_patch143_cuda_scatter_gather_contract.cpp
      tests/CodeM/test_patch145_cuda_provider_inventory_contract.cpp
      tests/CodeM/test_patch148_cuda_torch_family_parity.cpp
      tests/CodeM/test_patch152_cuda_native_strict_parity_and_soak.cpp
      tests/CodeM/test_patch153_cuda_native_lane_expansion.cpp
      tests/CodeM/test_patch154_cuda_reduction_contract.cpp
      tests/CodeM/test_patch155_cuda_elementwise_contract.cpp
      tests/CodeM/test_patch156_cuda_scan_activation_contract.cpp
      tests/CodeM/test_patch157_cuda_masking_contract.cpp
      tests/CodeM/test_patch158_cuda_bitwise_compose_contract.cpp
      tests/CodeM/test_patch159_cuda_linalg_batched_rank_contract.cpp
      tests/CodeM/test_patch160_cuda_linalg_expanded_contract.cpp
      tests/CodeM/test_patch161_cuda_matmul_rank_contract.cpp
      tests/CodeM/test_patch162_cuda_counting_contract.cpp
      tests/CodeM/test_patch163_cuda_selection_route_contract.cpp
      tests/CodeM/test_patch164_cuda_factory_contract.cpp
      tests/CodeM/test_patch165_cuda_stochastic_contract.cpp
      tests/CodeM/test_patch166_cuda_cublas_strict_route.cpp
      tests/CodeM/test_patch176_cuda_no_bridge_contract.cpp
    )

    add_executable(plussqit_codem_mps_selection_tests
      ${CODEM_MPS_GATE_TEST_SOURCES}
    )
    target_link_libraries(plussqit_codem_mps_selection_tests PRIVATE PlussQit::CodeM Catch2::Catch2WithMain)
    target_compile_features(plussqit_codem_mps_selection_tests PRIVATE cxx_std_20)

    add_executable(plussqit_codem_cuda_gate_tests
      ${CODEM_CUDA_GATE_TEST_SOURCES}
    )
    target_link_libraries(plussqit_codem_cuda_gate_tests PRIVATE PlussQit::CodeM Catch2::Catch2WithMain)
    target_compile_features(plussqit_codem_cuda_gate_tests PRIVATE cxx_std_20)

    add_executable(plussqit_codem_optim_tests
      tests/CodeM/test_patch67_autodiff_transforms.cpp
      tests/CodeM/test_patch68_constrained_optim_phase1.cpp
      tests/CodeM/test_patch69_sequential_qp_phase2.cpp
      tests/CodeM/test_patch70_interior_point_phase3.cpp
      tests/CodeM/test_patch71_optima_dispatch.cpp
      tests/CodeM/test_patch72_optima_recovery.cpp
      tests/CodeM/test_patch73_optima_validation.cpp
      tests/CodeM/test_patch74_optima_derivative_audit.cpp
      tests/CodeM/test_patch75_optima_progress_hook.cpp
      tests/CodeM/test_patch76_optima_exit_behavior.cpp
      tests/CodeM/test_patch77_optima_garch_objective.cpp
      tests/CodeM/test_patch78_optima_scaling.cpp
      tests/CodeM/test_patch79_optima_cg.cpp
      tests/CodeM/test_patch80_optima_lbfgs.cpp
      tests/CodeM/test_patch81_optima_primal_dual_ip.cpp
      tests/CodeM/test_patch82_optima_feasibility_phase.cpp
      tests/CodeM/test_patch83_optima_exact_curvature.cpp
      tests/CodeM/test_patch84_optima_sqp_soc.cpp
      tests/CodeM/test_patch85_optima_linear_policy.cpp
      tests/CodeM/test_patch86_optima_normalization.cpp
      tests/CodeM/test_patch87_optima_output_parity.cpp
      tests/CodeM/test_patch88_optima_parity_suite.cpp
      tests/CodeM/test_patch89_optima_sqp_structured.cpp
      tests/CodeM/test_patch90_optima_stall_diagnostics.cpp
      tests/CodeM/test_patch91_optima_pathology_suite.cpp
      tests/CodeM/test_patch92_optima_working_set.cpp
      tests/CodeM/test_patch171_unique_dim_contract.cpp
      tests/CodeM/test_patch172_series_set_ops.cpp
      tests/CodeM/test_patch173_series_set_number_ops.cpp
      tests/CodeM/test_patch174_torch_inplace_parity_aliases.cpp
    )
    target_link_libraries(plussqit_codem_optim_tests PRIVATE PlussQit::CodeM Catch2::Catch2WithMain)
    target_compile_features(plussqit_codem_optim_tests PRIVATE cxx_std_20)
    if (PLUSSMA_INCLUDE_DIR)
      target_include_directories(plussqit_codem_optim_tests PRIVATE ${PLUSSMA_INCLUDE_DIR})
    endif()

    # CodeN-only tests (kept separate so CodeN can evolve even when CodeM is mid-refactor).
    add_executable(plussqit_coden_tests ${CODEN_TEST_SOURCES})
    target_link_libraries(plussqit_coden_tests PRIVATE PlussQit::CodeN Catch2::Catch2WithMain)
    if (TARGET plussqit_codem)
      target_link_libraries(plussqit_coden_tests PRIVATE PlussQit::CodeM)
    endif()
    target_compile_features(plussqit_coden_tests PRIVATE cxx_std_20)

    if (PLUSSQIT_BUILD_CODEH AND CODEH_TEST_SOURCES)
      add_executable(plussqit_codeh_tests ${CODEH_TEST_SOURCES})
      target_link_libraries(plussqit_codeh_tests PRIVATE PlussQit::CodeH Catch2::Catch2WithMain)
      target_compile_features(plussqit_codeh_tests PRIVATE cxx_std_20)
    endif()

    include(CTest)
    include(Catch)
    catch_discover_tests(plussqit_tests)
    catch_discover_tests(plussqit_codem_autodiff_tests)
    catch_discover_tests(plussqit_codem_mps_selection_tests)
    catch_discover_tests(plussqit_codem_cuda_gate_tests)
    catch_discover_tests(plussqit_codem_optim_tests)
    catch_discover_tests(plussqit_coden_tests)
    if (PLUSSQIT_BUILD_CODEH AND TARGET plussqit_codeh_tests)
      catch_discover_tests(plussqit_codeh_tests)
    endif()
  endif()
endif()


# ---- Install (optional) ----
include(GNUInstallDirs)

install(TARGETS plussqit_coden
  EXPORT PlussQitTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (PLUSSQIT_BUILD_CODEM)
  install(TARGETS plussqit_codem
    EXPORT PlussQitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

if (PLUSSQIT_BUILD_CODEH)
  install(TARGETS plussqit_codeh
    EXPORT PlussQitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PATTERN ".DS_Store" EXCLUDE
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/CodeM.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/core
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/compat/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/compat
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/Indexing/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/Indexing
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/Traverse/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/Traverse
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/native/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/native
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeN/core/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeN/core
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeN/compat/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeN/compat
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeN/generated/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeN/generated
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/MSharp/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MSharp
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/Flame/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Flame
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
  PATTERN ".DS_Store" EXCLUDE
)

if (PLUSSQIT_USE_FMT AND PLUSSQIT_FMT_INCLUDE_DIR)
  install(DIRECTORY ${PLUSSQIT_FMT_INCLUDE_DIR}/fmt/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fmt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN ".DS_Store" EXCLUDE
  )
endif()

if (PLUSSQIT_BUILD_CODEH)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeH/core/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeH/core
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inc"
    PATTERN ".DS_Store" EXCLUDE
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeH/compat/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeH/compat
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inc"
    PATTERN ".DS_Store" EXCLUDE
  )
endif()

# Generated Series headers are included for completeness (CodeM only).
if (PLUSSQIT_BUILD_CODEM)
  install(DIRECTORY ${SERIES_GEN_DIR}/CodeM/generated/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/generated
  )
  install(DIRECTORY ${SERIES_GEN_DIR}/CodeM/Series/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/Series
  )
endif()

if (PLUSSQIT_BUILD_CODEH)
  install(DIRECTORY ${PLUSSQIT_GEN_DIR}/CodeH/generated/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeH/generated
  )
endif()

install(EXPORT PlussQitTargets
  NAMESPACE pluss::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PlussQit
)

if(PLUSSQIT_BUILD_CODEM)
  set(_pluss_cpu_blas_backend "${PLUSSQIT_CPU_BLAS_BACKEND}")
  string(TOUPPER "${_pluss_cpu_blas_backend}" _pluss_cpu_blas_backend)
  set(_pluss_cpu_blas_valid AUTO ACCELERATE OPENBLAS ONEMKL NONE)
  list(FIND _pluss_cpu_blas_valid "${_pluss_cpu_blas_backend}" _pluss_cpu_blas_backend_idx)
  if(_pluss_cpu_blas_backend_idx EQUAL -1)
    message(WARNING "Unknown PLUSSQIT_CPU_BLAS_BACKEND='${PLUSSQIT_CPU_BLAS_BACKEND}', falling back to AUTO.")
    set(_pluss_cpu_blas_backend "AUTO")
  endif()

  set(_pluss_cpu_blas_provider "NONE")
  set(_pluss_cpu_blas_provider_macro 0)
  set(_pluss_has_accelerate 0)
  set(_pluss_has_mps 0)
  set(_pluss_has_cuda 0)
  set(_pluss_has_accelerate_new_lapack 0)

  if(APPLE)
    find_library(PLUSS_ACCELERATE_FRAMEWORK Accelerate)
    find_library(PLUSS_METAL_FRAMEWORK Metal)
    find_library(PLUSS_MPS_FRAMEWORK MetalPerformanceShaders)
    find_library(PLUSS_MPS_GRAPH_FRAMEWORK MetalPerformanceShadersGraph)
    find_library(PLUSS_FOUNDATION_FRAMEWORK Foundation)
    if(PLUSS_ACCELERATE_FRAMEWORK)
      # Link frameworks as proper libraries so static-lib consumers resolve symbols
      # in the final link pass (instead of relying on early link options ordering).
      target_link_libraries(plussqit_codem PUBLIC "-framework Accelerate")
      set(_pluss_has_accelerate 1)
      set(_pluss_has_accelerate_new_lapack 1)
    endif()
    if(PLUSS_METAL_FRAMEWORK AND PLUSS_MPS_FRAMEWORK AND PLUSS_FOUNDATION_FRAMEWORK)
      target_link_libraries(plussqit_codem PUBLIC
        "-framework Metal"
        "-framework MetalPerformanceShaders"
        "-framework Foundation"
      )
      if(PLUSS_MPS_GRAPH_FRAMEWORK)
        target_link_libraries(plussqit_codem PUBLIC "-framework MetalPerformanceShadersGraph")
      endif()
      set(_pluss_has_mps 1)
    endif()
  endif()

  if(_pluss_cpu_blas_backend STREQUAL "NONE")
    set(_pluss_cpu_blas_provider "NONE")
  elseif(_pluss_cpu_blas_backend STREQUAL "ACCELERATE")
    if(APPLE AND PLUSS_ACCELERATE_FRAMEWORK)
      set(_pluss_cpu_blas_provider "ACCELERATE")
    else()
      message(WARNING "PLUSSQIT_CPU_BLAS_BACKEND=ACCELERATE requested but Accelerate is unavailable. Falling back to NONE.")
    endif()
  elseif(_pluss_cpu_blas_backend STREQUAL "OPENBLAS")
    find_package(BLAS QUIET)
    if(BLAS_FOUND)
      target_link_libraries(plussqit_codem PUBLIC ${BLAS_LIBRARIES})
      if(DEFINED BLAS_INCLUDE_DIRS AND BLAS_INCLUDE_DIRS)
        target_include_directories(plussqit_codem PUBLIC ${BLAS_INCLUDE_DIRS})
      endif()
      set(_pluss_cpu_blas_provider "OPENBLAS")
    else()
      message(WARNING "PLUSSQIT_CPU_BLAS_BACKEND=OPENBLAS requested but BLAS was not found. Falling back to NONE.")
    endif()
  elseif(_pluss_cpu_blas_backend STREQUAL "ONEMKL")
    find_package(BLAS QUIET)
    if(BLAS_FOUND)
      target_link_libraries(plussqit_codem PUBLIC ${BLAS_LIBRARIES})
      if(DEFINED BLAS_INCLUDE_DIRS AND BLAS_INCLUDE_DIRS)
        target_include_directories(plussqit_codem PUBLIC ${BLAS_INCLUDE_DIRS})
      endif()
      set(_pluss_cpu_blas_provider "ONEMKL")
    else()
      message(WARNING "PLUSSQIT_CPU_BLAS_BACKEND=ONEMKL requested but BLAS was not found. Falling back to NONE.")
    endif()
  else()
    if(APPLE AND PLUSS_ACCELERATE_FRAMEWORK)
      set(_pluss_cpu_blas_provider "ACCELERATE")
    else()
      find_package(BLAS QUIET)
      if(BLAS_FOUND)
        target_link_libraries(plussqit_codem PUBLIC ${BLAS_LIBRARIES})
        if(DEFINED BLAS_INCLUDE_DIRS AND BLAS_INCLUDE_DIRS)
          target_include_directories(plussqit_codem PUBLIC ${BLAS_INCLUDE_DIRS})
        endif()
        set(_pluss_blas_libs_joined "${BLAS_LIBRARIES}")
        string(TOLOWER "${_pluss_blas_libs_joined}" _pluss_blas_libs_lower)
        if(_pluss_blas_libs_lower MATCHES "mkl")
          set(_pluss_cpu_blas_provider "ONEMKL")
        elseif(_pluss_blas_libs_lower MATCHES "openblas")
          set(_pluss_cpu_blas_provider "OPENBLAS")
        else()
          set(_pluss_cpu_blas_provider "CBLAS")
        endif()
      endif()
    endif()
  endif()

  if(_pluss_cpu_blas_provider STREQUAL "ACCELERATE")
    set(_pluss_cpu_blas_provider_macro 1)
  elseif(_pluss_cpu_blas_provider STREQUAL "OPENBLAS")
    set(_pluss_cpu_blas_provider_macro 2)
  elseif(_pluss_cpu_blas_provider STREQUAL "ONEMKL")
    set(_pluss_cpu_blas_provider_macro 3)
  elseif(_pluss_cpu_blas_provider STREQUAL "CBLAS")
    set(_pluss_cpu_blas_provider_macro 4)
  else()
    set(_pluss_cpu_blas_provider_macro 0)
  endif()

  if(PLUSSQIT_ENABLE_CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
      target_link_libraries(plussqit_codem PRIVATE CUDA::cudart)
      set(_pluss_has_cuda 1)
    else()
      message(WARNING "PLUSSQIT_ENABLE_CUDA=ON requested but CUDAToolkit was not found. CUDA runtime probe disabled.")
    endif()
  endif()

  target_compile_definitions(plussqit_codem PUBLIC
    PLUSS_HAS_ACCELERATE=${_pluss_has_accelerate}
    PLUSS_HAS_MPS=${_pluss_has_mps}
    PLUSS_HAS_CUDA=${_pluss_has_cuda}
  )
  if(_pluss_has_accelerate_new_lapack EQUAL 1)
    target_compile_definitions(plussqit_codem PUBLIC ACCELERATE_NEW_LAPACK=1)
  endif()
  target_compile_definitions(plussqit_codem PRIVATE PLUSSQIT_CPU_BLAS_PROVIDER=${_pluss_cpu_blas_provider_macro})
  message(STATUS "CodeM CPU BLAS provider: ${_pluss_cpu_blas_provider} (request=${_pluss_cpu_blas_backend})")
  message(STATUS "CodeM CUDA runtime probe: ${_pluss_has_cuda} (requested=${PLUSSQIT_ENABLE_CUDA})")
endif()

if(BUILD_BENCHMARKS)
  add_executable(bench_ops benchmarks/bench_ops.cpp)
  if (PLUSSQIT_BUILD_CODEM)
    target_link_libraries(bench_ops PRIVATE PlussQit::CodeM)
  else()
    target_link_libraries(bench_ops PRIVATE plussqit)
  endif()
  target_include_directories(bench_ops PRIVATE include)

  if (PLUSSQIT_BUILD_CODEM AND PLUSS_ENABLE_TORCH)
    add_executable(bench_torch_parity benchmarks/bench_torch_parity.cpp)
    target_link_libraries(bench_torch_parity PRIVATE PlussQit::CodeM ${TORCH_LIBRARIES})
    target_include_directories(bench_torch_parity PRIVATE include)
    if (TORCH_CXX_FLAGS)
      target_compile_options(bench_torch_parity PRIVATE ${TORCH_CXX_FLAGS})
    endif()

    add_custom_target(codem_torch90_tranche
      COMMAND $<TARGET_FILE:bench_torch_parity> --target-score=90 --emit-csv
      DEPENDS bench_torch_parity
      USES_TERMINAL
      COMMENT "Run CodeM torch-level CPU tranche scorecard (target >= 90)"
    )
  endif()
endif()
