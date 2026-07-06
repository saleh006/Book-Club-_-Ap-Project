cmake_minimum_required(VERSION 3.19)
project(ap-project LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network QuickWidgets Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network QuickWidgets Quick)


qt_standard_project_setup()
qt_add_resources(RESOURCES
    "res/resources.qrc"
)

qt_add_executable(Book-Club-_-Ap-Project
    WIN32 MACOSX_BUNDLE

    main.cpp



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
    ${RESOURCES}
)

qt_add_qml_module(Book-Club-_-Ap-Project
    URI "BookClubAuth"
    VERSION 1.0
    QML_FILES
        src/auth/firstPage.qml
        RESOURCES res/resources.qrc
        SOURCES
        SOURCES src/auth/loginwindow.h src/auth/loginwindow.cpp
        SOURCES src/auth/signupwindow.h src/auth/signupwindow.cpp
)

target_include_directories(Book-Club-_-Ap-Project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/auth
)

target_link_libraries(Book-Club-_-Ap-Project
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
        Qt::Network
        Qt::QuickWidgets
        Qt::Quick
)

install(TARGETS Book-Club-_-Ap-Project
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Book-Club-_-Ap-Project
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})