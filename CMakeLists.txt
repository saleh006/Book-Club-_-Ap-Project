cmake_minimum_required(VERSION 3.19)
project(ap-project LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

qt_standard_project_setup()

qt_add_executable(Book-Club-_-Ap-Project
    WIN32 MACOSX_BUNDLE

    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    src/database/databasemanager.h
    src/database/databasemanager.cpp
    src/database/databasemanager_core.cpp
    src/database/databasemanager_user.cpp
    src/database/databasemanager_book.cpp
    src/database/databasemanager_social.cpp
    src/database/databasemanager_library.cpp
    src/database/databasemanager_cart.cpp
)

target_include_directories(Book-Club-_-Ap-Project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
)

target_link_libraries(Book-Club-_-Ap-Project
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
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