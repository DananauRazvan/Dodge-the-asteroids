#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <string.h>
#include "LedControl.h"

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;

const int xPin = A0;
const int yPin = A1;
const int joystickButtonPin = 7;
const int highAxis = 800;
const int lowAxis = 200;
bool xAxisReset;
bool yAxisReset;
bool lastButtonVal;

const int RS = 9;
const int enable = 8;
const int d4 = 5;
const int d5 = 4;
const int d6 = 3;
const int d7 = 2;

const int buzzer = 13;

bool highscoreNavStart;

unsigned long int lastDisplayMillis = 0;
int displayDelay = 200;
unsigned long int startingTime = 0;
int menuSelected = 1;
int menuWasClicked = 0;

bool menuJustStarted = 1;
bool gameJustStarted = 1;
bool settingsJustStarted = 1;
bool highscoreJustStarted = 1;

bool menuChanged;
bool settingsChanged;
bool highscoreNavChanged;
bool alphabetState = 1;
bool changingName;
bool infoChanged = 1;

int settingSelected = 1;
int settingsDisplayStart = 1; 
int infoDisplayStart = 1; 

const int nameSize = 4;
char currentPlayer[nameSize + 1] = "ABCD";
int selectedChar = 65;
int charToReplace = 0;

unsigned long int lastMillisForAlphabet;
const int millisForAlphabet = 500;
bool characterShow = 1;

int gameOverScreenNo;
bool gameOverScreenChanged;
bool gameOverSelection;

int instructionOpt = 1;
bool viewInstructionTo;
bool instructionState;

int maxBrightnessValue = 15;
int matrixBrightnessValue = 1;

bool gameOver;
int baseLevel;
int currentScore;
int currentLives;

const int maxLevel = 25;
int startingLevelValue = 1;

const int startingLives = 3;

struct player {
  int pScore = 0;
  char pName[nameSize + 1];
};

LiquidCrystal lcd(RS, enable, d4, d5, d6, d7);

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);

void clearDisk(){
  for (int i = 0 ; i < EEPROM.length() ; i++){
    EEPROM.write(i, 0);
  }
}

struct player getPlayer(byte pos){
  player p;
  EEPROM_readAnything((pos - 1) * 10, p);
  
  return p;
}

void writePlayer(byte pos, player p){
  EEPROM_writeAnything((pos - 1) * 10, p);
}

player player1, player2, player3;
/// Joystick
int readJoyStickButton(){
  bool buttonVal = digitalRead(joystickButtonPin);
  
  if (buttonVal == 0 && lastButtonVal == 1){
    lastButtonVal = 0;
    return 1;
  }
  
  if (buttonVal == 1 && lastButtonVal == 0){
    lastButtonVal = 1;
  }
  
  return 0;
}

int readJoyStickX(){
  int xAxis = analogRead(xPin);
  
  if (xAxis > highAxis && xAxisReset){
    xAxisReset = 0;
    return 1;
  }
  else 
    if (xAxis < lowAxis && xAxisReset){
      xAxisReset = 0;
      return -1;
    }
    
  if (xAxis <= highAxis - 200 && xAxis >= lowAxis + 200){
    xAxisReset = 1;
  }
  
  return 0;
}

int readJoyStickY(){
  int yAxis = analogRead(yPin);
  
  if (yAxis > highAxis && yAxisReset){
    yAxisReset = 0;
    return -1;
  }
  else 
    if (yAxis < lowAxis && yAxisReset){
      yAxisReset = 0;
      return 1;
    }
  
  if (yAxis <= highAxis - 200 && yAxis >= lowAxis + 200){
    yAxisReset = 1;
  }
  
  return 0;
}
// Display top 3 highscores
void displayHighScore(){
  if (highscoreNavChanged){
    if (highscoreNavStart == 0){
      lcd.clear();
      
      lcd.print("1. ");
      lcd.print(player1.pName);
      lcd.print("  ");
      lcd.print(player1.pScore);
      
      lcd.setCursor(0, 1);
      lcd.print("2. ");
      lcd.print(player2.pName);
      lcd.print("  ");
      lcd.print(player2.pScore);
      
      lcd.setCursor(15, 1);
      lcd.print("v");
    }
    else{
      lcd.clear();
      
      lcd.print("3. ");
      lcd.print(player3.pName);
      lcd.print("  ");
      lcd.print(player3.pScore);
      
      lcd.setCursor(15, 0);
      lcd.print("^"); 
    }
    
    highscoreNavChanged = 0;
  }
}
/// Navigate highscore, entry and exit
void navigateHighScores(){
  if (highscoreJustStarted){
    highscoreNavChanged = 1;
    highscoreJustStarted = 0;
  }
  
  displayHighScore();
  
  if (readJoyStickY()){
    highscoreNavStart = !highscoreNavStart;
    highscoreNavChanged = 1;
  }
  
  if (readJoyStickButton()){
    menuWasClicked = 0;
    highscoreJustStarted = 1;
    highscoreNavStart = 0;
  }
}
/// Display how to play
bool instructions(){
  if (instructionState){
    lcd.clear();
    
    if (instructionOpt == 1){
      lcd.print("You should avoid");
      
      lcd.setCursor(0, 1);
      lcd.print("the asteroids");
      lcd.setCursor(15, 1);
      lcd.print("v");
    }
    else 
      if (instructionOpt == 2){
        lcd.print("using your");
        lcd.setCursor(15, 0);
        lcd.print("^");
        
        lcd.setCursor(0, 1);
        lcd.print("joystick. You");
        lcd.setCursor(15, 1);
        lcd.print("v");
      }
      else 
        if (instructionOpt == 3){
          lcd.print("lose a life");
          lcd.setCursor(15, 0);
          lcd.print("^");
          
          lcd.setCursor(0, 1);
          lcd.print("when an");
          lcd.setCursor(15, 1);
          lcd.print("v");
        }
        else 
          if (instructionOpt == 4){
            lcd.print("asteroid hits");
            lcd.setCursor(15, 0);
            lcd.print("^");
            
            lcd.setCursor(0, 1);
            lcd.print("you. Good luck");
            lcd.setCursor(15, 1);
            lcd.print("v");
          }
          else 
            if (instructionOpt == 5){
              lcd.print("and have fun!");
              lcd.setCursor(15, 0);
              lcd.print("^");
            }
    
    instructionState = 0;
  }
  
  int yAxis = readJoyStickY();
  
  if (yAxis == -1){
    instructionOpt++;
    
    if (instructionOpt > 5){
      instructionOpt = 1;
    }
    
    instructionState = 1;
  }
  else 
    if (yAxis == 1){
      instructionOpt--;
      
      if (instructionOpt < 1){
        instructionOpt = 5;
      }
      
      instructionState = 1;
    }
    
  if (readJoyStickButton()){
    return 0;
  }
  else{
    return 1;
  }
}
/// Display alphabet
void displayAlphabet(){
  if (alphabetState){
    lcd.clear();
    
    for (int i = 65; i < 81; i++){
      if (i != selectedChar){
        lcd.print(char(i));
      }
      else{
        if (characterShow){
          lcd.print(char(i));
        }
        else{
          lcd.print(' ');
        }
      }
    }
    lcd.setCursor(0, 1);
    for (int i = 81; i < 91; i++){
      if (i != selectedChar){
        lcd.print(char(i));
      }
      else{
        if (characterShow){
          lcd.print(char(i));
        }
        else{
          lcd.print(' ');
        }
      }
    }
    
    alphabetState = 0;
  }
}
/// Navigate alphabet and select char
char selectFromAlphabet(){
  if (millis() - lastMillisForAlphabet >= millisForAlphabet){
    characterShow = !characterShow;
    alphabetState = 1;
    lastMillisForAlphabet = millis();
  }
  
  displayAlphabet();
  
  int xAxis = readJoyStickX();
  
  if (xAxis == 1){
    selectedChar++;
    
    if (selectedChar > 90){
      selectedChar = 65;
    }
    
    alphabetState = 1;
  }
  else 
    if (xAxis == -1){
      selectedChar--;
      
      if (selectedChar < 65){
        selectedChar = 90;
      }
      
      alphabetState = 1;
    }

  if (readJoyStickButton()){
    int aux = selectedChar;
    selectedChar = 65;
    return char(aux);
  }
  
  return '.';
}
/// Display all settings
void displaySettings(){
  if (settingSelected == 2){
    if (millis() - lastMillisForAlphabet >= millisForAlphabet){
      characterShow = !characterShow;
      settingsChanged = 1;
      lastMillisForAlphabet = millis();
    }
  }
  if (settingsChanged > 0){  
    if (settingsDisplayStart == 1){
      lcd.clear();
      
      if (settingSelected == 1){
        lcd.print(">");
      }
      else{
        lcd.print(" ");
      }
      
      lcd.print("Level:  ");
      lcd.print(startingLevelValue);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      
      if (settingSelected == 2){
        lcd.print(">Name: ");
        
        for (int i = 0; i < nameSize; i++){
          if (i != charToReplace){
            lcd.print(currentPlayer[i]);
          }
          else{
            if (characterShow){
              if (currentPlayer[i] == ' '){
                lcd.print('_');
              }
              else{
                lcd.print(currentPlayer[i]);
              }
            }
            else{
              lcd.print(' ');
            }
          }
        }
      }
      else{
        lcd.print(" Name: ");
        lcd.print(currentPlayer);
      } 
      
      lcd.setCursor(15, 1);
      lcd.print("v");
    }
    else if (settingsDisplayStart == 2){
      lcd.clear();
      
      if (settingSelected == 3){
        lcd.print(">");
      }
      else{
        lcd.print(" ");
      }
      
      lcd.print("How to play?");
      lcd.setCursor(0, 1);
      
      if (settingSelected == 4){
        lcd.print(">");
      }
      else{
        lcd.print(" ");
      }
      
      lcd.print("Brightness: ");
      lcd.print(matrixBrightnessValue);
      lcd.print(" ");
      
      lcd.setCursor(15, 0);
      lcd.print("^");
      lcd.setCursor(15, 1);
      lcd.print("v");
    }
    else 
      if (settingsDisplayStart == 3) {
        lcd.clear();
        
        if (settingSelected == 5) {
          lcd.print(">");
          lcd.print("Back");
          lcd.setCursor(15, 0);
          lcd.print("^"); 
        }
      }
    
    settingsChanged = 0;
  }
}
// Modify settings
void changeSettings(){
  if (settingsJustStarted){
    settingsChanged = 1;
    settingsJustStarted = 0;
  }
  // Show in main settings modified char
  if (changingName){
    char aux = selectFromAlphabet();
    
    if (aux == '.'){
      return;
    }
    
    currentPlayer[charToReplace] = aux;
    changingName = 0;
  }
  else 
    if (viewInstructionTo){
      if (instructions()){
        return;
      }
      else{
        viewInstructionTo = 0;
        instructionOpt = 1;
        settingsChanged = 1;
      }
    }
  
  displaySettings();
  // modify starting level
  if (settingSelected == 1){
    
    int xAxis = readJoyStickX();
    
    if (xAxis == -1){
      startingLevelValue--;
      
      if (startingLevelValue < 1){
        startingLevelValue = maxLevel;
      }
      
      settingsChanged = 1;
    }
    else 
      if (xAxis == 1){
        startingLevelValue++;
        
        if (startingLevelValue > maxLevel){
          startingLevelValue = 1;
        }
        
        settingsChanged = 1;
      }
  }
  else // select char from name to replace
    if (settingSelected == 2){
      int xAxis = readJoyStickX();
      
      if (xAxis == 1){
        if (charToReplace + 1 < 4){
          charToReplace++;
          settingsChanged = 1;
        }
      }
      else 
        if (xAxis == -1){
          if (charToReplace - 1 >= 0){
            charToReplace--;
            settingsChanged = 1;
          }
        }
    }
    
  int yAxis = readJoyStickY();
  
  if (yAxis == -1){
    settingSelected++;
    
    if (settingSelected > 5){
      settingSelected = 1;
    }
    
    settingsDisplayStart = (settingSelected + 1) / 2;
    settingsChanged = 1;
  }
  else 
    if (yAxis == 1){
      settingSelected--;
      
      if (settingSelected < 1){
        settingSelected = 5;
      }
      
      settingsDisplayStart = (settingSelected + 1) / 2;
      settingsChanged = 1;
    }

  if (settingSelected == 2){
    if (readJoyStickButton()){
      changingName = 1;
      char aux = currentPlayer[charToReplace];
      selectedChar = (int)aux;
    }
  }
  else 
    if (settingSelected == 3){
      if (readJoyStickButton()){
        viewInstructionTo = 1;
        instructionState = 1;
      }
    }
    else // modify brightness
      if (settingSelected == 4) {
        int xAxis = readJoyStickX();
        
        if (xAxis == -1) {
          matrixBrightnessValue--;
          
          if (matrixBrightnessValue < 1) {
            matrixBrightnessValue = maxBrightnessValue;
          }
          
          settingsChanged = 1;
        }
        else 
          if (xAxis == 1) {
            matrixBrightnessValue++;
            
            if (matrixBrightnessValue > maxBrightnessValue) {
              matrixBrightnessValue = 1;
            }
            
            settingsChanged = 1;
          }
      }
      else // back
        if (settingSelected == 5){
          if (readJoyStickButton()){
            menuWasClicked = 0;
            settingsJustStarted = 1;
            settingSelected = 1;
            settingsDisplayStart = 1;
            charToReplace = 0;
          }
        }
}
// Display info
void displayInfo(){
  if (infoChanged){
    if (infoDisplayStart == 1){
      lcd.clear();
      
      lcd.print("Asteroids' game");
      lcd.setCursor(0, 1);
      lcd.print("Dananau Stefan");
      lcd.setCursor(15, 1);
      lcd.print("v");
    }
    else 
      if (infoDisplayStart == 2){
        lcd.clear();
        lcd.print("https://github");
        lcd.setCursor(15, 0);
        lcd.print("^");
        
        lcd.setCursor(0, 1);
        lcd.print(".com/DananauRa");
        lcd.setCursor(15, 1);
        lcd.print("v");
      }
      else
        if (infoDisplayStart == 3) {
          lcd.clear();

          lcd.print("zvan/Dodge-the");
          lcd.setCursor(15, 0);
          lcd.print("^");
          
          lcd.setCursor(0, 1);
          lcd.print("-asteroids");         
        }
    
    infoChanged = 0;
  }

  int yAxis = readJoyStickY();
  
  if (yAxis == 1){
    infoDisplayStart--;
    
    if (infoDisplayStart < 1){
      infoDisplayStart = 3;
    }
    
    infoChanged = 1;
  }
  else 
    if (yAxis == -1){
      infoDisplayStart++;
      
      if (infoDisplayStart > 3){
        infoDisplayStart = 1;
      }
      
      infoChanged = 1;
    }
    
  if (readJoyStickButton()){
    menuWasClicked = 0;
    infoChanged = 1;
    infoDisplayStart = 1;
  }
}
// Display menu
void displayMenu(){
  if (menuChanged){
    lcd.clear();
    
    lcd.setCursor(0, 0);
    if (menuSelected == 1){
      lcd.print(">");
    }
    else {
      lcd.print(" ");
    }
    
    lcd.print("Start ");
    if (menuSelected == 2){
      lcd.print(">");
    }
    else {
      lcd.print(" ");
    }
    
    lcd.print("Settings");
    lcd.setCursor(0, 1);
    if (menuSelected == 3){
      lcd.print(">");
    }
    else {
      lcd.print(" ");
    }
    
    lcd.print("Highscore ");
    if (menuSelected == 4){
      lcd.print(">");
    }
    else {
      lcd.print(" ");
    }
    
    lcd.print("Info");

    menuChanged = 0;
  }
}
// Navigate menu
void menuSelect(){
  if (menuJustStarted){
    menuChanged = 1;
    menuJustStarted = 0;
  }
  
  displayMenu();
  
  int xAxis = readJoyStickX();
  
  if (xAxis == 1){
    menuSelected++;
    
    if (menuSelected > 4){
      menuSelected = 1;
    }
    
    menuChanged = 1;
  }
  else 
    if (xAxis == -1){
      menuSelected--;
      
      if (menuSelected < 1){
        menuSelected = 4;
      
      }
      menuChanged = 1;
    }
  
  int yAxis = readJoyStickY();
  
  if (yAxis != 0){
    if (menuSelected == 1 || menuSelected == 2){
      menuSelected += 2;
    }
    else{
      menuSelected -= 2;
    }
    
    menuChanged = 1;
  }
}
// Game
void playGame(){
  if (gameJustStarted){
    lcd.clear();

    lcd.setCursor(5, 0);
    lcd.print("Ready?");
    delay(2000);
    
    lcd.clear();

    lcd.setCursor(5, 0);
    lcd.print("Start!");
    delay(500);
    // Setup from settings
    gameSetup(startingLevelValue, matrixBrightnessValue);
    gameJustStarted = 0;
  }
  // Highscores write, if exists
  if (gameOver){
    if (gameOverScreenNo == 0){
      if (currentScore > player1.pScore){
        for (int i = 0; i < nameSize; i++){
          player3.pName[i] = player2.pName[i];
        }
        
        player3.pScore = player2.pScore;
        for (int i = 0; i < nameSize; i++){
          player2.pName[i] = player1.pName[i];
        }
        
        player2.pScore = player1.pScore;
        for (int i = 0; i < nameSize; i++){
          player1.pName[i] = currentPlayer[i];
        }
        
        player1.pScore = currentScore;
        gameOverScreenNo = 1;
        
        writePlayer(1, player1);
        writePlayer(2, player2);
        writePlayer(3, player3);
      }
      else 
        if (currentScore > player2.pScore){
          for (int i = 0; i < nameSize; i++){
            player3.pName[i] = player2.pName[i];
          }
          
          player3.pScore = player2.pScore;
          for (int i = 0; i < nameSize; i++){
            player2.pName[i] = currentPlayer[i];
          }
          
          player2.pScore = currentScore;
          gameOverScreenNo = 2;
          
          writePlayer(2, player2);
          writePlayer(3, player3);
        }
        else 
          if (currentScore > player3.pScore){
            for (int i = 0; i < nameSize; i++){
              player3.pName[i] = currentPlayer[i];
            }
            
            player3.pScore = currentScore;
            gameOverScreenNo = 3;
            
            writePlayer(3, player3);
          }
          else {
            gameOverScreenNo = 4;
          }
          
      gameOverScreenChanged = 1;
    }
    // Messages at the end of the game
    if (gameOverScreenNo == 1){
      if (gameOverScreenChanged){
        lcd.clear();
        
        lcd.print("Congrats ");
        lcd.print(currentPlayer);
        lcd.setCursor(0, 1);
        lcd.print("New highscore!");
        
        gameOverScreenChanged = 0;
      }
    }
    else 
      if (gameOverScreenNo == 2){
        if (gameOverScreenChanged){
          lcd.clear();
          
          lcd.print(currentPlayer);
          lcd.print(", You are");
          lcd.setCursor(0, 1);
          lcd.print("in second place.");
          
          gameOverScreenChanged = 0;
        }
      }
      else 
        if (gameOverScreenNo == 3){
          if (gameOverScreenChanged){
            lcd.clear();
            
            lcd.print("Good, ");
            lcd.print(currentPlayer);
            lcd.setCursor(0, 1);
            lcd.print("You're 3rd place");
            
            gameOverScreenChanged = 0;
          }
        }
        else 
          if (gameOverScreenNo == 4){
            if (gameOverScreenChanged){
              lcd.clear();

              lcd.print("Better luck next");
              lcd.setCursor(0, 1);
              lcd.print("time, ");
              lcd.print(currentPlayer);
              
              gameOverScreenChanged = 0;
            }
          }
    // Press to move forward
    if (gameOverScreenNo >= 1 && gameOverScreenNo <= 4){
      if (readJoyStickButton()){
        gameOverScreenNo = 5;
        gameOverScreenChanged = 1;
      }
    }
    else // Play again or go back in menu 
      if (gameOverScreenNo == 5){
        if (gameOverScreenChanged){
          lcd.clear();
          
          lcd.print(currentPlayer);
          lcd.print(": ");
          lcd.print(currentScore);
          
          lcd.setCursor(0, 1);
          if (gameOverSelection == 0){
            lcd.print(">");
          }
          else {
            lcd.print(" ");
          }
          
          lcd.print("Again? ");
          if (gameOverSelection == 1){
            lcd.print(">");
          }
          else {
            lcd.print(" ");
          }
          
          lcd.print("Back");
          
          gameOverScreenChanged = 0;
        }
        
        if (readJoyStickX()){
          gameOverSelection = !gameOverSelection;
          gameOverScreenChanged = 1;
        }
        
        if (readJoyStickButton()){
          gameOver = 0;
          gameJustStarted = 1;
          gameOverScreenNo = 0;
         
          if (gameOverSelection == 1){
            menuWasClicked = 0;
          }
          
          gameOverSelection = 0;
        }
      }

    displayPlayer();
    
    return;
  }

  iterateGame();
  displayGameStatus(currentLives, baseLevel, currentScore);
}

int matrixSize = 8;

int asteroids[] = {0,0,0,0,0,0,0,0};

int playerPos;

bool playerIsDead;
bool dontMoveLevel;

int customLevelMillis;

int randPos;

float gameSpeed;
unsigned long now;

int totalAsteroids;
int totalAsteroidsValue;
int delayAsteroids;
int delayAsteroidsValue;

const int scoreMultiplier = 3;
const float gameSpeedMultiplier = 1.15;

bool gameStarted;

void displayPlayer() {
  lc.setLed(0, matrixSize - 1, playerPos, true);  
}
// On lcd when u play
void displayGameStatus(int currentLives, int levelValue, int currentScore) {
  if (millis() - lastDisplayMillis > displayDelay) {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Lives:");
    lcd.print(currentLives);

    lcd.print(" Level:");
    lcd.print(levelValue);

    lcd.setCursor(0, 1);
    lcd.print("Score:");
    lcd.print(currentScore);

    lastDisplayMillis = millis();
  }
}
// Spawn objects
void spawnAsteroids() {
  if(millis() - now > gameSpeed){
    
    now = millis();
    
    currentScore += scoreMultiplier * baseLevel;
    
    if (gameStarted) {
      gameSpeed /= pow(gameSpeedMultiplier, baseLevel);
      gameStarted = false;
    }
    
    if(delayAsteroids == 1){
      totalAsteroids++;
      
      if (totalAsteroids == totalAsteroidsValue) {
        baseLevel++;
        totalAsteroids = 0;
        gameSpeed /= gameSpeedMultiplier;
      }
     
      randPos = random(0, matrixSize);
        
        if(asteroids[randPos] == 0){  
          asteroids[randPos] = 1;
      }
    }

    if(delayAsteroids != delayAsteroidsValue) {
      delayAsteroids++;
    }
     else {
      delayAsteroids = 1;
     }

    for(int i = 0; i < matrixSize; i++){
      if(asteroids[i] == 10)
        asteroids[i] = 0;
        
      if(asteroids[i] != 0)
        asteroids[i]++;
    }

    lc.clearDisplay(0);
  }
}

void updateDirection() {
  int xAxis = readJoyStickX();  

  if (xAxis == 1) {
    playerPos++;

    if (playerPos > matrixSize - 1) {
      playerPos = 0;

    lc.clearDisplay(0);
    }
  }
  else
    if (xAxis == -1) {
      playerPos--;

      if (playerPos < 0) {
        playerPos = matrixSize - 1;
      }

      lc.clearDisplay(0);
    }
    else {
      lc.clearDisplay(0);
    }
}

void moveAsteroids() {
  for(int i = 0; i < matrixSize; i++){
    if(asteroids[i] > 0)
      lc.setLed(0, asteroids[i] - 2, i, true);
      lc.setLed(0, asteroids[i] - 3, i, true);
  }
}

void detectImpact() {
  if (asteroids[playerPos] == 9){
    asteroids[playerPos] = 0;
    currentLives--;
    
    tone(buzzer, 500);
    explosionAnimation();
    noTone(buzzer);
    delay(500);
    
    if (currentLives == 0) {
      gameOver = true;
      
      lc.clearDisplay(0);
      sadAnimation();
    }
  }
}

void gameSetup(int startingLevelValue, int matrixBrightnessValue){
  lc.clearDisplay(0);
  baseLevel = startingLevelValue;
  currentScore = 0;
  currentLives = startingLives;
  playerIsDead = 0;
  customLevelMillis = 0;
  gameSpeed = 300;
  now = millis();
  delayAsteroids = 1;
  gameOver = false;
  playerPos = 3;
  gameStarted = true;
  totalAsteroids = 0;
  totalAsteroidsValue = 9;
  delayAsteroidsValue = 3;
}
// Runing during the game
void iterateGame(){
  displayPlayer();
  updateDirection();
  spawnAsteroids();
  moveAsteroids();
  detectImpact();
}

void smileAnimation(){
  const byte smileByte[8] = {
    B00000000,
    B01100110,
    B01100110,
    B00000000,
    B10000001,
    B01000010,
    B00111100,
    B00000000
  };

  for (int i = 0; i < matrixSize; i++) {
    lc.setRow(0, i, smileByte[i]);
  }
}

void sadAnimation() {
  const byte sadByte[8] = {
    B00000000,
    B01100110,
    B01100110,
    B00000000,
    B00000000,
    B00111100,
    B01000010,
    B10000001
  };

  for (int i = 0; i < matrixSize; i++) {
    lc.setRow(0, i, sadByte[i]);
  }
}

void explosionAnimation() {
  for(int i = 0; i < 4; i++) {
      lc.setLed(0, 7, playerPos + i, true);
      lc.setLed(0, 7, playerPos - i, true);
      lc.setLed(0, 7 - i, playerPos + i, true);
      lc.setLed(0, 7 - i, playerPos - i, true);
      lc.setLed(0, 7 - 1.5 * i, playerPos, true);
    }
}

void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);
  
  lcd.begin(16, 2);
  
  lc.shutdown(0, false);
  lc.clearDisplay(0);

  player1 = getPlayer(1);
  player2 = getPlayer(2);
  player3 = getPlayer(3);
        
  lcd.print("    Welcome!");
  delay(3000);
  
  smileAnimation();
}

void loop() {
  lc.setIntensity(0, matrixBrightnessValue);
  if (!menuWasClicked){
    if (readJoyStickButton()){
      menuWasClicked = 1;
      menuJustStarted = 1;
    }
    else {
      menuSelect();
    }
  }
  else {
    if (menuSelected == 1){
      playGame();    
    }
    else 
      if (menuSelected == 2){
        changeSettings();
      }
      else 
        if (menuSelected == 3){
          navigateHighScores();
        }
        else 
          if (menuSelected == 4){
            displayInfo();
          }
  }
}
