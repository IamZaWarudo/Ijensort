

#this will compile the current library.
add_library(GHISTOGRAMER SHARED 
  GHistogramer.cxx
)


target_include_directories(GHISTOGRAMER
  #PRIVATE
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${ROOT_INCLUDE_DIRS}
) 

root_generate_dictionary(G__GHistogramer 
  GHistogramer.h 
  MODULE GHISTOGRAMER 
  LINKDEF Linkdef.h
)

target_link_libraries(GHISTOGRAMER PUBLIC ${ROOT_LIBRARIES})

#link the compiled libraries above to the final executable.

