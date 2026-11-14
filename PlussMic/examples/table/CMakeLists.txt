cmake_minimum_required(VERSION 3.28)
project(PlussMic_examples_table NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_dataframe_initial_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_initial_demo.cpp
)
add_executable(plussmic_dataframe_ab_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_ab_reference_demo.cpp
)
add_executable(plussmic_slate_a_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/slate_a_reference_demo.cpp
)
add_executable(plussmic_dataframe_b_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_b_reference_demo.cpp
)
add_executable(plussmic_slate_b_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/slate_b_reference_demo.cpp
)
add_executable(plussmic_dataframe_cd_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_cd_reference_demo.cpp
)
add_executable(plussmic_slate_cd_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/slate_cd_reference_demo.cpp
)
add_executable(plussmic_dataframe_view_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_view_demo.cpp
)
add_executable(plussmic_dataframe_describe_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_describe_demo.cpp
)
add_executable(plussmic_display_policy_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/display_policy_demo.cpp
)
add_executable(plussmic_df_ex_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/df_ex_demo.cpp
)
add_executable(plussmic_dataframe_versatile_methods_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_versatile_methods_demo.cpp
)
add_executable(plussmic_dataframe_e_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_e_reference_demo.cpp
)
add_executable(plussmic_dataframe_efg_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_efg_reference_demo.cpp
)
add_executable(plussmic_dataframe_hijk_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_hijk_reference_demo.cpp
)
add_executable(plussmic_dataframe_rich_reference_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_rich_reference_demo.cpp
)
add_executable(plussmic_us_accidents_eda_verbs_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/us_accidents_eda_verbs_demo.cpp
)
add_executable(plussmic_eda_us_accidents_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/eda_us_accidents.cpp
)
add_executable(plussmic_dataframe_plot_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_plot_demo.cpp
)
add_executable(plussmic_dataframe_slate_values_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_slate_values_demo.cpp
)
add_executable(plussmic_loader_saver_parity_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/loader_saver_parity_demo.cpp
)
add_executable(plussmic_bike_sharing_eda_dfplot_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/bike_sharing_eda_dfplot.cpp
)
add_executable(plussmic_sp500_eda_ml_prep_notebook_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/sp500_eda_ml_prep_notebook_demo.cpp
)
add_executable(plussmic_option_smile_skew_surface_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/option_smile_skew_surface_demo.cpp
)
add_executable(plussmic_atlas_plot_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/atlas_plot_demo.cpp
)
add_executable(plussmic_time_series_accessor_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/time_series_accessor_demo.cpp
)
add_executable(plussmic_dataframe_tl_batched_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_tl_batched_demo.cpp
)
add_executable(plussmic_csv_engine_bench
  ${CMAKE_CURRENT_SOURCE_DIR}/csv_engine_bench.cpp
)
add_executable(plussmic_native_gap_api_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/pluss_native_gap_api_demo.cpp
)
add_executable(plussmic_hierarchical_cases_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/hierarchical_cases_demo.cpp
)
add_executable(plussmic_flash_dataset_bridge_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/flash_dataset_bridge_demo.cpp
)
add_executable(plussmic_bi_demo_suite
  ${CMAKE_CURRENT_SOURCE_DIR}/bi_demo_suite.cpp
)
# Scratch doc-verification target for the Nexus API reference rewrite:
# not a permanent example, just a compile+run harness for confirming
# real output before it gets pasted into docs/sphinx/api/nexus*/*.md.
# Source lives in scratch/nexus_docs/, edited freely, never installed.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/nexus_docs/nexus_docs_verify.cpp")
  add_executable(nexus_docs_verify
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/nexus_docs/nexus_docs_verify.cpp
  )
  target_link_libraries(nexus_docs_verify PRIVATE PlussMic)
  set_target_properties(nexus_docs_verify PROPERTIES CXX_STANDARD 20)
endif()
# Same idea, for the Series API reference gap-filling work (fft,
# view_as_real, clamp/clamp_, the in-place `_`-suffixed methods): not a
# permanent example, just a compile+run harness. Source lives in
# scratch/series_docs_gap/, edited freely, never installed.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/series_docs_verify.cpp")
  add_executable(series_docs_verify
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/series_docs_verify.cpp
  )
  target_link_libraries(series_docs_verify PRIVATE PlussMic)
  set_target_properties(series_docs_verify PROPERTIES CXX_STANDARD 20)
endif()
# Batched verification for the in-place (`_`-suffixed) method sweep: one
# throwaway harness per batch of methods being folded into their out-of-place
# counterpart pages. Not permanent examples.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_a.cpp")
  add_executable(inplace_batch_a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_a.cpp
  )
  target_link_libraries(inplace_batch_a PRIVATE PlussMic)
  set_target_properties(inplace_batch_a PROPERTIES CXX_STANDARD 20)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_b.cpp")
  add_executable(inplace_batch_b
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_b.cpp
  )
  target_link_libraries(inplace_batch_b PRIVATE PlussMic)
  set_target_properties(inplace_batch_b PROPERTIES CXX_STANDARD 20)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_c.cpp")
  add_executable(inplace_batch_c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_c.cpp
  )
  # Linked against the lighter PlussQit::CodeM (just the Series engine)
  # rather than PlussMic, to avoid an unrelated pre-existing PlussMic
  # compile error in Stream.cpp that
  # this harness doesn't need to exercise at all.
  target_link_libraries(inplace_batch_c PRIVATE PlussQit::CodeM)
  set_target_properties(inplace_batch_c PROPERTIES CXX_STANDARD 20)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_d.cpp")
  add_executable(inplace_batch_d
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_d.cpp
  )
  target_link_libraries(inplace_batch_d PRIVATE PlussQit::CodeM)
  set_target_properties(inplace_batch_d PROPERTIES CXX_STANDARD 20)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_e.cpp")
  add_executable(inplace_batch_e
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_e.cpp
  )
  target_link_libraries(inplace_batch_e PRIVATE PlussQit::CodeM)
  set_target_properties(inplace_batch_e PROPERTIES CXX_STANDARD 20)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_f.cpp")
  add_executable(inplace_batch_f
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../scratch/series_docs_gap/inplace_batch_f.cpp
  )
  target_link_libraries(inplace_batch_f PRIVATE PlussQit::CodeM)
  set_target_properties(inplace_batch_f PROPERTIES CXX_STANDARD 20)
endif()

target_link_libraries(plussmic_dataframe_initial_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_ab_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_slate_a_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_b_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_slate_b_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_cd_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_slate_cd_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_view_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_describe_demo PRIVATE PlussMic)
target_link_libraries(plussmic_display_policy_demo PRIVATE PlussMic)
target_link_libraries(plussmic_df_ex_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_versatile_methods_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_e_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_efg_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_hijk_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_rich_reference_demo PRIVATE PlussMic)
target_link_libraries(plussmic_us_accidents_eda_verbs_demo PRIVATE PlussMic)
target_link_libraries(plussmic_eda_us_accidents_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_plot_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_slate_values_demo PRIVATE PlussMic)
target_link_libraries(plussmic_loader_saver_parity_demo PRIVATE PlussMic)
target_link_libraries(plussmic_bike_sharing_eda_dfplot_demo PRIVATE PlussMic)
target_link_libraries(plussmic_sp500_eda_ml_prep_notebook_demo PRIVATE PlussMic)
target_link_libraries(plussmic_option_smile_skew_surface_demo PRIVATE PlussMic)
target_link_libraries(plussmic_atlas_plot_demo PRIVATE PlussMic)
target_link_libraries(plussmic_time_series_accessor_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_tl_batched_demo PRIVATE PlussMic)
target_link_libraries(plussmic_csv_engine_bench PRIVATE PlussMic)
target_link_libraries(plussmic_native_gap_api_demo PRIVATE PlussMic)
target_link_libraries(plussmic_hierarchical_cases_demo PRIVATE PlussMic)
target_link_libraries(plussmic_flash_dataset_bridge_demo PRIVATE PlussMic)
target_link_libraries(plussmic_bi_demo_suite PRIVATE PlussMic)

target_compile_definitions(plussmic_csv_engine_bench PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_initial_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_ab_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_slate_a_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_b_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_slate_b_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_cd_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_slate_cd_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_view_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_describe_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_display_policy_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_df_ex_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_versatile_methods_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_e_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_efg_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_hijk_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_rich_reference_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_us_accidents_eda_verbs_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_eda_us_accidents_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_plot_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_slate_values_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_loader_saver_parity_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_bike_sharing_eda_dfplot_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_sp500_eda_ml_prep_notebook_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_option_smile_skew_surface_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_atlas_plot_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_time_series_accessor_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_tl_batched_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_csv_engine_bench PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_native_gap_api_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_flash_dataset_bridge_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_bi_demo_suite PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
