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
    src/assets/dvd/beagle.c
    src/assets/beaglegotchi/eating.c
    src/assets/beaglegotchi/hungry.c
    src/assets/beaglegotchi/sleeping.c
    src/assets/beaglegotchi/standard.c
)

target_include_directories(app PRIVATE src/assets/beaglegotchi)
