# Zephyr Async UART Api on STM32F4 Disco 

Here is how you can enable the async api provided zephyr for your boards

## Required Tools 

2 Microcontrollers (I used STM32F4 Discovery)

Jumper wires

UART-USB adapter or debugger (unless your board comes with a built-in serial port — mine doesn’t)

### Important notes:
This took me a while to figure out, but make sure not to use UART pins that are already occupied by other peripherals like SCL, CLK, etc.
The documentation/manual of your board is your best friend. Sometimes LLMs alone can’t figure out the peripheral mappings.
Also, reaching out to the Zephyr community on Discord can really help.


### Prerequisites
- List (Zephyr, Python, VsCode etc...). Visit the website for more info

### DMA Configuration
Make sure your prj.conf has all the required Kconfig options.
Check your board’s DMA mapping, because it may be different from mine — unless you're using the same STM32F series.
(Still, check the manual!!)


### Installation
```bash
# Example of running commands

#build 
west build -p always -b stm32f4_disco  # Or your boards.

#flash 

west flash
