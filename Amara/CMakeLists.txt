project(AmaraCore)
if (MSVC)
    set(HERMES_PATH "E:/work/repos_i_wont_use/hermes")  # Windows-compatible path
else ()
    set(HERMES_PATH "/mnt/e/work/repos_i_wont_use/hermes")  # Linux path
endif ()
# Release flags
add_subdirectory("${HERMES_PATH}" "${CMAKE_BINARY_DIR}/hermes_build")
add_compile_definitions(USE_HERMES)

add_executable(test_jsx
        old/Engine.cpp)
add_executable(testtt r.cpp ui/Widget.cpp ui/ComponentContext.cpp runtime/hermes/Engine.cpp runtime/hermes/WidgetHostWrapper.cpp runtime/hermes/InstallEngine.cpp runtime/hermes/HermesPropMap.cpp utils/css/CssUtils.cpp utils/css/Style.cpp runtime/hermes/HermesWidgetHolder.cpp runtime/hermes/HermesArray.cpp)
target_link_libraries(test_jsx PUBLIC libhermes jsi masharifcore)

target_include_directories(test_jsx PUBLIC ${MASHARIF_CORE})

target_link_libraries(testtt PUBLIC libhermes jsi masharifcore)
target_include_directories(testtt PUBLIC ${MASHARIF_CORE})