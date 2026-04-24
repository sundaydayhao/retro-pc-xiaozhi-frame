cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "1.0.0")

# Suppress common warnings from generated or third-party code
add_compile_options(-Wno-missing-field-initializers)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiaozhi_chat)
