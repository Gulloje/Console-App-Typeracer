//CS 515 Final Project
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

#define MAX_USER_CHAR 50
#define MAX_FILE_ROW_LENGTH 70
#define true 1
#define false 0
#define MAX_PASSAGE_LENGTH 400


struct player {
    char username[MAX_USER_CHAR];
    int gamesPlayed;
    int scoreTotal;
};

struct player curPlayer;

/*
Helper function used to update the passed player's stats at the end of the game.

@param *player Pointer to the current player structure.
@param gamesPlayed The number of games to update the player struct to.
@param scoreTotal The current score Total of the player.
@param scoreToAdd The score to add to the player.
@author Jessie Gullo
*/
/*
Instance ID: 1-1
There is potential for integer overflow if a user were to play lots of games, gaining a total score of over the max integer value, then
causing wraparound to the min Int32 value, and setting it to that value in the csv file.
We resolved this issue by briefly promoting the scoreTotal and scoreToAdd and then checking if it is greater than INT32_Max.
If true, the user's scoreTotal is set to the max int value. 
*/
void updatePlayerStats(struct player *player, int gamesPlayed, int scoreTotal, int scoreToAdd) {
    if ((long)scoreTotal + (long)scoreToAdd > INT32_MAX) {
        player ->scoreTotal = INT32_MAX;
    } else {
        player -> scoreTotal = scoreTotal + scoreToAdd;
    }
    player -> gamesPlayed = gamesPlayed;
    
}
/*
Finds the passed player in users.csv and updates the player struct with the stats currently stored in users.csv.

@param player The player you are looking for in the users.csv file.
@author Jessie Gullo
*/
/*
Instance ID 5-2

Since we are dealing with file I/O, we need to verify the file is 1. found and 2. Make sure the file is actually read correctly. 
We solve the first issue by checking if the file pointer is null, suggesting it could not be opened, and returning from the function.
We solve the second issue by early returning when the data is correctly read. If the file data was tampered with or the user does not exist, than it will return
with a message saying the data could not be read. 
*/

void readPlayerData(struct player *player) {
    FILE *fp = fopen("users.csv", "r");
    if (fp == NULL) {
        printf("Error opening file.\n");
        return;
    }
    char line[MAX_FILE_ROW_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char* token = strtok(line, ",");
        if (strcmp(token, player->username) == 0) { //check if the username matches
            token = strtok(NULL, ",");
            int gamesPlayed = atoi(token);
            token = strtok(NULL, ",");
            int scoreTotal = atoi(token);
            updatePlayerStats(player, gamesPlayed, scoreTotal, 0);
            fclose(fp);
            return;
        }
    }
    printf("Problem Reading User Data");
    fclose(fp);

}


/*
Writes the data in player to the users.csv file.

@param player The player to search for and write the stats for.
@author Jesse Burlison
*/
/*
Race condition example
Instance ID: 2-1

There is potential for a race condition to happen here if there were multiple instances of the game being played at the same time. This would allow for one instance
to write to the users.csv file while another instance would also try to write to it. To solve for this issue we used flock from the fcntrl library to write lock the file while in use.
This ensures that another instance cannot write to the file while its already in use.

https://www.gnu.org/software/libc/manual/html_node/File-Locks.html
https://stackoverflow.com/questions/575328/fcntl-lockf-which-is-better-to-use-for-file-locking
https://www.man7.org/linux/man-pages/man0/fcntl.h.0p.html
*/
/*
  Format String Attack Example
  Instance ID: 4-1
  
  This method has the potential for a Format String Attack, to resolve this we made sure that fprintf is using the proper format specifier, i.e. %s and %d.
  Printf is also not using user provided input, so there are no issues.
  
  https://ccsu.blackboard.com/ultra/courses/_79262_1/cl/outline
*/


/*
Proper Error handling honorable mention. Requires a check to ensure the file is opened correctly.
*/
void writePlayerData(struct player *player) {
    FILE *fp = fopen("users.csv", "r+");
    if(fp == NULL){
        printf("Error opening file.\n");
        return;
    }

    //Lock the file to avoid race conditions
    if(flock(fileno(fp), LOCK_EX) != 0){
        printf("Error locking users file.\n");
        fclose(fp);
        return;
    }

    char line[MAX_FILE_ROW_LENGTH];
    long int pos = 0; //tracker for the current line
    int found = 0; //indicate if player is found

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *token = strtok(line, ",");
        if (strcmp(token, player->username) == 0) { //if the username is found
            found = 1; //player exists
            //rewind the cursor to the beginning of the line
            fseek(fp, pos, SEEK_SET);
            //overwrite the data on the correct line
            fprintf(fp, "%s,%d,%d", player->username, player->gamesPlayed, player->scoreTotal);
            break;
        }
        pos = ftell(fp); //update the position tracker
    }

    if (!found) {
        printf("Player not found.\n");
    }
    
    //Unlocks the file after writting
    flock(fileno(fp), LOCK_UN);
    //Close the file
    fclose(fp);
}

/*
Helper function to check if the username exists in users.csv

@param username The username to search users.csv for
@return 1 if username exists in the file, else 0.

@author Jessie Gullo
*/
/*
  Format String Attack Example
  Instance ID: 4-4
  
  This method has the potential for a Format String Attack, to resolve this we made sure that printf is using the proper format specifier, i.e. %s
  
  https://ccsu.blackboard.com/ultra/courses/_79262_1/cl/outline
*/

/*
Proper Error handling honorable mention. Requires a check to ensure the file is opened correctly.
*/
int checkUserExists(char *username) {
    FILE *fp;
    char row[MAX_USER_CHAR];
    char *token; //what to split by

    fp = fopen("users.csv", "r");
    if (fp == NULL) { //make sure file exists
        printf("Error: File not found.\n");
        return false; 
    }
    fgets(row, MAX_USER_CHAR, fp); //want to skip the first row since its just titles
    while (fgets(row, MAX_USER_CHAR, fp) != NULL) {
        token = strtok(row, ",");
        if (token != NULL) {
            if (strcmp(token, username) == 0) {
                //printf("%s", token);
                fclose(fp);
                return true;
            }
        }
    }
    fclose(fp);
    //printf("user was not found");
    return false; //username was not found
}

/*
Creates an account/record in users.csv with the passed username and initalize gamesPlayed and totalScore to 0.

@param username The username to create the record/account for.
@return 1 if successfully created the account, else 0.
@author Jessie Gullo
*/

/*
  Format String Attack Example
  Instance ID: 4-2
  
  This method has the potential for a Format String Attack, to resolve this we made sure that fprintf and printf are using the proper format specifier, i.e. %s
  
  https://ccsu.blackboard.com/ultra/courses/_79262_1/cl/outline
*/
/*
Proper Error handling honorable mention. Requires a check to ensure the file is opened correctly.
*/
int createAccount(char *username) {
    FILE *fp;
    fp = fopen("users.csv", "a");
    if (fp == NULL) {
        printf("Error: Unable to open file.\n");
        return false;
    }
    fprintf(fp, "\n%s,0,0", username);
    printf("Created Account with name: %s!\n", username);
    fclose(fp);
    return true;
    
}
/*
Helper function to display a countdown starting from 3.

@author Jesse Burlison
*/
void countdown() {
    int countdown = 3;
        while (countdown > 0) {
            mvprintw(0, (COLS - 2) / 2, "%d", countdown); //display number
            refresh();
            sleep(1); 
            refresh();
            countdown--;
            
        }
        mvprintw(0, (COLS - 2) / 2, "GO!!");
        move(0,0);
        flushinp(); //remove the characters that are gathered during the countdown
}

/*
Function which counts the number of words in a string.

@param passage The string to count the number of words in.
@return The number of words calculated in passage.
@author Jesse Burlison

*/
int countWords(char *passage) {
    int count = 0;
    int inWord = 0; //Flag to track if currently in a word
    
    for (int i = 0; passage[i] != '\0'; i++) {
        if (passage[i] == ' ' || passage[i] == '\n') {
            inWord = 0; 
        } else if (inWord == 0) {
            count++;
            inWord = 1; 
        }
    }
    return count;
}

/*
Counts the number of passages in the "ListOfTexts.txt" file. Then picks and returns a random passage.
*Note: Each passage is seperated by a new line character 

@return The randomly selected passage
@author Jessie Gullo
*/

/*
Instance ID: 3-2

There is a potential for buffer overflow on the line: "while (fgets(passage, MAX_PASSAGE_LENGTH, fp) != NULL) {". 
If a passage in the text file were to exceed the MAX_PASSAGE_LENGTH, you could potentially read the file past the buffer.
We solve this by using a constant of MAX_PASSAGE_LENGTH in combination with fgets() so that we do not read past the designated length

*/
/*
Instance ID 5-3
 
There are two instances of proper error handling in the following function. First, similar to 5-2, we have to ensure the file is properly opened and return if 
it could not be opened. Second, we have to verify the memory for the passage is properly allocated since the size of the passage is not known until runtime.
If there was insufficient memory, the function would return, indicating the problem. This is unlikely given the limited size of the max passage (MAX_PASSAGE_LENGTH),
but the check is important for providing roboust error handling. 
To solve the issue, we check if the passage is null and return if true. 
*/

char* getRandomPassage() {
    FILE *fp;
    fp = fopen("ListOfTexts.txt", "r");
    if (fp == NULL) {
        printf("Could not open file \n");
        return NULL;
    }
    
    int totalLines = 0;
    char c;
    while (c!= EOF) {
        c = fgetc(fp);
        if (c=='\n') {
            totalLines++;
        }
    }
    int randLine = (rand() % totalLines) + 1;
    rewind(fp); //because we already started the fp with getting to total lines, we have to set it back to the start of the file
    int curLine = 0;

    char *passage = (char*)malloc(MAX_PASSAGE_LENGTH * sizeof(char));
    if (passage == NULL) {
        printf("Memory allocation failed.\n");
        fclose(fp);
        return NULL; //problem when allocating, return
    }

    //read lines until the random line
    while (fgets(passage, MAX_PASSAGE_LENGTH, fp) != NULL) {
        curLine++;
        if (curLine == randLine) {
            //remove trailing newline character if present
            size_t length = strlen(passage);
            if (length > 0 && passage[length - 1] == '\n') {
                passage[length - 1] = '\0';
            }
            break;
        }
    }

    
    fclose(fp);
    return passage;
}
/*
The main game loop for a typeracer round. 

@return the number of words that were in the picked passage.
@author Jesse Burlison
*/
/*
  Format String Attack Example
  Instance ID: 4-3
  
  This method has the potential for a Format String Attack, to resolve this we made sure that printf is using the proper format specifier, i.e. %s, %c
  
  https://ccsu.blackboard.com/ultra/courses/_79262_1/cl/outline
*/
int playGame() {
    char* passageToDisplay = getRandomPassage();
    //char* passageToDisplay = "12345";
    clear();
    
    //Show the passage to the screen and start a countdown
    mvprintw(2, 0, "%s", passageToDisplay);
    refresh();
    countdown();
    

    //set our colors
    use_default_colors(); //https://stackoverflow.com/questions/32058813/c-obtain-neutral-background-with-ncurses
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    
    
    int errorExists = false;
    int counter = 0;
    int errorCounter = 0;
    
    move(2, 0 + counter);
    int y = 2;
    while (strlen(passageToDisplay)-1 != counter) {
        char inputChar = getch();
        //mvprintw(6,6, "%c", passageToDisplay[counter]);
        //mvprintw(6,6, "%c", inputChar);
        refresh();
        
        if ((counter + errorCounter) % COLS == 0) {
            if (counter > 1 && inputChar != 127 ){
                y++;
            }
        } 
        
        // If the user presses backspace
        if (inputChar == 127 ) {
           
            
            attroff(COLOR_PAIR(3));
            if(errorCounter > 0){
                errorCounter--;
            } else if(counter > 0){
                counter--;
            }
            
            mvprintw(y, ((counter + errorCounter) % COLS), "%c", passageToDisplay[counter + errorCounter ]);
            move(y, (counter + errorCounter) % COLS);
            if ((counter + errorCounter) % COLS == 0 && y != 2) {
                y--;
            }
            
            refresh();
            if(errorCounter == 0){
                errorExists = false;
            }
            
        } else if (inputChar == passageToDisplay[counter] && errorExists == false) {
            attron(COLOR_PAIR(1));
            mvprintw(y, (counter % COLS), "%c", passageToDisplay[counter]);
            counter++;
        } else {
            attron(COLOR_PAIR(2));
            
            if(counter+errorCounter < strlen(passageToDisplay)-1){
                mvprintw(y, ((counter + errorCounter) % COLS), "%c", passageToDisplay[counter + errorCounter ]);
                errorCounter++;
                errorExists = true;
            }
        }
    }
    //turn all colors off
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    attroff(COLOR_PAIR(3));
    int numWords = countWords(passageToDisplay);
    free(passageToDisplay); //free the memory that was used to display a passage
    return numWords;
        
}
/*
Entry point for the program. 

@author Jessie Gullo and Jesse Burlison
*/
int main(int argc, char *argv[]) {
    srand(time(NULL));
    char username[MAX_USER_CHAR];

    /*
    Instance ID 3-3
    
    Similar to 3-1, there is potential for buffer overflow if the command line parameter exceeds the buffersize.
    Using fgets with the sizeof(username), we can limit the number of characters that are stored in the username.
    The line "while ((c = getchar()) != '\n' && c != EOF); " is used to flush the remaining characters that did not fit into the buffer.

    */
    /*
    Instance ID 5-1

    Since the user is optionally able to enter command line parameters to sign into their account, we have to verify they entered the correct number of parameters.
    This is done with a straightforward "if" statement to verify the number of parameters. If their were not enough or too many parameters, the user is prompted
    to enter just a username. 
    

    */
    if (argc == 2) {
        strncpy(username, argv[1], MAX_USER_CHAR);
        username[MAX_USER_CHAR-1] = '\0'; //dont want the newline char on the username, just terminating char
    } else {
        printf("Please enter a username: \n");
        fgets(username, sizeof(username), stdin); //this will print user dne statement inputsize/maxchar times unless clear input buffer like below
        if (strlen(username) == sizeof(username) - 1 && username[sizeof(username) - 2] != '\n') {
            int c;
            while ((c = getchar()) != '\n' && c != EOF); 
        }
        username[strcspn(username, "\n")] = '\0';
      
    }
    /*
    Instance ID 3-1

    This while loop has the potential for buffer overflow while gathering input from the user. 
    This is resolved by having a a constant value for the size of response of MAX_USER_CHAR and using fgets with the sizeof(response).
    This effectively sets a cap on the length of the username and truncates it to MAX_USER_CHAR.
    BO is also possible when copying the response to the username. Similary, by using strncpy instead of 
    strcpy, we can cap the amount of characters to copy so that nothing is read past the buffer.
    The line "while ((c = getchar()) != '\n' && c != EOF); " is used to flush the remaining characters that did not fit into the buffer.

    */
    //make sure username exists, if not prompt to create a user while verifying it is not already taken
    int userExists = checkUserExists(username);
    while (userExists == false) {
        printf("User does not exist. Create (type 'create') a new user with username: \"%s\" or enter another username. \n", username);
        char response[MAX_USER_CHAR];
        fgets(response, sizeof(response), stdin); //this will print user dne statement inputsize/maxchar times unless clear input buffer like below
        if (strlen(response) == sizeof(response) - 1 && response[sizeof(response) - 2] != '\n') {
            int c;
            while ((c = getchar()) != '\n' && c != EOF); 
        }
        response[strcspn(response, "\n")] = '\0';
        if (strncmp(response, "create", strlen(response)) == 0) {
            userExists = createAccount(username);
        } else {
            strncpy(username, response, sizeof(response));
            userExists = checkUserExists(username);  
        }
    }
    struct player currentPlayer;
    strcpy(currentPlayer.username, username);
    readPlayerData(&currentPlayer);
    
    //now we want to play the game, set up curses terminal
    initscr();
    curs_set(0); //hides the cursor
    noecho(); //hides the user input from the screen
    cbreak();
    
    
    int continueGame = true;
    double wordsPerMin;
    while (continueGame) {
        move(0,0);
        printw("Welcome %s!  Press 1 to start a round. Press 2 to view stats. Press Q to quit. \n", currentPlayer.username);
        char input = getch();
        while (input != '1' && input != '2' && input != 'q') {
            input = getch();
            move(1, 0);
            clrtoeol();
            addch(input);
            refresh(); 
        }
        if (input == '1') {
            //start the timer
            clock_t startTime, endTime;
            double duration;
            startTime = time(NULL);
            double wordsInPassage = playGame();
            endTime = time(NULL);
            
            duration = difftime(endTime, startTime) -3; //the countdown is being counted in the clock, sub 3
            wordsPerMin = (wordsInPassage / duration) * 60;
            
            
            clear();
            //at the end of the game we need to add the data to the player stats
            updatePlayerStats(&currentPlayer, currentPlayer.gamesPlayed + 1, currentPlayer.scoreTotal, wordsPerMin);
            
        } else if (input =='2') {
            clear();
            int avgSpeed;
            if (currentPlayer.gamesPlayed != 0) {
                avgSpeed = (int) currentPlayer.scoreTotal / currentPlayer.gamesPlayed;
            } else {
                avgSpeed =0;
            }
            
            mvprintw(1,0,"You have %d games played with an average typing speed of %d WPM.", currentPlayer.gamesPlayed, avgSpeed);
        } else if (input == 'q') {
            continueGame = false;
        }
        refresh();
    }
    //update the users data at the end of the program
    writePlayerData(&currentPlayer);
    
    
    endwin();
    
    return 0;

}