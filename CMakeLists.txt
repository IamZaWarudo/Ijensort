cmake_minimum_required(VERSION 3.13)

#set(CMAKE_VERBOSE_MAKEFILE ON)

#project information
project(ijensort
        VERSION 0.1.2 
        DESCRIPTION "feed me"
        HOMEPAGE_URL "https://github.com/IamZaWarudo/Ijensort"
        LANGUAGES CXX)

#set(CMAKE_BUILD_TYPE Debug)

#find the root, set the various cmake variables.
find_package(ROOT REQUIRED
  COMPONENTS
  Core
  RIO
  Tree
  Hist
  Thread
  Spectrum
)

set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#setting output paths in the build directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_INSTALL_LIBDIR           ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(APPLE) 
  message("Building for Darwin")
  add_compile_definitions(__DARWIN__=TRUE)
elseif(LINUX) 
  message("Building for Linux")
  add_compile_definitions(__LINUX__=TRUE)
endif()


#This tells cmake to continue into these directories.
add_subdirectory(libraries)

#################
#################

function(add_ijensort_executable target source)
    add_executable(${target} ${source})

    target_include_directories(${target}  
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${ROOT_INCLUDE_DIRS}
    )

    target_link_libraries(${target} 
        PRIVATE
            GCORE
            GFORMAT
            GHISTOGRAMER
            GBCS
            ${ROOT_LIBRARIES}
    )

    string(ASCII 27 Esc)
    set(RED     "${Esc}[31m")
    set(GREEN   "${Esc}[32m")
    set(YELLOW  "${Esc}[33m")
    set(BLUE    "${Esc}[34m")
    set(MAGENTA "${Esc}[35m")
    set(CYAN    "${Esc}[36m")
    set(RESET   "${Esc}[0m")


    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN} How are you ijensort?, ${YELLOW} bing chilling!!!${RESET}"
        COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------"
    )

    endfunction()

    add_ijensort_executable(ijensort src/ijensort.cxx)
    add_ijensort_executable(printHits src/printHits.cxx)
    add_ijensort_executable(sort src/sort.cxx)