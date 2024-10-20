cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(freebot_demo)

include_directories(lib/freebot/inc)
target_sources(app PRIVATE src/main.c src/bt_lbs.c src/my_nus_stuff.c lib/freebot/src/fb_motor.c lib/freebot/src/fb_io.c lib/freebot/src/fb_pwr.c)
