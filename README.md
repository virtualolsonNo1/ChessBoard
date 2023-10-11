# CHESS BOARD
- The purpose of this project was initially a rather simple one: my friends and I play A TON of chess over the board, and wanted to have a way to evaluate and learn from games afterwards without writing down our moves as we play.
- In order to do so, a board to store the chess moves and a way of viewing this data visually later (as well as a chess engine's evaluation of each position) was necessary 

# PERIPHERAlS USED
- In order to achieve this goal, hall effect sensors were used to capture data from magnetic pieces on the 64 squares. 
- From there, shift registers are used to store the data and output it over a single serial wire using SPI, allowing for this data to be sent over USB-C (also how the board is powered) 
    - Sadly, I forgot to include a 48Mhz crystal oscillator in my PCB design, so I'm in the process of adding one to my board without another board spin, after which I will be able to write a desktop app to take my buffer of hall effect sensor data and translate it to chess logic using python
- Additionally, in order to make it an all inclusive experience, a chess clock was added, using 3 gpio pins and 3 push-buttons (one for each player after they move their piece, and a third to change the time control, reset the clock, etc)
- Also, two general purpose timers as well as a 7-segment LCD display to show each player's time were used, of which their was already a library online to display on it, so pin-muxing for that SPI peripherals and converting gneeral purpose timer data to displayable data was almost all that had be done to get it working
- finally the idea came up that when a piece is picked up to light up its possible moves, which prompted me to add 64 LEDs and another set of shift registers, this time serial in parallel out as opposed to parallel in serial out
- Since at the time, I had been working with an stm32f411 microcontroller for my project at work, I used this same microcontroller for this project as to further learn
- Similarly, this was first implemented using FreeRTOS, as I was comparing FreeRTOS and Zephyr for another work project on an NXP dev board when it comes to speed, ease of day-to-day coding, implementation differences (preemptive prioritization and time slicing vs tickless, event driven RTOS defaulting to cooperative threading), etc., which was very easy to add using ST's CubeMX tool, but was later removed as it was unnecessary for this bare metal project

# MY CODE VS LIBRARIES AND GENERATED CODE
- STM32CubeMX was used for pin muxing and the initial setup of each peripheral, which is why any code between "//USER CODE END" and "//USER CODE BEGIN" is auto generated, and any code not put between the "//USER CODE BEGIN" and "//USER CODE END" comments will be removed if any updates are made on CubeMX and code is regenerated
- Additionally, max7219.c and max7219.h were borrowed from tabur on github, with the only changes needed being the SPI peripheral and GPIO pin in the .h file in order to communicate with the 7-segment LCD I was using
- Other than this, I wrote all of the code used in this code base

# REMAINING TODOs
- Because I forgot a 48Mhz crystall oscillator, I still need to order one, connect it properly to my PCB, and then output my hall data over USB-C
    - After this, I will need to write a python desktop app to process said data, but this should not be hard with python's numerous libraries, namely the python chess library, which I've used extensively before
    - Similarly, because speed is not a huge concern with something like this where human movement is the limiting factor, python will be plenty quick for this job despite being much, much slower than C
- Additionally, I was having some power issues with the LEDs, where only the first 4 rows operated as intended, with the latter half being half lit up but not fully lit up or off. I resoldered all of the shift registers with new ICs, hooked up a logic analyzer to see if the correct data was going through, etc., and even had my boss who is an Electrical Engineer have a go at it, and nothing came of it, leading to the conclusion that there's most likely a power issue somewhere. 
    - This very well could be the case with 64 LEDs all being powered over USB-C, but a current limit was imposed on each LED using the shift registers for them, so this shouldn't be an issue, so I will have to do a board spin with just one row later, trying to figure out a better way to route power to each LED