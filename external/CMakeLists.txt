

if(NOT TARGET assimp)
  option(BUILD_SHARED_LIBS "BUILD_SHARED_LIBS" OFF)

  set(ASSIMP_BUILD_SAMPLES OFF CACHE INTERNAL "")
  set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "")
  set(ASSIMP_BUILD_ZLIB ON CACHE INTERNAL "")

  # open fbx\ply\obj importer
  set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
  set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
  set(ASSIMP_BUILD_PLY_IMPORTER TRUE)

  # open fbx\ply\obj exporter
  set(ASSIMP_BUILD_FBX_EXPORTER TRUE)
  set(ASSIMP_BUILD_OBJ_EXPORTER TRUE)
  set(ASSIMP_BUILD_PLY_EXPORTER TRUE)

  add_subdirectory(assimp)
  include_directories(assimp/include/)

  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(assimp PRIVATE /WX-)
  else()
    target_compile_options(assimp PRIVATE -Wno-error)
  endif()

  set_target_properties(assimp PROPERTIES FOLDER ${ext_folder}/assimp)
endif()