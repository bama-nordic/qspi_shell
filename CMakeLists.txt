#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qspi_shell)

target_compile_definitions_ifndef(
    SPI_IF
    app
    PRIVATE
    QSPI_IF
)

target_sources(app PRIVATE src/rpu_hw_if.c)
target_sources(app PRIVATE src/qspi_main.c)
target_sources_ifndef(SPI_IF app PRIVATE src/qspi_if.c)
target_sources_ifdef(SPI_IF app PRIVATE src/spi_if.c)
target_sources(app PRIVATE src/device.c)
