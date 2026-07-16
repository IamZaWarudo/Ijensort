
#this will compile the current library.
add_library(GFORMAT SHARED 
  #GChannel.cxx
  dataBlock.cxx 
  evtFile.cxx  
  #ddasHit.cxx
)

target_include_directories(GFORMAT
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${ROOT_INCLUDE_DIRS}
) 

root_generate_dictionary(G__GFormat 
  #GChannel.h 
  dataBlock.h 
  evtFile.h
  #ddasHit.h
  MODULE GFORMAT
  LINKDEF Linkdef.h
)

target_link_libraries(GFORMAT 
  PUBLIC
    ROOT::Core
    # ROOT::MathCore
)
