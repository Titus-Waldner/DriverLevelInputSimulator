add_executable(
    DriverLevelInputSimulator
    src/main.cpp
)

target_include_directories(
    DriverLevelInputSimulator
    PRIVATE
        ${PROJECT_SOURCE_DIR}/shared/include
)

target_compile_definitions(
    DriverLevelInputSimulator
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

if(MINGW)
    target_compile_options(
        DriverLevelInputSimulator
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

    target_link_options(
        DriverLevelInputSimulator
        PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
    )
endif()