# CHESS BOARD
- The purpose of this project was initially a rather simple one: my friends and I play A TON of chess over the board, and wanted to have a way to evaluate and learn from games afterwards without writing down our moves as we play.
- In order to do so, a board to store the chess moves and a way of viewing this data visually later (as well as a chess engine's evaluation of each position) was necessary 

# PERIPHERALS USED
- In order to achieve this goal, hall effect sensors were used to capture data from magnetic pieces on the 64 squares. 
- From there, shift registers are used to store the data and output it over a single serial wire using SPI, allowing for this data to be sent over USB-C (also how the board is powered) 
    - Sadly, I forgot to include a 48Mhz crystal oscillator in my PCB design, so I'm in the process of adding one to my board without another board spin, after which I will be able to write a desktop app to take my buffer of hall effect sensor data and translate it to chess logic using python
- Additionally, in order to make it an all inclusive experience, a chess clock was added, using 3 gpio pins and 3 push-buttons (one for each player after they move their piece, and a third to change the time control, reset the clock, etc)
- Also, two general purpose timers as well as a 7-segment LCD display to show each player's time were used, of which their was already a library online to display on it, so pin-muxing for that SPI peripherals and converting gneeral purpose timer data to displayable data was almost all that had be done to get it working
- finally the idea came up that when a piece is picked up to light up its possible moves, which prompted me to add 64 LEDs and another set of shift registers, this time serial in parallel out as opposed to parallel in serial out
- Since at the time, I had been working with an stm32f411 microcontroller for my project at work, I used this same microcontroller for this project as to further learn
- Similarly, this was first implemented using FreeRTOS, as I was comparing FreeRTOS and Zephyr for another work project on an NXP dev board when it comes to speed, ease of day-to-day coding, implementation differences (preemptive prioritization and time slicing vs tickless, event driven RTOS defaulting to cooperative threading), etc., which was very easy to add using ST's CubeMX tool, but was later removed as it was unnecessary for this bare metal project
- switched from CDC to HID device with custom report descriptor
- 

# MY CODE VS LIBRARIES AND GENERATED CODE
- STM32CubeMX was used for pin muxing and the initial setup of each peripheral, which is why any code between "//USER CODE END" and "//USER CODE BEGIN" is auto generated, and any code not put between the "//USER CODE BEGIN" and "//USER CODE END" comments will be removed if any updates are made on CubeMX and code is regenerated
- Additionally, max7219.c and max7219.h were borrowed from tabur on github, with the only changes needed being the SPI peripheral and GPIO pin in the .h file in order to communicate with the 7-segment LCD I was using
- Other than this, I wrote all of the code used in this code base

# REMAINING TODOs
# THINGS TO TEST AT OFFICE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
- lights when other team piece picked up, etc. 

# OTHER SHIT!
- if pick up take piece that isn't allowed, blink error??????????????????????????????????????? SAME WITH PUT DOWN?????!!!!!!
- Add in extra functionality
    - FIX RESET BUG???????????????????????????!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        - i.e. if error on desktop app, weird firmware bug when reset??? Or whatever causes weird firmware bug that makes it so you have to restart chessbaord for it to work
    - no clock mode and debouncing shit
    - increment clock mode???????????
    - no light mode???????????????????
    - fix having to auto queen
    - can play white as either side dynamically depending on which side of clock hit first
    - second piece light functionality and put back down
- make it display chess.com analysis afterwards
- can play live games on lichess??????????!!!!!!!!!!!!!!!!!!!!!!!!!
- redo PCB and Design fancy shit???
    - add crystal, maybe fix LED shift registers to match hall effect ones, actual decoupling caps, better mounting holes


# USE CASE
- Once finished, the chess board will function as follows:
    - Current functionality: 
        - once plugged in, chess clock will turn on, displaying default time control of 1:00 for each player
        - if either of two outside push buttons are pressed, it will start the opponents timer, signifying that the first player made their move then hit the button
        - if the middle button is pressed before the game starts it changes the time control. If the game has already started, it will reset the game so the players can start another one whenever they want
        - during this time, the chess clock display will properly display the time control chosen or each player's time if the game has already started
        - The hall effect data is stored in a buffer as well as the respective LED data to be sent to the LEDs, which can currently be sent out but has some hardware issues with 4 of the 8 rows
    - Yet to be added functionality: 
        - after each change in the state of the board buffer of the hall data, this data will be sent over USB-C to a python app that will display the pieces place on the board and the engine evaluation of the position
        - When a piece is picked up, it's potential squares to be moved to will be lit up by the LEDs
