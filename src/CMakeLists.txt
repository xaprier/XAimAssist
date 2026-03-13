function(xaimassist_configure_module_library target_name)
  target_include_directories(${target_name}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_features(${target_name} PUBLIC cxx_std_17)

  install(
    TARGETS ${target_name}
    EXPORT XAimAssistTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endfunction()

add_subdirectory(core)
add_subdirectory(engine)
add_subdirectory(world)
add_subdirectory(gameplay)
add_subdirectory(input)
add_subdirectory(stats)
add_subdirectory(persistence)
add_subdirectory(ui)
add_subdirectory(app)