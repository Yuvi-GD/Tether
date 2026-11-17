add_executable(tether_sandbox
    sandbox/app/app.c
    sandbox/ui/widgets/sandbox_widgets_zoo.c
    sandbox/lab/lab.c
    sandbox/results/results.c
    sandbox/events/sandbox_events.c
    sandbox/platform/sandbox_platform.c
)
target_include_directories(tether_sandbox PRIVATE
    sandbox/platform
    sandbox/events
    sandbox/app sandbox/ui
    sandbox/ui/widgets
    sandbox/lab
    sandbox/results
)
target_link_libraries(tether_sandbox PRIVATE tether_runtime tether_run_support)
if(WIN32)
    target_link_libraries(tether_sandbox PRIVATE comdlg32)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    tether_configure_wasm_target(tether_sandbox
        EXPORTED_FUNCTIONS
            _sandbox_platform_on_file_loaded
            _sandbox_platform_get_path_buf
            _sandbox_platform_on_sync_done
    )
endif()
tether_target_assets(tether_sandbox
    DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/sandbox/ui/scripts@UI"
        "${CMAKE_CURRENT_SOURCE_DIR}/Font@Font"
)
target_compile_definitions(tether_sandbox PRIVATE SANDBOX_REPORT_DIR="report/workbench")
if(TARGET tether_bench AND TARGET tether_test)
    add_dependencies(tether_sandbox tether_test tether_bench)
    target_compile_definitions(tether_sandbox PRIVATE SANDBOX_HAS_RUNNERS=1)
else()
    target_compile_definitions(tether_sandbox PRIVATE SANDBOX_HAS_RUNNERS=0)
endif()
if(TETHER_BUILD_TESTS AND TETHER_REGISTER_RENDER_TESTS AND TARGET tether_bench)
    # This deterministic failed result exercises Workbench failure presentation.
    # Generate it in the build tree; it is not a captured benchmark output.
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/failed_report.json" [=[
{
  "schema_version": 1,
  "project": "Tether",
  "kind": "correctness",
  "status": "failed",
  "summary": "Expected failure display fixture",
  "metrics": {"checks": 1, "failures": 1, "measured_steps": 0},
  "samples": []
}
]=])
    add_test(NAME sandbox_launch_results COMMAND tether_sandbox --smoke)
    set_tests_properties(sandbox_launch_results PROPERTIES LABELS "rendering;integration" RUN_SERIAL TRUE TIMEOUT 30)
    add_test(NAME sandbox_invalid_report COMMAND tether_sandbox --smoke-invalid-report
        "${CMAKE_CURRENT_BINARY_DIR}/nonexistent-report-for-smoke.json")
    set_tests_properties(sandbox_invalid_report PROPERTIES LABELS "rendering;integration" RUN_SERIAL TRUE TIMEOUT 30)
    add_test(NAME sandbox_failure_display COMMAND tether_sandbox --smoke-report
        "${CMAKE_CURRENT_BINARY_DIR}/failed_report.json" FAILED)
    set_tests_properties(sandbox_failure_display PROPERTIES LABELS "rendering;integration" RUN_SERIAL TRUE TIMEOUT 30)
endif()
