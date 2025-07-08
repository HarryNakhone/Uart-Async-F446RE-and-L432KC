cmake_minimum_required(VERSION 3.13.1)

# list(APPEND ZEPHYR_EXTRA_MODULES 


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(servo_test)
set(BOARD_FLASH_RUNNER stm32cubeprogrammer)

target_sources(app PRIVATE applications/uart_sender_nucle.c) #### Adjust the main file based on your preference. Here is the sender

