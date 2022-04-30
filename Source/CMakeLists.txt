add_library(fifo_calculator_library)

target_compile_features(fifo_calculator_library PUBLIC cxx_std_23)

target_sources(fifo_calculator_library PRIVATE
    "fifo_calculator.cppm"
)

add_executable(fifo_calculator)

target_link_libraries(fifo_calculator PRIVATE fifo_calculator_library)

target_sources(fifo_calculator PRIVATE "main.cpp")

if(BUILD_TESTING)
   add_executable(fifo_calculator_tests)
   target_link_libraries(fifo_calculator_tests PRIVATE fifo_calculator_library)

   find_package(Catch2 CONFIG REQUIRED)
   target_link_libraries(fifo_calculator_tests PRIVATE Catch2::Catch2)

   target_sources(fifo_calculator_tests PRIVATE "fifo_calculator.tests.cpp")

   include(Catch)
   catch_discover_tests(fifo_calculator_tests)
endif()

