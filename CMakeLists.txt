cmake_minimum_required(VERSION 3.16)

project(minimal_subsequential_transducer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_SOURCES
    transducer.cpp
    real_time_transducer.cpp
    subsequential_transducer.cpp
    new_automaton.cpp
)

add_executable(minimal_subsequential_transducer
    ${PROJECT_SOURCES}
    main.cpp
)

target_compile_options(minimal_subsequential_transducer PRIVATE
    -Wall -Wextra -pedantic
)

enable_testing()

add_executable(smoke_tests
    ${PROJECT_SOURCES}
    tests/smoke_tests.cpp
)

target_compile_options(smoke_tests PRIVATE
    -Wall -Wextra -pedantic
)

add_test(NAME smoke_tests COMMAND smoke_tests)

