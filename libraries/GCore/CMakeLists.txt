#this will compile the current library.
add_library(GCORE SHARED 
  globals.cxx
  ijen.cxx
)

target_include_directories(GCORE
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${ROOT_INCLUDE_DIRS}
) 

 
root_generate_dictionary(G__GCore 
  Ijen.h 
  # Gtypes.h 
  MODULE GCORE
  LINKDEF Linkdef.h
)

target_link_libraries(GCORE 
  PUBLIC
    # GPIPELINE
    ROOT::Core
    ROOT::Rint
    ROOT::RIO
    ROOT::Hist
)



#link the compiled libraries above to the final executable.