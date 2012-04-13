
project(choqok-plurk)
cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)
find_package(Choqok REQUIRED)
find_package(QJson REQUIRED)
find_package(QCA2 REQUIRED)
find_package(QtOAuth REQUIRED)

include(KDE4Defaults)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${KDE4_INCLUDES})
include_directories(${CHOQOK_INCLUDE_DIR} ${CHOQOK_INCLUDE_DIR}/choqok ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${QCA2_INCLUDE_DIR} ${QTOAUTH_INCLUDE_DIR})
include_directories(
${CHOQOK_INCLUDES}
${CMAKE_CURRENT_BINARY_DIR}
)

set( choqok_plurk_SRCS 
    ${choqok_plurk_UIS}
    plurkapioauth.cpp
    plurkaccount.cpp
    plurkmicroblog.cpp
    plurkpostwidget.cpp
    plurkeditaccount.cpp
    plurktimelinewidget.cpp
)

kde4_add_ui_files(choqok_plurk_UIS
plurkeditaccount_base.ui
)

kde4_add_plugin(choqok_plurk ${choqok_plurk_SRCS})

target_link_libraries(choqok_plurk
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QJSON_LIBRARY}
    ${QTOAUTH_LIBRARY}
    ${CHOQOK_LIBRARIES}
)

# Install

install(TARGETS choqok_plurk DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES choqok_plurk.desktop  DESTINATION ${SERVICES_INSTALL_DIR})

add_subdirectory(icons)
