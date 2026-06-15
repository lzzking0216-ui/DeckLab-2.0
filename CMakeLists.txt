cmake_minimum_required(VERSION 3.16)
project(DeckLab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(DeckLab
    main.cpp
    mainwindow.h
    mainwindow.cpp

    # 通讯层（Modbus TCP）
    communication/RegisterMap.h
    communication/ModbusTcpClient.h
    communication/ModbusTcpClient.cpp
    communication/PlcWorker.h
    communication/PlcWorker.cpp

    # 电机控制（卷筒电机 + RSD 丝杆）
    motor/MotorController.h
    motor/MotorController.cpp
    motor/MotorPanel.h
    motor/MotorPanel.cpp

    # 六自由度动平台（UDP）
    platform/PlatformData.h
    platform/UdpPlatformClient.h
    platform/UdpPlatformClient.cpp
    platform/PlatformWorker.h
    platform/PlatformWorker.cpp
    platform/PlatformPanel.h
    platform/PlatformPanel.cpp

    # 传感器
    sensor/SensorData.h
    sensor/SensorPanel.h
    sensor/SensorPanel.cpp
)

target_include_directories(DeckLab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(DeckLab PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    set_target_properties(DeckLab PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(DeckLab PRIVATE QT_MESSAGELOGCONTEXT)
    add_compile_options(/fsanitize=address)
    add_link_options(/fsanitize=address)
endif()
