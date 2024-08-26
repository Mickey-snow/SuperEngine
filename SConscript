# Main scons file

import shutil
import sys

Import('env')

########################################################## [ Root environment ]
root_env = env.Clone()
root_env.Append(
  CPPDEFINES = [
    "CASE_SENSITIVE_FILESYSTEM",
    "_THREAD_SAFE"
  ],

  CXXFLAGS = [
    "--ansi",
    "-Wall",
    "-Wno-sign-compare",
    "-Wno-narrowing",
    "-Wno-write-strings",    # For XPM support
    "-std=c++17"
  ]
)

# if root_env['PLATFORM'] == 'darwin':
#   root_env.Append(FRAMEWORKS=["OpenGL"])
# else:

root_env.ParseConfig("sdl-config --libs")

#########################################################################

# We used to set this just for an autogenerated header, but now it's wrapped
# its tentacles everywhere.
root_env.Append(
  CPPPATH = ["$BUILD_DIR"]
)

#########################################################################

librlvm_files = [
  "src/base/notification_details.cc",
  "src/base/notification_registrar.cc",
  "src/base/notification_service.cc",
  "src/base/notification_source.cc",
  "src/base/acodec/nwa.cc",
  "src/effects/blind_effect.cc",
  "src/effects/effect.cc",
  "src/effects/effect_factory.cc",
  "src/effects/fade_effect.cc",
  "src/effects/scroll_on_scroll_off.cc",
  "src/effects/wipe_effect.cc",
  "src/encodings/codepage.cc",
  "src/encodings/cp932.cc",
  "src/encodings/cp936.cc",
  "src/encodings/cp949.cc",
  "src/encodings/han2zen.cc",
  "src/encodings/western.cc",
  "src/libreallive/archive.cc",
  "src/libreallive/parser.cc",
  "src/libreallive/compression.cc",
  "src/libreallive/expression.cc",
  "src/libreallive/filemap.cc",
  "src/libreallive/gameexe.cc",
  "src/libreallive/intmemref.cc",
  "src/libreallive/scenario.cc",
  "src/libreallive/elements/bytecode.cc",
  "src/libreallive/elements/meta.cc",
  "src/libreallive/elements/comma.cc",
  "src/libreallive/elements/command.cc",
  "src/libreallive/elements/expression.cc",
  "src/libreallive/elements/textout.cc",
  "src/long_operations/button_object_select_long_operation.cc",
  "src/long_operations/load_game_long_operation.cc",
  "src/long_operations/pause_long_operation.cc",
  "src/long_operations/select_long_operation.cc",
  "src/long_operations/textout_long_operation.cc",
  "src/long_operations/wait_long_operation.cc",
  "src/long_operations/zoom_long_operation.cc",
  "src/machine/dump_scenario.cc",
  "src/machine/game_hacks.cc",
  "src/machine/general_operations.cc",
  "src/machine/long_operation.cc",
  "src/machine/mapped_rlmodule.cc",
  "src/machine/memory.cc",
  "src/machine/memory_intmem.cc",
  "src/machine/opcode_log.cc",
  "src/machine/reallive_dll.cc",
  "src/machine/reference.cc",
  "src/machine/rlmachine.cc",
  "src/machine/rlmodule.cc",
  "src/machine/rloperation.cc",
  "src/machine/rloperation/argc_t.cc",
  "src/machine/rloperation/complex_t.cc",
  "src/machine/rloperation/rlop_store.cc",
  "src/machine/save_game_header.cc",
  "src/machine/serialization_global.cc",
  "src/machine/serialization_local.cc",
  "src/machine/stack_frame.cc",
  "src/modules/module_bgm.cc",
  "src/modules/object_mutator_operations.cc",
  "src/modules/module_bgr.cc",
  "src/modules/module_dll.cc",
  "src/modules/module_debug.cc",
  "src/modules/module_event_loop.cc",
  "src/modules/module_g00.cc",
  "src/modules/module_gan.cc",
  "src/modules/module_grp.cc",
  "src/modules/module_jmp.cc",
  "src/modules/module_koe.cc",
  "src/modules/module_mem.cc",
  "src/modules/module_mov.cc",
  "src/modules/module_msg.cc",
  "src/modules/module_obj.cc",
  "src/modules/module_obj_creation.cc",
  "src/modules/module_obj_fg_bg.cc",
  "src/modules/module_obj_management.cc",
  "src/modules/module_obj_getters.cc",
  "src/modules/module_os.cc",
  "src/modules/module_pcm.cc",
  "src/modules/module_refresh.cc",
  "src/modules/module_scr.cc",
  "src/modules/module_se.cc",
  "src/modules/module_sel.cc",
  "src/modules/module_shk.cc",
  "src/modules/module_shl.cc",
  "src/modules/module_str.cc",
  "src/modules/module_sys.cc",
  "src/modules/module_sys_date.cc",
  "src/modules/module_sys_frame.cc",
  "src/modules/module_sys_name.cc",
  "src/modules/module_sys_save.cc",
  "src/modules/module_sys_syscom.cc",
  "src/modules/module_sys_timer.cc",
  "src/modules/module_sys_wait.cc",
  "src/modules/module_sys_index_series.cc",
  "src/modules/module_sys_timetable2.cc",
  "src/modules/modules.cc",
  "src/modules/object_module.cc",
  "src/systems/base/anm_graphics_object_data.cc",
  "src/systems/base/cgm_table.cc",
  "src/systems/base/colour.cc",
  "src/systems/base/colour_filter_object_data.cc",
  "src/systems/base/digits_graphics_object.cc",
  "src/systems/base/drift_graphics_object.cc",
  "src/systems/base/event_listener.cc",
  "src/systems/base/event_system.cc",
  "src/systems/base/frame_counter.cc",
  "src/systems/base/gan_graphics_object_data.cc",
  "src/systems/base/graphics_object.cc",
  "src/systems/base/graphics_object_data.cc",
  "src/systems/base/graphics_object_of_file.cc",
  "src/systems/base/graphics_stack_frame.cc",
  "src/systems/base/graphics_system.cc",
  "src/systems/base/graphics_text_object.cc",
  "src/systems/base/hik_renderer.cc",
  "src/systems/base/hik_script.cc",
  "src/systems/base/koepac_voice_archive.cc",
  "src/systems/base/little_busters_ef00dll.cc",
  "src/systems/base/little_busters_pt00dll.cc",
  "src/systems/base/mouse_cursor.cc",
  "src/systems/base/nwk_voice_archive.cc",
  "src/systems/base/object_mutator.cc",
  "src/systems/base/object_settings.cc",
  "src/systems/base/ovk_voice_archive.cc",
  "src/systems/base/ovk_voice_sample.cc",
  "src/systems/base/parent_graphics_object_data.cc",
  "src/systems/base/platform.cc",
  "src/systems/base/rltimer.cc",
  "src/systems/base/rlbabel_dll.cc",
  "src/systems/base/rect.cc",
  "src/systems/base/selection_element.cc",
  "src/systems/base/sound_system.cc",
  "src/systems/base/surface.cc",
  "src/systems/base/system.cc",
  "src/systems/base/system_error.cc",
  "src/systems/base/text_key_cursor.cc",
  "src/systems/base/text_page.cc",
  "src/systems/base/text_system.cc",
  "src/systems/base/text_waku.cc",
  "src/systems/base/text_waku_normal.cc",
  "src/systems/base/text_waku_type4.cc",
  "src/systems/base/text_window.cc",
  "src/systems/base/text_window_button.cc",
  "src/systems/base/tomoyo_after_dt00dll.cc",
  "src/systems/base/tone_curve.cc",
  "src/systems/base/voice_archive.cc",
  "src/systems/base/voice_cache.cc",
  "src/utilities/exception.cc",
  "src/utilities/file.cc",
  "src/utilities/graphics.cc",
  "src/utilities/string_utilities.cc",
  "src/utilities/date_util.cc",
  "src/utilities/find_font_file.cc",
  "src/utilities/math_util.cc",
  "vendor/xclannad/endian.cpp",
  "vendor/xclannad/file.cc",
  "vendor/xclannad/koedec_ogg.cc",
  "vendor/xclannad/nwatowav.cc",
  "vendor/xclannad/wavfile.cc"
]

root_env.StaticLibrary('rlvm', librlvm_files)

libsystemsdl_files = [
  "src/systems/sdl/sdl_audio_locker.cc",
  "src/systems/sdl/sdl_colour_filter.cc",
  "src/systems/sdl/sdl_event_system.cc",
  "src/systems/sdl/sdl_graphics_system.cc",
  "src/systems/sdl/sdl_music.cc",
  "src/systems/sdl/sdl_render_to_texture_surface.cc",
  "src/systems/sdl/sdl_sound_chunk.cc",
  "src/systems/sdl/sdl_sound_system.cc",
  "src/systems/sdl/sdl_surface.cc",
  "src/systems/sdl/sdl_system.cc",
  "src/systems/sdl/sdl_text_system.cc",
  "src/systems/sdl/sdl_text_window.cc",
  "src/systems/sdl/sdl_utils.cc",
  "src/systems/sdl/shaders.cc",
  "src/systems/sdl/texture.cc",

  # Parts of zresample
  "src/systems/sdl/resample.cc",
  "src/systems/sdl/audiofile.cc",
  "src/systems/sdl/dither.cc",
  "src/systems/sdl/zresample.cc",

  # Parts of pygame.
  "vendor/pygame/alphablit.cc"
]

root_env.StaticLibrary('system_sdl', libsystemsdl_files)

guichan_platform = [
  "src/platforms/gcn/gcn_button.cc",
  "src/platforms/gcn/gcn_graphics.cc",
  "src/platforms/gcn/gcn_info_window.cc",
  "src/platforms/gcn/gcn_menu.cc",
  "src/platforms/gcn/gcn_platform.cc",
  "src/platforms/gcn/gcn_save_load_window.cc",
  "src/platforms/gcn/gcn_scroll_area.cc",
  "src/platforms/gcn/gcn_true_type_font.cc",
  "src/platforms/gcn/gcn_utils.cc",
  "src/platforms/gcn/gcn_window.cc",
]

root_env.StaticLibrary('guichan_platform', guichan_platform)
