cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(badge_launcher)

target_sources(app PRIVATE 
    src/main.c
    src/snake_game.c
    src/button_test.c
    src/beaglegotchi.c
    src/dvd_app.c
    
    # Assets
    src/assets/dvd/dvd.c
    src/assets/shutdown/qr.c
    src/assets/ti_logo.c
    src/assets/badge/badge_andrei.c
    src/badge_mode_app.c
    src/i2c_scanner_app.c
    src/shutdown_app.c
    src/beagle_run.c
    src/space_invaders.c
    src/brick_breaker.c
    src/beagle_man.c
    src/froggr.c
    src/about_app.c
    src/serial_monitor.c
    src/timer_app.c
    src/chip_tunez.c
    src/assets/dvd/beagle.c
    src/assets/beaglegotchi/eating.c
    src/assets/beaglegotchi/hungry.c
    src/assets/beaglegotchi/sleeping.c
    src/assets/beaglegotchi/standard.c
    
    # Doom
    src/doom_app.c
    src/w_file_zephyr.c
    src/doom_wad.c
    src/doom_stubs.c
    
    # DoomGeneric Core (Excluding platform/stdc files)
    src/doomgeneric/doomgeneric/am_map.c
    src/doomgeneric/doomgeneric/d_event.c
    src/doomgeneric/doomgeneric/d_items.c
    src/doomgeneric/doomgeneric/d_iwad.c
    src/doomgeneric/doomgeneric/d_loop.c
    src/doomgeneric/doomgeneric/d_main.c
    src/doomgeneric/doomgeneric/d_mode.c
    src/doomgeneric/doomgeneric/d_net.c
    src/doomgeneric/doomgeneric/doomdef.c
    src/doomgeneric/doomgeneric/doomgeneric.c
    src/doomgeneric/doomgeneric/doomstat.c
    src/doomgeneric/doomgeneric/dstrings.c
    src/doomgeneric/doomgeneric/f_finale.c
    src/doomgeneric/doomgeneric/f_wipe.c
    src/doomgeneric/doomgeneric/g_game.c
    src/doomgeneric/doomgeneric/gusconf.c
    src/doomgeneric/doomgeneric/hu_lib.c
    src/doomgeneric/doomgeneric/hu_stuff.c
    src/doomgeneric/doomgeneric/i_cdmus.c
    src/doomgeneric/doomgeneric/i_endoom.c
    src/doomgeneric/doomgeneric/i_input.c
    src/doomgeneric/doomgeneric/i_joystick.c
    src/doomgeneric/doomgeneric/i_scale.c
    src/doomgeneric/doomgeneric/i_sound.c
    src/doomgeneric/doomgeneric/i_system.c
    src/doomgeneric/doomgeneric/i_timer.c
    src/doomgeneric/doomgeneric/i_video.c
    src/doomgeneric/doomgeneric/icon.c
    src/doomgeneric/doomgeneric/info.c
    src/doomgeneric/doomgeneric/m_argv.c
    src/doomgeneric/doomgeneric/m_bbox.c
    src/doomgeneric/doomgeneric/m_cheat.c
    src/doomgeneric/doomgeneric/m_config.c
    src/doomgeneric/doomgeneric/m_controls.c
    src/doomgeneric/doomgeneric/m_fixed.c
    src/doomgeneric/doomgeneric/m_menu.c
    src/doomgeneric/doomgeneric/m_misc.c
    src/doomgeneric/doomgeneric/m_random.c
    src/doomgeneric/doomgeneric/memio.c
    src/doomgeneric/doomgeneric/mus2mid.c
    src/doomgeneric/doomgeneric/p_ceilng.c
    src/doomgeneric/doomgeneric/p_doors.c
    src/doomgeneric/doomgeneric/p_enemy.c
    src/doomgeneric/doomgeneric/p_floor.c
    src/doomgeneric/doomgeneric/p_inter.c
    src/doomgeneric/doomgeneric/p_lights.c
    src/doomgeneric/doomgeneric/p_map.c
    src/doomgeneric/doomgeneric/p_maputl.c
    src/doomgeneric/doomgeneric/p_mobj.c
    src/doomgeneric/doomgeneric/p_plats.c
    src/doomgeneric/doomgeneric/p_pspr.c
    src/doomgeneric/doomgeneric/p_saveg.c
    src/doomgeneric/doomgeneric/p_setup.c
    src/doomgeneric/doomgeneric/p_sight.c
    src/doomgeneric/doomgeneric/p_spec.c
    src/doomgeneric/doomgeneric/p_switch.c
    src/doomgeneric/doomgeneric/p_telept.c
    src/doomgeneric/doomgeneric/p_tick.c
    src/doomgeneric/doomgeneric/p_user.c
    src/doomgeneric/doomgeneric/r_bsp.c
    src/doomgeneric/doomgeneric/r_data.c
    src/doomgeneric/doomgeneric/r_draw.c
    src/doomgeneric/doomgeneric/r_main.c
    src/doomgeneric/doomgeneric/r_plane.c
    src/doomgeneric/doomgeneric/r_segs.c
    src/doomgeneric/doomgeneric/r_sky.c
    src/doomgeneric/doomgeneric/r_things.c
    src/doomgeneric/doomgeneric/s_sound.c
    src/doomgeneric/doomgeneric/sha1.c
    src/doomgeneric/doomgeneric/sounds.c
    src/doomgeneric/doomgeneric/st_lib.c
    src/doomgeneric/doomgeneric/st_stuff.c
    src/doomgeneric/doomgeneric/statdump.c
    src/doomgeneric/doomgeneric/tables.c
    src/doomgeneric/doomgeneric/v_video.c
    src/doomgeneric/doomgeneric/w_checksum.c
    src/doomgeneric/doomgeneric/w_file.c
    src/doomgeneric/doomgeneric/w_main.c
    src/doomgeneric/doomgeneric/w_wad.c
    src/doomgeneric/doomgeneric/wi_stuff.c
    src/doomgeneric/doomgeneric/z_zone.c
)

target_include_directories(app PRIVATE 
    src/assets/beaglegotchi
    src/doomgeneric/doomgeneric
)

target_compile_definitions(app PRIVATE 
    DOOMGENERIC_RESX=320 
    DOOMGENERIC_RESY=200
)

# Force include string.h for strdup in doomgeneric files
target_compile_options(app PRIVATE -include string.h)
target_compile_definitions(app PRIVATE _GNU_SOURCE)

# Disable original I_ZoneBase in i_system.c so we can override it in doom_stubs.c
set_source_files_properties(src/doomgeneric/doomgeneric/i_system.c PROPERTIES COMPILE_DEFINITIONS "I_ZoneBase=I_ZoneBase_Disabled")
