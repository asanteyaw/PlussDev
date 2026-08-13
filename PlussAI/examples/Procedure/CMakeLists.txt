if(NOT TARGET pluss)
  return()
endif()

set(PROCEDURE_DEMOS
  procedure_serial_series_demo
  procedure_soft_failure_demo
  procedure_row_guard_demo
  procedure_future_guard_demo
  procedure_custom_module_demo
  procedure_dataframe_callable_demo
  procedure_labeled_model_demo
  procedure_sink_chain_demo
  procedure_result_artifact_demo
  procedure_disable_audit_demo
  procedure_explicit_ports_demo
  procedure_vector_text_artifact_demo
  procedure_scalar_artifact_demo
  procedure_empty_sink_demo
  procedure_options_metadata_demo
  procedure_multi_artifact_join_demo
  procedure_raw_series_parallel_demo
  procedure_safety_contract_demo
  procedure_registry_persistence_demo
  procedure_lymph_policy_payload_demo
  procedure_lymph_model_workflow_demo
  procedure_nodek_ice_demo
  procedure_nodeb_text_demo
  procedure_bundle_audit_demo
)

foreach(demo ${PROCEDURE_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_link_libraries(plussai_${demo} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_include_directories(plussai_${demo} PRIVATE
    ${CMAKE_SOURCE_DIR}/PlussMic/include
    ${CMAKE_SOURCE_DIR}/PlussMic/src
  )
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
endforeach()
