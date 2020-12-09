if(WIN32)
  get_filename_component(
    WINDOWS_10_KITS_ROOT
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
    ABSOLUTE CACHE
  )
  set(WINDOWS_10_KIT_DIR "${WINDOWS_10_KITS_ROOT}/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" CACHE PATH "Current Windows 10 kit directory")
  set(SIGNTOOL_KEY_FILE "" CACHE PATH "Path to signing key file in .pfx format")
  find_program(
    SIGNTOOL_EXE
    signtool
    PATHS
    "${WINDOWS_10_KIT_DIR}/x64"
    "${WINDOWS_10_KIT_DIR}/x86"
    DOC "Path to signtool.exe if SIGNTOOL_KEY_FILE is set"
  )
endif()
function(sign_target TARGET)
  if(SIGNTOOL_KEY_FILE AND WIN32 AND EXISTS "${SIGNTOOL_KEY_FILE}")
    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND
      "${SIGNTOOL_EXE}"
      ARGS
      sign
      /f "${SIGNTOOL_KEY_FILE}"
      /t http://timestamp.digicert.com
      /fd SHA256
      "$<TARGET_FILE:${TARGET}>"
    )
  endif()
endfunction()
