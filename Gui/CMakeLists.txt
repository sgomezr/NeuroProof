CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (GUI)

IF(NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
message("${CMAKE_CURRENT_BINARY_DIR}")


set (SOURCES StackQTController.cpp StackQTUi.cpp)

if (QT_BUILT)
    QT4_WRAP_CPP(QTHEADERS StackQTUi.h StackQTController.h)
    QT4_WRAP_UI(QTFORMS neuroproof_stack_viewer.ui)
    add_library (Gui SHARED ${SOURCES} ${QTFORMS} ${QTHEADERS})
else()
    add_library (Gui SHARED ${SOURCES})
endif()


if (NOT BUILDEM_DIR STREQUAL "None")
    add_dependencies (Gui ${boost_NAME} ${vtk_NAME} RescanQt4)
endif()
