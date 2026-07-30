cmake_minimum_required(VERSION 3.21)

project(DNA_SearchEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CORE_SOURCES
    core/DnaLimits.h
    core/Genome.cpp
    core/Genome.h
    core/PositionList.h
    core/ReadGenerator.cpp
    core/ReadGenerator.h
    core/SearchEngine.cpp
    core/SearchEngine.h
    core/SearchResult.cpp
    core/SearchResult.h
    core/ZipperHashTable.cpp
    core/ZipperHashTable.h
)

add_library(DnaCore STATIC ${CORE_SOURCES})
target_include_directories(DnaCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(DnaCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(DnaCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(DNA_SearchEngine
    main.cpp
    ui/AlignmentDetailWidget.cpp
    ui/AlignmentDetailWidget.h
    ui/AlignmentViewData.h
    ui/GenomeAlignmentWidget.cpp
    ui/GenomeAlignmentWidget.h
    ui/GenomeOverviewWidget.cpp
    ui/GenomeOverviewWidget.h
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/ResultPanel.cpp
    ui/ResultPanel.h
    ui/SearchSessionModel.cpp
    ui/SearchSessionModel.h
    ui/SequenceFileIO.cpp
    ui/SequenceFileIO.h
    ui/SnackbarHost.cpp
    ui/SnackbarHost.h
)

target_link_libraries(DNA_SearchEngine PRIVATE DnaCore Qt6::Widgets)
set_target_properties(DNA_SearchEngine PROPERTIES WIN32_EXECUTABLE TRUE)

if(MSVC)
    target_compile_options(DNA_SearchEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(DNA_SearchEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS DNA_SearchEngine RUNTIME DESTINATION .)
