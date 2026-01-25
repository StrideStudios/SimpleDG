# CMake Parent File Requirements
cmake_minimum_required(VERSION 3.31...4.2)

set(CMAKE_CXX_STANDARD 17)

# Project Definition
project(
        SimpleDGTest
)

function(addTest Name)

    add_executable(SimpleDG-${Name}
        "${Name}.cpp"
    )
    # Prevent MSVC from complaining about template usage
    target_compile_options(SimpleDG-${Name} PRIVATE /bigobj)
    target_include_directories(SimpleDG-${Name} PUBLIC ../include)
    target_link_libraries(SimpleDG-${Name} SimpleDG)
endfunction()

addTest(Test)
