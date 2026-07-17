cmake_minimum_required(VERSION 3.19)
project(ap-project LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network QuickWidgets Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network QuickWidgets Quick)


qt_standard_project_setup()
qt_add_resources(RESOURCES
    "res/resources.qrc"
)

qt_add_executable(BookClubServer
    main_server.cpp
    src/database/databasemanager.h
    src/database/databasemanager.cpp
    src/database/databasemanager_core.cpp
    src/database/databasemanager_user.cpp
    src/database/databasemanager_book.cpp
    src/database/databasemanager_social.cpp
    src/database/databasemanager_library.cpp
    src/database/databasemanager_cart.cpp
    src/database/databasemanager_publisher.cpp
    src/server/servermanager.h
    src/server/servermanager.cpp
    src/server/serverui.h
    src/server/serverui.cpp
)

target_include_directories(BookClubServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
)

target_link_libraries(BookClubServer PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Sql
    Qt::Network
)

install(TARGETS BookClubServer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
)

qt_generate_deploy_app_script(
    TARGET BookClubServer
    OUTPUT_SCRIPT deploy_script_server
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script_server})

qt_add_executable(BookClubClient
    WIN32 MACOSX_BUNDLE
    main_client.cpp
    src/server/serverwindow.h
    src/server/serverwindow.cpp
    src/server/serverwindow.ui
    ${RESOURCES}
)

qt_add_qml_module(BookClubClient
    URI "BookClubAuth"
    VERSION 1.0
    QML_FILES
        src/auth/firstPage.qml
        RESOURCES res/resources.qrc res/gif.gif
    SOURCES
        SOURCES src/auth/loginwindow.h src/auth/loginwindow.cpp
        SOURCES src/auth/signupwindow.h src/auth/signupwindow.cpp
        SOURCES src/auth/recoverywindow.h src/auth/recoverywindow.cpp
)

target_include_directories(BookClubClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/auth
)

target_link_libraries(BookClubClient
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::QuickWidgets
        Qt::Quick
)

install(TARGETS BookClubClient
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
)
qt_generate_deploy_app_script(
    TARGET BookClubClient
    OUTPUT_SCRIPT deploy_script_client
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script_client})