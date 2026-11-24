# NMS LGPL overlay refresh for existing as well as clean FetchContent builds.
set(path "${GME_SOURCE_DIR}/gme/Gb_Oscs.cpp")
set(local "${CMAKE_CURRENT_LIST_DIR}/gme-agb-patch/Gb_Oscs.cpp")
file(SHA256 "${path}" actual)
file(SHA256 "${local}" patched)
if(NOT actual STREQUAL patched AND NOT actual STREQUAL "7a1645716d8ca2bd7195d802e8beb638085c2c83bbef43a6a5ed62e2ed3e45aa" AND NOT actual STREQUAL "8ab988ca76abbdde2b0f9bfbd06a754fd3e4e09065da1a63ceb249f3ea3fcaa0")
 message(FATAL_ERROR "Unexpected libgme Gb_Oscs source for PCM patch")
endif()
configure_file("${local}" "${path}" COPYONLY)
