add_executable(tether_ui_contract_test ui_contract_test.c)
target_link_libraries(tether_ui_contract_test PRIVATE tether_runtime)
foreach(mode IN ITEMS retained volatile)
    add_test(NAME ui_contract_${mode} COMMAND tether_ui_contract_test ${mode}
        "${PROJECT_SOURCE_DIR}/examples/Font/Roboto-Regular.ttf"
        "${CMAKE_CURRENT_BINARY_DIR}/${mode}-scene.yaml")
    set_tests_properties(ui_contract_${mode} PROPERTIES LABELS "rendering;integration" RUN_SERIAL TRUE TIMEOUT 30)
    add_test(NAME rendering_${mode} COMMAND tether_test --scenario ui --mode ${mode}
        --count 100 --spawn 7 --delete 5 --mutate 21 --frames 8
        --output "$<TARGET_FILE_DIR:tether_test>/report/rendering/${mode}.json")
    set_tests_properties(rendering_${mode} PROPERTIES LABELS "rendering" RUN_SERIAL TRUE TIMEOUT 60)
endforeach()
