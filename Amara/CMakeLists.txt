cmake_minimum_required(VERSION 3.26)
project(Amara VERSION "0.0.1")
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(USE_HERMES)

add_subdirectory(external)
if(NOT DEFINED TARGET_PLATFORM)
    message(FATAL_ERROR "You must define -DTARGET_PLATFORM=<windows|linux|android|ios>")
endif()
file(GLOB_RECURSE AMARA_CORE_SOURCES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
)
add_library(amaracore STATIC ${AMARA_CORE_SOURCES})

if(TARGET_PLATFORM STREQUAL "windows")
    target_compile_definitions(amaracore PUBLIC PLATFORM_WINDOWS)
elseif(TARGET_PLATFORM STREQUAL "linux")
    target_compile_definitions(amaracore PUBLIC PLATFORM_LINUX)
elseif(TARGET_PLATFORM STREQUAL "android")
    target_compile_definitions(amaracore PUBLIC PLATFORM_ANDROID)
elseif(TARGET_PLATFORM STREQUAL "ios")
    target_compile_definitions(amaracore PUBLIC PLATFORM_IOS)
endif()


add_subdirectory(platform/${TARGET_PLATFORM})





add_library(amara INTERFACE)

target_include_directories(amaracore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_jsx
        old/Engine.cpp)

target_link_libraries(amaracore PRIVATE
        libhermes jsi masharifcore
)





#add_executable(testtt r.cpp ui/Widget.cpp ui/ComponentContext.cpp runtime/hermes/Engine.cpp runtime/hermes/WidgetHostWrapper.cpp runtime/hermes/InstallEngine.cpp runtime/hermes/HermesPropMap.cpp utils/css/CssUtils.cpp utils/css/Style.cpp runtime/hermes/HermesWidgetHolder.cpp runtime/hermes/HermesArray.cpp)
#target_link_libraries(test_jsx PUBLIC libhermes jsi masharifcore)
#
#target_include_directories(test_jsx PUBLIC ${MASHARIF_CORE})
#
#target_link_libraries(testtt PUBLIC libhermes jsi masharifcore)
#target_include_directories(testtt PUBLIC ${MASHARIF_CORE})