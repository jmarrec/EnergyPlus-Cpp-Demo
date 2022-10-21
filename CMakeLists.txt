cmake_minimum_required(VERSION 3.17.0)
# cmake_policy(SET CMP0048 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific extensions, for eg on GCC use -std=c++1z (=c++17) and not -std=gnu++17
set(CMAKE_CXX_EXTENSIONS OFF)

# Use ccache is available, has to be before "project()"
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  # Support Unix Makefiles and Ninja
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(EnergyPlus-Cpp-Demo VERSION 0.0.1)

# The PROJECT_SOURCE_DIR here is root of this repo, which we will use instead of using CMAKE_SOURCE_DIR
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# We will use BINARY_DIR instead of CMAKE_BINARY_DIR
set(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}")

#MESSAGE(FATAL_ERROR "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}, BINARY_DIR=${BINARY_DIR}, PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}, CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}, CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

#if(POLICY CMP0020)
  #cmake_policy(SET CMP0020 NEW)
#endif()
#if(POLICY CMP0022)
  #cmake_policy(SET CMP0022 NEW)
#endif()
#if(POLICY CMP0042)
  #cmake_policy(SET CMP0042 NEW)
#endif()
#if(POLICY CMP0054)
  #cmake_policy(SET CMP0054 NEW)
#endif()
#if(POLICY CMP0077)
  #cmake_policy(SET CMP0077 NEW)
#endif()

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

###############################################################################
#                                  N I N J A                                  #
###############################################################################
# Ninja support: has to be atop for it to take effect before anything else is done
# Add Color Output if Using Ninja
macro(AddCXXFlagIfSupported flag test)
  CHECK_CXX_COMPILER_FLAG(${flag} ${test})
  if(${${test}})
    message("adding ${flag}")
    target_compile_options(project_options INTERFACE "${flag}")
  endif()
endmacro()

if("Ninja" STREQUAL ${CMAKE_GENERATOR})
  # Clang
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    AddCXXFlagIfSupported(-fcolor-diagnostics COMPILER_SUPPORTS_fcolor-diagnostics)
  endif()

  # g++
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # For some reason it doesn't say its supported, but it works...
    # AddCXXFlagIfSupported(-fdiagnostics-color COMPILER_SUPPORTS_fdiagnostics-color)
    message(STATUS "Ninja: Forcing -fdiagnostics-color=always")
    target_compile_options(project_options INTERFACE -fdiagnostics-color=always)
  endif()
endif()

# Xcode/Ninja generators undefined MAKE
if(CMAKE_GENERATOR MATCHES "Make")
  set(MAKE "$(MAKE)")
else()
  set(MAKE make)
endif()

# Enable runtime checking features: TSAN, ASAN, UBSAN
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

###############################################################################
#                         C M A K E    C O N T R O L                          #
###############################################################################

# High level project configuration
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")

set(LIBRARY_SEARCH_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug")

###############################################################################
#          C O M P I L E R    A N D    S Y S T E M    O P T I O N S           #
###############################################################################

# Compiler and system specific options
if(UNIX)

  # all warnings
  target_compile_options(project_options INTERFACE -fPIC -fno-strict-aliasing -Winvalid-pch)
  # Treat all warnings as errors, extra errors, and be pedantic
  target_compile_options(project_options INTERFACE -Wall -Wextra -Werror -pedantic-errors -pedantic) # Turn on warnings
  if(APPLE)
     target_compile_options(project_options INTERFACE -Wno-overloaded-virtual -ftemplate-depth=1024)
  endif()

  # Note: CMAKE_CXX_STANDARD set to 20 should already take care of adding -std=c++20 or equivalent
endif()

#######################################################################
#                              C O N A N                              #
#######################################################################

# This is done via the connafile.txt with CMakeToolChain generator, called beforehand
# Gotta include the project binary dir so we find the cmake stuff added by conan
message("PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")
include_directories(${PROJECT_BINARY_DIR})

find_package(fmt REQUIRED)
find_package(ftxui REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(ctre REQUIRED)

###############################################################################
#                             E N E R G Y P L U S                             #
###############################################################################

find_package(energyplus REQUIRED)
message("energyplus=${energyplus}")

###############################################################################
#                              E X E C U T A B L E                            #
###############################################################################

option(INCLUDE_WHAT_YOU_USE OFF)

# Enable globally:
if (INCLUDE_WHAT_YOU_USE)
  find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path}
    -Xiwyu --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/iwyu.imp
    -Xiwyu --max_line_length=150
    -Xiwyu --no_fwd_decls # Annoys me when proposing to forward declare EnergyPlus::Error and ftxui::Event
  )
  message("CMAKE_CXX_INCLUDE_WHAT_YOU_USE=${CMAKE_CXX_INCLUDE_WHAT_YOU_USE}")
endif()

find_program(iwyu_path NAMES include-what-you-use iwyu)

add_executable(epcli
  src/main.cpp
  src/MainComponent.hpp
  src/MainComponent.cpp

  src/AboutComponent.hpp
  src/AboutComponent.cpp

  src/LogDisplayer.hpp
  src/LogDisplayer.cpp

  src/ErrorMessage.hpp
  src/ErrorMessage.cpp

  src/EnergyPlus.hpp
  src/EnergyPlus.cpp

  src/sqlite/PreparedStatement.hpp
  src/sqlite/PreparedStatement.cpp
  src/sqlite/SQLiteReports.hpp
  src/sqlite/SQLiteReports.cpp

  src/utilities/ASCIIStrings.hpp

  # TODO: TEMP, pending new release of FTXUI
  src/ftxui/modal.hpp
  src/ftxui/modal.cpp
)

target_link_libraries(epcli
  PRIVATE
  project_options
  fmt::fmt
  # ftxui::screen
  # ftxui::dom
  # ftxui::component
  ftxui::ftxui
  SQLite::SQLite3
  ctre::ctre
  energyplus::energyplusapi
)

add_custom_command(
  TARGET epcli
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:energyplus::energyplusapi>" "$<TARGET_FILE_DIR:epcli>"
  # TODO: WORKAROUND
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:energyplus::energyplusapi>/$<$<PLATFORM_ID:Darwin>:libpython3.8.dylib>$<$<PLATFORM_ID:Linux>:libpython3.8.so.1.0>$<$<PLATFORM_ID:Windows>:python38.dll>" "$<TARGET_FILE_DIR:epcli>"
  # Example files to play with
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:energyplus::energyplusapi>/ExampleFiles/1ZoneUncontrolled.idf" "$<TARGET_FILE_DIR:epcli>/in.idf"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:energyplus::energyplusapi>/WeatherData/USA_IL_Chicago-OHare.Intl.AP.725300_TMY3.epw" "$<TARGET_FILE_DIR:epcli>/in.epw"
)
if(APPLE)
  add_custom_command(
    TARGET epcli
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:energyplus::energyplusapi>/libintl.8.dylib" "$<TARGET_FILE_DIR:epcli>"
  )
endif()

target_compile_definitions(epcli PRIVATE ENERGYPLUS_ROOT="$<TARGET_FILE_DIR:energyplus::energyplusapi>")

# enable_testing()
# include(GoogleTest)
# gtest_discover_tests(testlib_tests
#   # WORKING_DIRECTORY Testing
# )

set(LIB_DESTINATION_DIR "lib")
if(WIN32)
  # No Rpaths on Windows
  set(LIB_DESTINATION_DIR "bin")
endif()

get_target_property(LIBAPI energyplus::energyplusapi IMPORTED_LOCATION_RELEASE)

install(FILES "$<TARGET_FILE_DIR:energyplus::energyplusapi>/$<$<PLATFORM_ID:Darwin>:libpython3.8.dylib>$<$<PLATFORM_ID:Linux>:libpython3.8.so.1.0>$<$<PLATFORM_ID:Windows>:python38.dll>" DESTINATION ${LIB_DESTINATION_DIR} COMPONENT "CLI")


add_custom_target(genexdebug COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE_NAME:energyplus::energyplusapi>")

#install(IMPORTED_RUNTIME_ARTIFACTS energyplus::energyplusapi LIBRARY DESTINATION ${LIB_DESTINATION_DIR} COMPONENT "CLI")
install(FILES ${LIBAPI} DESTINATION ${LIB_DESTINATION_DIR} COMPONENT "CLI")

install(TARGETS epcli DESTINATION bin COMPONENT "CLI")

if(APPLE)
  set_target_properties(epcli PROPERTIES
    INSTALL_RPATH "@executable_path;@executable_path/../lib/"
  )

  install(FILES "$<TARGET_FILE_DIR:energyplus::energyplusapi>/libintl.8.dylib" DESTINATION ${LIB_DESTINATION_DIR} COMPONENT "CLI")

  install(CODE [[
    execute_process(COMMAND "install_name_tool" -change "$<TARGET_PROPERTY:energyplus::energyplusapi,IMPORTED_SONAME_RELEASE>" "@rpath/$<TARGET_FILE_NAME:energyplus::energyplusapi>" "${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:epcli>")
  ]]
    COMPONENT "CLI"
  )

elseif(UNIX)
  set_target_properties(epcli PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/")
endif()


include(CPack)
