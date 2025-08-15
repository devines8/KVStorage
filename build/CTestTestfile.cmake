# CMake generated Testfile for 
# Source directory: /home/devines/vk_test_assignment/Storage
# Build directory: /home/devines/vk_test_assignment/Storage/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(kv_storage_tests "/home/devines/vk_test_assignment/Storage/build/kv_storage_tests")
set_tests_properties(kv_storage_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/devines/vk_test_assignment/Storage/CMakeLists.txt;37;add_test;/home/devines/vk_test_assignment/Storage/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
