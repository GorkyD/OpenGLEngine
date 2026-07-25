if(NOT DEFINED SRC_DIR OR NOT DEFINED DST_DIR)
    message(FATAL_ERROR "CopyWwiseBanks.cmake requires -DSRC_DIR=... -DDST_DIR=...")
endif()

if(NOT EXISTS "${SRC_DIR}")
    message(STATUS "Wwise: GeneratedSoundBanks folder not found at ${SRC_DIR}, skipping bank sync")
    return()
endif()

file(GLOB _wwise_bank_files "${SRC_DIR}/*.bnk")
if(NOT _wwise_bank_files)
    message(STATUS "Wwise: no .bnk files found in ${SRC_DIR}")
    return()
endif()

file(MAKE_DIRECTORY "${DST_DIR}")
foreach(_bank_file ${_wwise_bank_files})
    file(COPY "${_bank_file}" DESTINATION "${DST_DIR}")
endforeach()

list(LENGTH _wwise_bank_files _wwise_bank_count)
message(STATUS "Wwise: synced ${_wwise_bank_count} SoundBank(s) from ${SRC_DIR} to ${DST_DIR}")
