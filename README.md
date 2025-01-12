# CHESS BOARD
- The purpose of this project was initially a rather simple one: my friends and I play A TON of chess over the board, and wanted to have a way to evaluate and learn from games afterwards without writing down our moves as we play.
- In order to do so, a board to store the chess moves and a way of viewing this data visually later (as well as a chess engine's evaluation of each position) was necessary 

# PERIPHERALS USED
- In order to achieve this goal, hall effect sensors were used to capture data from magnetic pieces on the 64 squares. 
- From there, shift registers are used to store the data and output it over a single serial wire using SPI, allowing for this data to be sent over USB-C (also how the board is powered) 
    - Sadly, I forgot to include a crystal oscillator in my PCB design so usb can be at 48MHz, so I'm in the process of adding one to my board without another board spin, but for now a baudge on my first board design adding that does the trick
- Additionally, in order to make it an all inclusive experience, a chess clock was added, using 3 gpio pins and 3 push-buttons (one for each player after they move their piece, and a third to change the time control, reset the clock, etc)
    - another button will have to be added as to allow for being able to choose a piece to promote to
- Also, two general purpose timers as well as a 7-segment LCD display to show each player's time were used, of which their was already a library online to display on it, so pin-muxing for that SPI peripherals and converting gneeral purpose timer data to displayable data was almost all that had be done to get it working
- finally the idea came up that when a piece is picked up to light up its possible moves, which prompted me to add 64 LEDs and another set of shift registers, this time serial in parallel out as opposed to parallel in serial out
    - future board spin will use rgb LEDs that won't require extra shift registers but still work with SPI
- Since at the time, I had been working with an stm32f411 microcontroller for my project at work, I used this same microcontroller for this project as to further learn
- Similarly, this was first implemented using FreeRTOS, as I was comparing FreeRTOS and Zephyr for another work project on an NXP dev board when it comes to speed, ease of day-to-day coding, implementation differences (preemptive prioritization and time slicing vs tickless, event driven RTOS defaulting to cooperative threading), etc., which was very easy to add using ST's CubeMX tool, but was later removed as it was unnecessary for this bare metal project
    - I HATE st's version of freertos, it objectively sucks ass in every way imaginable, so I'll probably end up moving away from it eventually, but only if i have the motivation
- switched from CDC to custom HID device with custom report descriptor

# MY CODE VS LIBRARIES AND GENERATED CODE
- STM32CubeMX was used for pin muxing and the initial setup of each peripheral, which is why any code between "//USER CODE END" and "//USER CODE BEGIN" is auto generated, and any code not put between the "//USER CODE BEGIN" and "//USER CODE END" comments will be removed if any updates are made on CubeMX and code is regenerated
- Additionally, max7219.c and max7219.h were borrowed from tabur on github, with the only changes needed being the SPI peripheral and GPIO pin in the .h file in order to communicate with the 7-segment LCD I was using
    - Also had to look through datasheet to add in extra letters to spell "nocl" for no clock mode
- Other than this, I wrote all of the code used in this code base

# code overview:
- main(): 
    - initializes necessary peripherals (primarily spi for the lights and hall sensors, another spi for the clock, gpio pins for the clock button inputs, and usb to communicate with the desktop app), initializes the game struct, and inits the clock 7-segment LCD display with initTime()
- 4 main/important tasks:
    - updateMove(): 
        - continually loops, updating the hall effect sensor data and checking if a game is started or not. If it hasn't, makes sure pieces are on their starting squares with checkStartingSquares. If a game has started, it calls updateMoveShit(), which is where all of the chess logic resides. updateMoveShit and a lot of the other important functionality is in game.c, with the necessary structs and other variables/shared functions declared in game.h
        - updateMoveShit(): 
            - keeps track of state of the current move (if a piece has been picked up/moved, if another piece has been picked up (i.e. if a piece is taken, castling, etc), and based on the lights, can determine if a valid move is being played once the final state is reached (i.e. if a player hits their chess clock button or if in no clock mode, a valid move is played))
            - similarly, checkCastling() and checks for en passant are used within updateMoveShit
        - if the final state is reached (i.e. a move is trying to be played), some basic final checks are made, the active player is changed, and current move values are reset as to prepare for another move
            
    - updateTime()
        - calls updateTimeOld() at the top of main, which in turn, based on who the active player is, changes the display according to their remaining time (and for some god forsaken reason resets the game as well if reset button hit :( )
    - blinkError():
        - if an error occurrs, I don't want the chess board to shit itself, rather I want it to force users to put the piece back on its initial valid square and continue on with the game in a valid fashion. Similarly, if pieces are accidentally knocked over, it'd suck if the game ended there
        - that's where this function comes into play, which when an error occurrs, an error is sent to its queue, the updateMove() task is temporarily suspended till the error is resolved, and from there, this function checks the hall sensors, blinking where the error occurs until its resolved, and then resuming everything as normal
    - animateLights():
        - another glorified function wrapper as a task, also controlled by a semaphore. Because a blocking call on the main thread would halt any of my other stuff from running (i.e. checking if state of board has changed, etc), need a task to animate lights outward when piece first picked up, so all this does is move outward from the piece, checking if potential moves exist and after a delay, adding increasingly further moves to the things being lit up
            - also needed to not use lightState, as this is, at times, our "source of truth" of what is considered legal/possible chess wise, so don't want animation to change that, or if another piece is picked up or the piece is moved, etc, don't want to continue animating
        - FOR OTHER LIGHTS TURNING ON/OFF, it's almost always done by updateLights, which simply uses lightState to update the lights on the board, so you'll see this everywhere and it's even (probably badly :() called by lightsOff

- interrupts
    - used for timers, gpio inputs for buttons, and USB!!!!!
        - for timers, used for both players' clocks as well as for the no clock mode, where instead of hitting button to signify move, timer used to check if piece on valid move square for a full second, after which the "finalState" bool will be true
    - usb hid report descriptor can be found at usbd_custom_hid_if.c
        - is probably best documented part of all this shit lol
    - that same file holds the interrupt in CUSTOM_HID_OutEvent_FS(), which checks the kind of report, and from there acts accordingly depending on if it's light data, piece data for the state of the board in characters, an error, etc


- other important variables/functions to know
    - the overall structure is the game struct has the players, who is white, if the game has started, and the state of the board, as well as a move struct. 
    - a move contains if the lights are on, if light data has been received, the pickup state, which player is white, and the row/column info associated with said move
        - game.h also has the time control info, clock info, player info, etc all outlined
    - for USB, there's a lightReport struct that is a union containing the row and column values of piece(s) picked up as well as the reportID, which is important/necessary for usb hid stuff. all this is in usb.h
    - game.currentMove->lightState is current state of the board in 8x8 fashion, with a 1 being light on and 0 off
        - allPieceLights contains not just current lights, but those for all of piece, so don't have to query desktop app again when piece is moved over valid potential squares. i.e. it is, for a large part, our "source of truth" for what is valid
    - game.currentMove->piecePickupState is an enum that's checked to determine if one piece, two pieces (for takes or castling), or no pieces have been picked up yet
    - final state is when player hits their button to signify playing a move, or if in no clock mode, a valid move is simply played for a second. Couldn't be in enum as need to be able to check if it is both the final state or not as well as the pickup state, hence a severe (and almost criminal) lack of switch statements
    - helper functions such as rotate8x8Array(), convert2DArrayToBitarray(), resetNow(), changeTimeControl(), etc., are fairly self explanatory and do what they imply (transform arrays, reset the game, change the time control before the game, etc) 
        - in particular, rotate8x8Array() is a recent addition allowing either side to play as white so one can exchange pieces when re-setting up the board after a game as opposed to having to physically rotate the whole board, so it's applied to the hall data as well as light data to make the data being input uniform so none of the chess-related logic has to change
        
- One last SUPER important note:
    - a lot of the chess related logic regarding possible moves (especially castling and en passant) for the pieces (either possible moves for the active player's pieces or the active player's pieces that can take the opponent's piece to light up), keeping track of the game as a whole (i.e. converting row/column data into smith notation moves, displaying the game on chess.com afterwards for analysis, etc) is done on the desktop app repo: https://github.com/virtualolsonNo1/ChessDesktopApp
        - TODO: THESE NEED TO BE COMBINED INTO MONOREPO LATER, 2nd one maybe for PCB files


# REMAINING TODOs
- Add in extra functionality
    - debouncing shit
    - increment clock mode???????????
    - no light mode???????????????????
    - fix having to auto queen
    - can play white as either side dynamically depending on which side of clock hit first
- can play live games on lichess??????????!!!!!!!!!!!!!!!!!!!!!!!!!
    - robot arm to move pieces
    - Integrate with lichess on desktop app end
    - Fancy ass algorithm to put pieces back on starting squares with least robot arm movement possible
- redo PCB and Design fancy shit
    - add crystal, two extra columns on each side for taken pieces, RGB LEDs, actual decoupling caps, better mounting holes


# USE CASE
- Once finished, the chess board will function as follows:
    - Current functionality: 
        - once plugged in, chess clock will turn on, displaying default time control of 1:00 for each player 
        - if either of two outside push buttons are pressed, it will start the opponents timer, signifying which side is white and that the first player must make their move then hit their button
        - if the middle button is pressed before the game starts it changes the time control. If the game has already started, it will reset the game so the players can start another one whenever they want
        - during this time, the chess clock display will properly display the time control chosen or each player's time if the game has already started
        - Once game has started:
            - first piece pickup:
                - once a game has started, if a player's piece is picked up, all it's possible moves will light up, animating outward from the piece's current spot
                - if a first piece that can be taken is picked up first, all pieces for the active player that can take it will have their squares light up
                - one thing to note is that for castling, the square next to and two from the king will be lit up. If the king is moved to the square 2 away from it, in no clock mode a move will not be played till the rook is moved to its (lit up) final square, and in clock mode it will force you to put the king back and replay the move until you move both the king and rook to their respective casting squares properly
                    - if the rook is moved to its "castling" square before picking up the king, in no clock mode it will play it as a rook move, so be careful in this scenario. In clock mode this isn't an issue, as as long as the clock button isn't hit till the full castling has been played (both king and rook moved), there won't be a problem
            - second piece pickup:
                - if a valid second pieced is picked up, only that square and the first (either taking or taken) piece's squares will be lit up
                    - exceptions to this are en passant and castling. For en passant, only the starting and ending square for the piece that's doing the taking will be lit up. For castling, only the ending squares for the king and rook will be lit up
        - if any piece is picked up that isn't allowed, pieces are accidentally knocked over, etc., board will enter an error state where the squares that the pieces need to be put back on to resume the normal game will blink on and off every half second until they're put back, after which the game will resume as before
        - similarly, if pieces are randomly added back to the board and a move is attempted to be played, the board will force them to be taken back off to resume the current move properly once again
        - In no clock mode, the side where "nocl" is displayed on the 7-segment LCD is whose move it is, and will change whenever a valid move is played for 1 second, after which it becomes the other player's turn and nocl is displayed for that other person
        - after the game, once reset hit, chess.com analysis board will pop up on default browser showing the full game
        
    - Yet to be added functionality: 
