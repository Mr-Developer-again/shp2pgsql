cmake_minimum_required(VERSION 3.5)

project(shp2pgsql VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

file(GLOB PROJECT_SOURCES "src/*.cpp")
file(GLOB PROJECT_HEADERS "include/*.h")
file(GLOB PROJECT_UI      "res/*.ui")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(shp2pgsql
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
		${PROJECT_HEADERS}
		${PROJECT_UI}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET shp2pgsql APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(shp2pgsql SHARED
            ${PROJECT_SOURCES}
			${PROJECT_HEADERS}
			${PROJECT_UI}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(shp2pgsql
            ${PROJECT_SOURCES}
			${PROJECT_HEADERS}
			${PROJECT_UI}
        )
    endif()
endif()

target_link_libraries(shp2pgsql PRIVATE Qt${QT_VERSION_MAJOR}::Widgets "-lpq")

set_target_properties(shp2pgsql PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS shp2pgsql
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(shp2pgsql)
endif()
