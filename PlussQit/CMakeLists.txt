cmake_minimum_required(VERSION 3.25)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
  include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PlussQit LANGUAGES CXX)

option(PLUSS_ENABLE_TORCH "Enable LibTorch interop" OFF)
if(PLUSS_ENABLE_TORCH)
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
option(BUILD_BENCHMARKS "Build benchmark executables" OFF)

set(PLUSSQIT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PLUSSQIT_GEN_DIR})

include(FetchContent)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if (PLUSSQIT_USE_FMT)
  find_package(fmt QUIET)
  if (NOT fmt_FOUND AND PLUSSQIT_FETCH_DEPS)
    FetchContent_Declare(
      fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)
  endif()
  if (NOT fmt_FOUND)
    message(STATUS "fmt not found; disabling fmt support (set PLUSSQIT_FETCH_DEPS=ON to fetch, or install fmt).")
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


set(SERIES_OPSINTERNAL_GEN_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/generate_series_methods_from_opsinternal.py)
set(SERIES_OPSINTERNAL_H ${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/OpsInternal.h)

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
  DEPENDS ${SERIES_OPSINTERNAL_H} ${SERIES_OPSINTERNAL_GEN_SCRIPT}
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
          --out ${SERIES_METHODS_GEN_HEADER}
  DEPENDS ${CODEM_NATIVE_YAML} ${SERIES_OPSINTERNAL_H} ${SERIES_METHODS_SEED_LIST} ${SERIES_METHODS_GEN_SCRIPT}
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
  "src/CodeM/core/ops_impl/*.cpp"
  "src/CodeM/compat/*.cpp"
  "src/CodeM/native/cpu/*.cpp"
)
list(REMOVE_ITEM PLUSSQIT_CODEM_CORE_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/core/SeriesTorchAlignedStubs.cpp") # optional stub file remains explicit below
# Duplicate compare implementation; keep CompareOps.cpp as canonical kernel entry.
list(REMOVE_ITEM PLUSSQIT_CODEM_CORE_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/CodeM/native/cpu/Compare.cpp")

if(APPLE)
  set(PLUSSQIT_CODEM_MPS_SRC
    src/CodeM/native/mps/BufferCache.mm
    src/CodeM/native/mps/Cast.mm
    src/CodeM/native/mps/Cum.mm
    src/CodeM/native/mps/Elementwise.mm
    src/CodeM/native/mps/Gather.mm
    src/CodeM/native/mps/Matmul.mm
    src/CodeM/native/mps/Reductions.mm
    src/CodeM/native/mps/Scatter.mm
  )
else()
  set(PLUSSQIT_CODEM_MPS_SRC
    src/CodeM/native/mps/BufferCacheStub.cpp
    src/CodeM/native/mps/CastStub.cpp
    src/CodeM/native/mps/CumStub.cpp
    src/CodeM/native/mps/ElementwiseStub.cpp
    src/CodeM/native/mps/GatherStub.cpp
    src/CodeM/native/mps/MatmulStub.cpp
    src/CodeM/native/mps/ReductionsStub.cpp
    src/CodeM/native/mps/ScatterStub.cpp
  )
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
  ${PLUSSQIT_GEN_DIR}/CodeM/ops/ops_generated.cpp
)

add_library(PlussQit::CodeM ALIAS plussqit_codem)

target_compile_definitions(plussqit_codem PRIVATE PLUSS_CODEM_GEN_AVAILABLE)

target_include_directories(plussqit_codem
  PUBLIC
    $<BUILD_INTERFACE:${PLUSSQIT_GEN_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(PLUSS_ENABLE_TORCH)
  if(TORCH_INCLUDE_DIRS)
    target_include_directories(plussqit_codem PUBLIC ${TORCH_INCLUDE_DIRS})
  endif()
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
  target_link_libraries(plussqit_codem PUBLIC fmt::fmt)
  target_compile_definitions(plussqit_codem PUBLIC PLUSSQIT_HAS_FMT=1)
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

  add_executable(plussqit_codem_aux_let_transmit_demo examples/CodeM/aux_let_transmit_demo.cpp)
  target_link_libraries(plussqit_codem_aux_let_transmit_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_transmit_demo examples/CodeM/transmit_demo.cpp)
  target_link_libraries(plussqit_codem_transmit_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_suppose_demo examples/CodeM/suppose_demo.cpp)
  target_link_libraries(plussqit_codem_suppose_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_series_methods_demo examples/CodeM/series_methods_demo.cpp)
  target_link_libraries(plussqit_codem_series_methods_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_linear_alg_demo examples/CodeM/linear_alg_demo.cpp)
  target_link_libraries(plussqit_codem_linear_alg_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_stats_demo examples/CodeM/stats_demo.cpp)
  target_link_libraries(plussqit_codem_stats_demo PRIVATE PlussQit::CodeM)

  add_executable(plussqit_codem_signal_demo examples/CodeM/signal_demo.cpp)
  target_link_libraries(plussqit_codem_signal_demo PRIVATE PlussQit::CodeM)

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

target_include_directories(plussqit_coden
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(plussqit_coden PUBLIC cxx_std_20)
target_compile_definitions(plussqit_coden PRIVATE CODEN_BUILD_MAIN_LIB)

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
    target_compile_features(plussqit_tests PRIVATE cxx_std_20)

    # CodeN-only tests (kept separate so CodeN can evolve even when CodeM is mid-refactor).
    add_executable(plussqit_coden_tests ${CODEN_TEST_SOURCES})
    target_link_libraries(plussqit_coden_tests PRIVATE PlussQit::CodeN Catch2::Catch2WithMain)
    target_compile_features(plussqit_coden_tests PRIVATE cxx_std_20)

    if (PLUSSQIT_BUILD_CODEH AND CODEH_TEST_SOURCES)
      add_executable(plussqit_codeh_tests ${CODEH_TEST_SOURCES})
      target_link_libraries(plussqit_codeh_tests PRIVATE PlussQit::CodeH Catch2::Catch2WithMain)
      target_compile_features(plussqit_codeh_tests PRIVATE cxx_std_20)
    endif()

    include(CTest)
    include(Catch)
    catch_discover_tests(plussqit_tests)
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
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/aux/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aux
)

# Generated Series headers are included for completeness (CodeM only).
if (PLUSSQIT_BUILD_CODEM)
  install(DIRECTORY ${SERIES_GEN_DIR}/CodeM/generated/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CodeM/generated
  )
  install(DIRECTORY ${SERIES_GEN_DIR}/CodeM/Series/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Series
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

if(APPLE AND PLUSSQIT_BUILD_CODEM)
  find_library(PLUSS_ACCELERATE_FRAMEWORK Accelerate)
  find_library(PLUSS_METAL_FRAMEWORK Metal)
  find_library(PLUSS_MPS_FRAMEWORK MetalPerformanceShaders)
  find_library(PLUSS_FOUNDATION_FRAMEWORK Foundation)
  if(PLUSS_ACCELERATE_FRAMEWORK)
    target_link_libraries(plussqit_codem PUBLIC ${PLUSS_ACCELERATE_FRAMEWORK})
    target_compile_definitions(plussqit_codem PUBLIC PLUSS_HAS_ACCELERATE=1)
    target_compile_definitions(plussqit_codem PUBLIC ACCELERATE_NEW_LAPACK=1)
  else()
    target_compile_definitions(plussqit_codem PUBLIC PLUSS_HAS_ACCELERATE=0)
  endif()
  if(PLUSS_METAL_FRAMEWORK AND PLUSS_MPS_FRAMEWORK AND PLUSS_FOUNDATION_FRAMEWORK)
    target_link_libraries(plussqit_codem PUBLIC ${PLUSS_METAL_FRAMEWORK} ${PLUSS_MPS_FRAMEWORK} ${PLUSS_FOUNDATION_FRAMEWORK})
    target_compile_definitions(plussqit_codem PUBLIC PLUSS_HAS_MPS=1)
  else()
    target_compile_definitions(plussqit_codem PUBLIC PLUSS_HAS_MPS=0)
  endif()
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
