qt_add_library(common SHARED
    src/pse-common/random.cpp
    src/pse-common/random.h
    src/pse-common/utility.cpp
    src/pse-common/utility.h
    src/pse-common/types.h
    src/pse-common/common_autoport.h
)

target_compile_definitions(common PRIVATE COMMON_LIBRARY)

target_include_directories(common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(common
    PUBLIC
        Qt6::Core
        Qt6::Qml
)
