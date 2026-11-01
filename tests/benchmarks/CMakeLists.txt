add_executable(tether_bench ../runners/main.c)
target_compile_definitions(tether_bench PRIVATE TOOL_IS_TEST=0)
target_link_libraries(tether_bench PRIVATE tether_workloads)
# Benchmarks remain explicitly invoked; ordinary CI does not rank performance.
