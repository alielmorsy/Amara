cmake_minimum_required(VERSION 3.26)
project(AmaraWindowsLauncher)

# Source files
file(GLOB SOURCES
        "${PROJECT_SOURCE_DIR}/*.cpp"
        "${PROJECT_SOURCE_DIR}/win32/*.cpp"
)

# Build executable
#add_executable(amara_launcher WIN32 ${SOURCES})

# Link core
#target_link_libraries(amara_launcher PRIVATE amaracore)

# Include headers
#target_include_directories(amara_launcher PRIVATE
#        ${PROJECT_SOURCE_DIR}/../../include
#)