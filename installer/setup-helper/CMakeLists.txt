add_executable(
    GenericInputDeviceSetup
    src/main.cpp
)

target_compile_definitions(
    GenericInputDeviceSetup
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

target_link_libraries(
    GenericInputDeviceSetup
    PRIVATE
        setupapi
        cfgmgr32
        newdev
        advapi32
)

if(MINGW)
    target_compile_options(
        GenericInputDeviceSetup
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

    target_link_options(
        GenericInputDeviceSetup
        PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
    )
endif()