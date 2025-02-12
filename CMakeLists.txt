cmake_minimum_required(VERSION 3.0.0)
project(aesd-assignments)
# A list of all automated test source files
# At minimum it should include the files in the test/assignmentX directory
# Students can also add paths to their own test files using a ../ prefix to
# reference this working directory

set(CMAKE_C_FLAGS "-pthread")

set(AUTOTEST_SOURCES
    test/assignment1/Test_hello.c
    test/assignment1/Test_assignment_validate.c
    test/assignment4/Test_threading.c
)
# A list of all files containing test code that is used for assignment validation
set(TESTED_SOURCE
    ../examples/autotest-validate/autotest-validate.c
    ../examples/threading/threading.c
)
add_subdirectory(assignment-autotest)

# Detect cross-compilation
if(DEFINED ENV{CROSS_COMPILE})
    set(CROSS_COMPILE_PREFIX $ENV{CROSS_COMPILE})
else()
    set(CROSS_COMPILE_PREFIX "")
endif()

set(CMAKE_C_COMPILER "${CROSS_COMPILE_PREFIX}gcc")
# Define the writer application sources
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/finder-app)
set(WRITER_SOURCE ${SOURCE_DIR}/writer.c)

# Create the writer executable
add_executable(writer ${WRITER_SOURCE})
install(TARGETS writer DESTINATION bin)
install(TARGETS writer DESTINATION bin)
