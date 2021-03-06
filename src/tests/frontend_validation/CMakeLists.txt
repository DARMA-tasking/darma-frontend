
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

include(GoogleTest)

include_directories(${CMAKE_CURRENT_LIST_DIR})
function(add_unit_test test_name)

  set(frontendtestfiles "${frontendtestfiles};${test_name}.cc" PARENT_SCOPE)

  add_executable(${test_name} ${test_name}.cc gtest_main.cc)
  target_link_libraries(${test_name} darma_frontend::darma_frontend)
  target_link_libraries(${test_name} GTest::GTest GTest::Main)
  target_link_libraries(${test_name} ${GMOCK_BOTH_LIBRARIES})

  add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")

  set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
    FAIL_REGULAR_EXPRESSION "FAILED;timeout"
    PASS_REGULAR_EXPRESSION "PASSED")
  if (DARMA_COVERAGE)
    setup_target_for_coverage(frontend_coverage ${test_name} coverage)
  endif()
endfunction()

add_unit_test(test_read_access)
add_unit_test(test_initial_access)
add_unit_test(test_create_work)
add_unit_test(test_access_handle)
add_unit_test(test_functor)
add_unit_test(test_functor_migrate)
add_unit_test(test_serialize)
add_unit_test(test_sso_key)
add_unit_test(test_collectives)
add_unit_test(test_array)
add_unit_test(test_index_range)
add_unit_test(test_mpi_interop)
add_unit_test(test_make_key_functor)
add_unit_test(test_resource_count)
add_unit_test(test_publish)
add_unit_test(test_create_parallel_for)
add_unit_test(test_task_collection)
add_unit_test(test_create_work_if)
add_unit_test(test_create_work_while)
add_unit_test(test_anti_flows)
add_unit_test(test_darma_region)
add_unit_test(test_lambda_migrate)

#add_executable(run_all_frontend_tests ${frontendtestfiles} gtest_main.cc)

#target_link_libraries(run_all_frontend_tests ${DARMA_BACKEND_LIBNAME} ${GMOCK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
#if (DARMA_COVERAGE)
#setup_target_for_coverage(frontend_coverage run_all_frontend_tests coverage)
#endif()
