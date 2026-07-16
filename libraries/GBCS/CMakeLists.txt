add_library(GBCS SHARED
  GBCS.cxx
  GPIN.cxx
  GDSSD.cxx
  # TOFCorrector.cxx
  GSSSD.cxx
)

target_include_directories(GBCS
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${ROOT_INCLUDE_DIRS}
)

root_generate_dictionary(G__GBCS
  GBCS.h
  GPIN.h
  GDSSD.h
  # TOFCorrector.h
  GSSSD.h
  MODULE GBCS
  LINKDEF Linkdef.h
)

target_link_libraries(GBCS
  PUBLIC
    GFORMAT
    ROOT::Core
    ROOT::RIO
    ROOT::MathCore
)
