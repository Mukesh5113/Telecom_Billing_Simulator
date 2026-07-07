cmake_minimum_required(VERSION 3.16)

project(TelecomBillingSimulator
        VERSION 1.0.0
        DESCRIPTION "Telecom Billing Simulator (C++17)"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Core library: all billing logic lives here so the CLI stays thin/testable.
# ---------------------------------------------------------------------------
add_library(tbs STATIC
    src/io/CsvReader.cpp
    src/io/JsonLoader.cpp
    src/io/DataLoader.cpp
    src/io/InvoiceWriter.cpp
    src/engines/RatingEngine.cpp
    src/engines/RecurringChargeEngine.cpp
    src/engines/DiscountEngine.cpp
    src/engines/TaxEngine.cpp
    src/engines/BillingEngine.cpp
    src/engines/PrepaidEngine.cpp
)

target_include_directories(tbs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ---------------------------------------------------------------------------
# CLI executable (binary name: tbs)
# ---------------------------------------------------------------------------
add_executable(tbs_app src/main.cpp)
set_target_properties(tbs_app PROPERTIES OUTPUT_NAME tbs)
target_link_libraries(tbs_app PRIVATE tbs)

# ---------------------------------------------------------------------------
# Tests (self-contained, no external framework)
# ---------------------------------------------------------------------------
enable_testing()
add_executable(tbs_tests tests/test_main.cpp)
target_link_libraries(tbs_tests PRIVATE tbs)
add_test(NAME tbs_tests COMMAND tbs_tests)
