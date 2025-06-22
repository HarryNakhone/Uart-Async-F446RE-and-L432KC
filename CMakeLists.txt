cmake_minimum_required(VERSION 3.13.1)

# App

# Services
# list(APPEND ZEPHYR_EXTRA_MODULES 
#     ${CMAKE_CURRENT_LIST_DIR}/services/L298n_service
#     ${CMAKE_CURRENT_LIST_DIR}/services/HCSR04_service)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(servo_test)
set(BOARD_FLASH_RUNNER stm32cubeprogrammer)

target_sources(app PRIVATE applications/main_async_uart.c)

# target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/services/L298n_service)
# target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/services/HCSR04_service)