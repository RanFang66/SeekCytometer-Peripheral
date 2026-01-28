cmake_minimum_required(VERSION 3.16)

project(SeekCytometer_Peripheral VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets SerialPort LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(TS_FILES SeekCytometer_Peripheral_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        ${TS_FILES}
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SeekCytometer_Peripheral
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        MicroFluidicWidget.cpp MicroFluidicWidget.h OpticsControlWidget.cpp OpticsControlWidget.h SampleChipWidget.cpp SampleChipWidget.h
        ModbusMaster.h ModbusMaster.cpp
        SerialPortConfigWidget.h SerialPortConfigWidget.cpp
        ModbusRegistersTable.h
        ToggleSwitch.h ToggleSwitch.cpp
        ToggleRunButton.h ToggleRunButton.cpp
        ArrowButton.h ArrowButton.cpp
        StatusLight.h StatusLight.cpp
        SampleParaWidget.h SampleParaWidget.cpp
        PZTCtrlWidget.h PZTCtrlWidget.cpp
        MFCParasSetWidget.h MFCParasSetWidget.cpp
        resource.qrc
        app_icon.rc
        TempControlWidget.h TempControlWidget.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SeekCytometer_Peripheral APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(SeekCytometer_Peripheral SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(SeekCytometer_Peripheral
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SeekCytometer_Peripheral PRIVATE Qt${QT_VERSION_MAJOR}::Core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SeekCytometer_Peripheral)
endif()
set_target_properties(SeekCytometer_Peripheral PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SeekCytometer_Peripheral
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SeekCytometer_Peripheral)
endif()
