# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

#FILE(GLOB app_sources DVD-Screensaver/*.c)
#FILE(GLOB app_sources Beaglegotchi/*.c)
FILE(GLOB app_sources Button-Test/*.c)
target_sources(app PRIVATE ${app_sources})
