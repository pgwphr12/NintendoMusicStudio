cmake_minimum_required(VERSION 3.24)
# Cog-pinned lazyusf2; cached interpreter, no executable JIT. See LICENSE-EVIDENCE.json.
add_library(lazyusf STATIC
 ai/ai_controller.c
 api/callbacks.c
 debugger/dbg_decoder.c
 main/main.c
 main/rom.c
 main/savestates.c
 main/util.c
 memory/memory.c
 pi/cart_rom.c
 pi/pi_controller.c
 r4300/cached_interp.c
 r4300/cp0.c
 r4300/cp1.c
 r4300/exception.c
 r4300/instr_counters.c
 r4300/interupt.c
 r4300/mi_controller.c
 r4300/pure_interp.c
 r4300/r4300.c
 r4300/r4300_core.c
 r4300/recomp.c
 r4300/reset.c
 r4300/tlb.c
 rdp/rdp_core.c
 ri/rdram.c
 ri/rdram_detection_hack.c
 ri/ri_controller.c
 rsp/rsp_core.c
 rsp_hle/alist.c
 rsp_hle/alist_audio.c
 rsp_hle/alist_naudio.c
 rsp_hle/alist_nead.c
 rsp_hle/audio.c
 rsp_hle/cicx105.c
 rsp_hle/hle.c
 rsp_hle/hvqm.c
 rsp_hle/jpeg.c
 rsp_hle/memory.c
 rsp_hle/mp3.c
 rsp_hle/musyx.c
 rsp_hle/plugin.c
 rsp_hle/re2.c
 rsp_lle/rsp.c
 si/cic.c
 si/game_controller.c
 si/n64_cic_nus_6105.c
 si/pif.c
 si/si_controller.c
 usf/barray.c
 usf/resampler.c
 usf/usf.c
 vi/vi_controller.c
 r4300/empty_dynarec.c
)
target_include_directories(lazyusf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/usf")
target_link_libraries(lazyusf PRIVATE zlibstatic)
target_include_directories(lazyusf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../zlib" "${CMAKE_BINARY_DIR}/third_party/zlib")
target_compile_definitions(lazyusf PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES NOMINMAX)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
 target_compile_definitions(lazyusf PRIVATE __x86_64__)
endif()
