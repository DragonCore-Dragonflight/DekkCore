# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES)

add_library(extractor_common STATIC ${PRIVATE_SOURCES})

target_link_libraries(extractor_common
  PRIVATE
    trinity-core-interface
  PUBLIC
    casc
    common)

target_include_directories(extractor_common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(extractor_common
    PROPERTIES
      FOLDER
        "tools")

if (WIN32)
  if ("${CMAKE_MAKE_PROGRAM}" MATCHES "MSBuild")
    add_custom_command(TARGET extractor_common
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tact.keys ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
  elseif (MINGW)
    add_custom_command(TARGET extractor_common
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tact.keys ${CMAKE_BINARY_DIR}/bin/
    )
  endif()
endif()

if (UNIX)
  install(FILES tact.keys DESTINATION bin)
elseif (WIN32)
  install(FILES tact.keys DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
