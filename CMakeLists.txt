#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("ESIREM_MAIN_FW_nRF52833")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/settings.c
  src/ble_service_config.c
  src/ble_service_user.c
  src/ble.c
  src/core.c
)

# NORDIC SDK APP END
zephyr_include_directories(${CMAKE_SOURCE_DIR})
zephyr_library_include_directories(.)
