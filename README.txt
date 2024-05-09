CS 515 Course Project


This project is a console-based game that mimics popular games similar to Type Racer. When the user begins the game they will be prompted to enter their username, if the user name doesn't already exist, the game will prompt them to create that username by typing 'create' and hitting enter (Note: You can also enter the username as a command line parameter). Once logged in the user is prompted to either enter '2' to go to their stats view which displays average typing speed in Words Per Minute (WPM), or start a round by entering '1'. The user is also given the option to quit by pressing 'q'. Once a round is started, a random text exerpt will be displayed on the screen along with a 3 second count down, once the 3 second countdown is over it is the user's objective to type out the exerpt as fast as possible. Correctly typed characters will be highlted green and incorrectly typed characters will be highlighted red. The user must go back and fix any incorrectly typed characters before continuing by using backspace, else the rest of their typed characters will also be considered wrong. Once the exerpt is completely and correctly typed, the game loop brings the user back to the view score/play game selection, their speed that round is then logged, and will be considered when the user views their average speed.

To Compile and Run: 
    1. Make sure ListOfTexts.txt and users.csv are in the same level directory as typeracer_v1.c
    2. To compile: gcc -o typeracer typeracer_verified.c -lncurses
    3. Run the executable: ./typeracer OR ./typeracer username
