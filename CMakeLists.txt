cmake_minimum_required(VERSION 3.14)

project(VoiceRecognizer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(VCPKG_ROOT "D:/vcpkg/vcpkg")
set(VCPKG_TRIPLET "x64-windows")

# Manually find library and include
find_library(WHISPER_LIB NAMES whisper PATHS "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/lib")
find_path(WHISPER_INCLUDE_DIR NAMES whisper.h PATHS "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/include")

if (NOT WHISPER_LIB OR NOT WHISPER_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find whisper in vcpkg path")
endif()

# Export as variables
set(WHISPER_INCLUDE_DIR ${WHISPER_INCLUDE_DIR})
set(WHISPER_LIB ${WHISPER_LIB})

message(STATUS "Found WHISPERCPP: ${WHISPER_LIB}")
message(STATUS "Include WHISPERCPP: ${WHISPER_INCLUDE_DIR}")

find_library(PORTAUDIO_LIB portaudio PATHS "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/lib" NO_DEFAULT_PATH)
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h PATHS "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/include" NO_DEFAULT_PATH)


if (NOT PORTAUDIO_LIB OR NOT PORTAUDIO_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find portaudio in vcpkg path")
endif()

# Export as variables
set(PORTAUDIO_INCLUDE_DIR ${PORTAUDIO_INCLUDE_DIR})
set(PORTAUDIO_LIB ${PORTAUDIO_LIB})

message(STATUS "Found PORTAUDIO: ${PORTAUDIO_LIB}")
message(STATUS "Include PORTAUDIO: ${PORTAUDIO_INCLUDE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick Multimedia)

set(PROJECT_SOURCES
        main.cpp
        qml.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VoiceRecognizer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        stthandler.h stthandler.cpp
    )
target_include_directories(VoiceRecognizer PRIVATE
    ${WHISPER_INCLUDE_DIR}
)

# Link Whisper library
target_link_libraries(VoiceRecognizer PRIVATE
    ${WHISPER_LIB}
)

target_include_directories(VoiceRecognizer PRIVATE
    ${PORTAUDIO_INCLUDE_DIR}
)

# Link PORTAUDIO library
target_link_libraries(VoiceRecognizer PRIVATE
    ${PORTAUDIO_LIB}
)
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET VoiceRecognizer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(VoiceRecognizer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(VoiceRecognizer
          ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(VoiceRecognizer
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::Multimedia)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VoiceRecognizer)
endif()
set_target_properties(VoiceRecognizer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VoiceRecognizer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(VoiceRecognizer)
    qt_finalize_executable(VoiceRecognizer)
endif()
