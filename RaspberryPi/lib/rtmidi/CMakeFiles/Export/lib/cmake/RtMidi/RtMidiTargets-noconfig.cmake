#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "RtMidi::rtmidi" for configuration ""
set_property(TARGET RtMidi::rtmidi APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(RtMidi::rtmidi PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/librtmidi.so.4.0.0"
  IMPORTED_SONAME_NOCONFIG "librtmidi.so.4"
  )

list(APPEND _IMPORT_CHECK_TARGETS RtMidi::rtmidi )
list(APPEND _IMPORT_CHECK_FILES_FOR_RtMidi::rtmidi "${_IMPORT_PREFIX}/lib/librtmidi.so.4.0.0" )

# Import target "RtMidi::rtmidi_static" for configuration ""
set_property(TARGET RtMidi::rtmidi_static APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(RtMidi::rtmidi_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/librtmidi_static.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS RtMidi::rtmidi_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_RtMidi::rtmidi_static "${_IMPORT_PREFIX}/lib/librtmidi_static.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
