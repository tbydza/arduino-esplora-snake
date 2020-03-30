#include <Esplora.h>
#include <SPI.h>
#include <TFT.h>

#define SCREENWIDTH 160 //width of tft screen in pixels
#define SCREENHEIGHT 128 //height of tft screen in pixels

//difficulties (as time in ms between each draw):
#define EASY 1000 
#define MEDIUM 500
#define HARD 250

#define BLOCK 8 //size of rectangular block (tile) in pixels

//coordinates of starting position oh snakes head
#define STARTPOS_X 10
#define STARTPOS_Y 1

//buttons aliases:
#define DOWN  SWITCH_1
#define RIGHT  SWITCH_4
#define LEFT SWITCH_2
#define UP  SWITCH_3

//states of game defined by direction of snakes movement
enum states {
  up, down, left, right };
enum states STATE, NEXT_STATE;

//states of cursor in menu
enum menuStates{
  easy,medium,hard};
enum menuStates MSTATE, NEXT_MSTATE;

//to button detection
byte buttonFlag = 0;

//my custom resolutuion
int xRes = SCREENWIDTH / BLOCK;
int yRes = SCREENHEIGHT / BLOCK;

//coordinates of snakes head
int xPosHead;
int yPosHead;

//coordinates of snakes tail
int xPosTail;
int yPosTail;

// X and Y direction of snakes movement
int xDir;
int yDir;

// game field represented as matrix of numbers, each representing corresponding tile
/*
it helds info about game field
-1 represents empty tile
-2 represents tile with food
0 represents head of snake
1 represents body of snake and info that next tile with body is to the right to current position
2 represents body of snake and info that next tile with body is down from current position
3 represents body of snake and info that next tile with body is to the left to current position
4 represents body of snake and info that next tile with body is up from current position
these are important to correctly change position of the tail
*/
int gameField[SCREENHEIGHT / BLOCK][SCREENWIDTH / BLOCK];

//game score
int score;
//represents program state true => game phase, false => menu navigation
bool isGamePhase;
//for first iteration of menu state, to display menu
bool firstIter;
//time between draw
int drawTime;

//it checks wether was the button pressed
bool buttonEvent(int button)
{
  switch (button)
  {
  case RIGHT:
    if (Esplora.readButton(RIGHT) == LOW)
    {
      buttonFlag |= 1;
    }
    else if (buttonFlag & 1)
    {
      buttonFlag ^= 1;
      return true;
    }
    break;

  case DOWN:
    if (Esplora.readButton(DOWN) == LOW)
    {
      buttonFlag |= 2;
    }
    else if (buttonFlag & 2)
    {
      buttonFlag ^= 2;
      return true;
    }
    break;

  case UP:
    if (Esplora.readButton(UP) == LOW)
    {
      buttonFlag |= 4;
    }
    else if (buttonFlag & 4)
    {
      buttonFlag ^= 4;
      return true;
    }
    break;

  case LEFT:
    if (Esplora.readButton(LEFT) == LOW)
    {
      buttonFlag |= 8;
    }
    else if (buttonFlag & 8)
    {
      buttonFlag ^= 8;
      return true;
    }
  }
  return false;
}

//checks if head of snake is out of border (it hits the edge)
bool OutOfBorders()
{
  if (xPosHead >= xRes)
    return true;
  if (xPosHead < 0)
    return true;
  if (yPosHead >= yRes)
    return true;
  if (yPosHead < 0)
    return true;

  return false;
}

//it ends the game and show results
//then it waits until user press any button (to go back to menu)
void GameOver()
{
  char buff[4]; //output char buffer
  String(score).toCharArray(buff, 4);
  EsploraTFT.background(255, 0, 0);
  EsploraTFT.stroke(255, 255, 255);
  EsploraTFT.setTextSize(2);
  EsploraTFT.text("Game Over", 20, 30);
  EsploraTFT.setTextSize(1);
  EsploraTFT.text("Score: ", 20, 60);
  EsploraTFT.text(buff, 70, 60);
  EsploraTFT.text("Press ANY ", 20, 80);
  EsploraTFT.text("of the four buttons", 20, 90);
  EsploraTFT.text("to restart the game.", 20, 100);
  delay(1000); //wait to eliminate double click
  while (true)
  {
    if( buttonEvent(UP)||buttonEvent(DOWN)||buttonEvent(LEFT)||buttonEvent(RIGHT) )
      return;  
  }
}

//main game function, it moves head , and redraw the snake accordingly.
//takes care of scoring, and checking rules
void Draw()
{
  int tempX, tempY; //temporary copy of coordinates
  bool pointGained = false; //for detecting food

  //write info of head direction to game field matrix:
  if (yDir > 0)
    gameField[yPosHead][xPosHead] = 2;
  if (yDir < 0)
    gameField[yPosHead][xPosHead] = 4;

  if (xDir > 0)
    gameField[yPosHead][xPosHead] = 1;
  if (xDir < 0)
    gameField[yPosHead][xPosHead] = 3;

  //draw snakes body part over heads old positions
  EsploraTFT.fill(0, 255, 0);
  EsploraTFT.stroke(0, 255, 0);
  EsploraTFT.rect(xPosHead * BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);

  // change position of head:
  xPosHead += xDir;
  yPosHead += yDir;

  //check if snakes head crosses to snakes body
  if (gameField[yPosHead][xPosHead] >= 0)
  {
    GameOver();
    //set program to menu state and returns
    isGamePhase=false;
    firstIter = true;  
    return;
  }

  //if food was eaten poit was gained
  if (gameField[yPosHead][xPosHead] == -2)
    pointGained = true;

  //write info about heads position into game field matrix
  gameField[yPosHead][xPosHead] = 0;

  //draw head
  EsploraTFT.fill(0, 255, 0);
  EsploraTFT.stroke(255, 255, 255);
  EsploraTFT.rect(xPosHead * BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);

  if (pointGained)
  {
    GenerateFood();    //generate new food item in the game field
    score += 10;       //increment score
  }
  else
  {
    //change position of tail accordingly to direction of snakes body
    tempX = xPosTail;
    tempY = yPosTail;
    switch ( gameField[tempY][tempX] )
    {
    case 1:
      ++xPosTail;
      break;
    case 2:
      ++yPosTail;
      break;
    case 3:
      --xPosTail;
      break;
    case 4:
      --yPosTail;
      break;
    }
    //clear old tail position
    gameField[tempY][tempX] = -1;
    EsploraTFT.fill(0, 0, 0);
    EsploraTFT.stroke(0, 0, 0);
    EsploraTFT.rect(tempX * BLOCK, tempY * BLOCK, BLOCK, BLOCK);
  }

}

//generates food, and randomly place it into the playfield and draw it
void GenerateFood()
{
  int x, y;
  //genarate random coordinates and check if it is safe to place it there else repeat
  do 
  {
    x = (int)random(xRes);
    y = (int)random(yRes);
  }
  while ( gameField[y][x] >= 0 );

  gameField[y][x] = -2;
  EsploraTFT.fill(255, 0, 0);
  EsploraTFT.stroke(255, 0, 0);
  EsploraTFT.rect(x * BLOCK, y * BLOCK, BLOCK, BLOCK);
}

//initialization of game (set up and draw initial blocks)
void GameInit()
{
  xPosHead = STARTPOS_X ;
  yPosHead = STARTPOS_Y ;

  STATE = right;
  NEXT_STATE = right;

  xDir = 1;
  yDir = 0;

  score = 0;

  //game field matrix init
  for (int i = 0; i < yRes; ++i)
  {
    for (int j = 0; j < xRes; ++j)
    {
      gameField[i][j] = -1;
    }
  }

  EsploraTFT.background(0, 0, 0);
  EsploraTFT.fill(0, 255, 0);
  EsploraTFT.stroke(255, 255, 255);

  EsploraTFT.rect(xPosHead * BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);
  EsploraTFT.fill(0, 255, 0);
  EsploraTFT.stroke(0, 255, 0);
  EsploraTFT.rect((xPosHead - 1)*BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);
  EsploraTFT.rect((xPosHead - 2)*BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);
  EsploraTFT.rect((xPosHead - 3)*BLOCK, yPosHead * BLOCK, BLOCK, BLOCK);

  gameField[yPosHead][xPosHead] = 0;
  gameField[yPosHead][xPosHead - 1] = 1;
  gameField[yPosHead][xPosHead - 2] = 1;
  gameField[yPosHead][xPosHead - 3] = 1;

  xPosTail = xPosHead - 3;
  yPosTail = yPosHead;

  GenerateFood();
  isGamePhase = true;
}

//displays menu
void displayMenu()
{

  EsploraTFT.background(0,0,0);
  EsploraTFT.stroke(0,255,0);

  EsploraTFT.setTextSize(3);
  EsploraTFT.text("Snake", 0, 0);

  EsploraTFT.setTextSize(2);
  EsploraTFT.text("for Esplora", 30, 30);

  EsploraTFT.stroke(255,255,255);
  EsploraTFT.setTextSize(1);
  EsploraTFT.text(">  EASY", 30, 60);
  EsploraTFT.text("   MEDIUM",30, 80);
  EsploraTFT.text("   HARD",30, 100);

}

//it graphically moves cursor in the menu acordingly to the parameter
void changeCursorPosition(int pos)
{
  EsploraTFT.stroke(0,0,0);
  EsploraTFT.text(">", 30, 60);
  EsploraTFT.text(">", 30, 80);
  EsploraTFT.text(">", 30, 100);

  EsploraTFT.stroke(255,255,255);

  switch(pos)
  {
  case 1:
    EsploraTFT.text(">", 30, 60);
    break;
  case 2:
    EsploraTFT.text(">",30, 80);
    break;
  case 3:
    EsploraTFT.text(">",30, 100);
    break;
  }
}

//menu navigation
//it tracks position of the cursor in the menu
//set drawTime from chosen difficulty, call init, which set program to game phase
void Menu()
{
  switch (MSTATE)
  {
  case easy:                       
    if(buttonEvent(DOWN))                 
    {
      changeCursorPosition(2);                 
      NEXT_MSTATE = medium;
    }
    else if (buttonEvent(LEFT) || buttonEvent(RIGHT))          
    {
      NEXT_MSTATE = easy;
      drawTime = EASY;
      GameInit();
    }
    break;

  case medium:                       
    if (buttonEvent(DOWN))              
    {
      changeCursorPosition(3);                
      NEXT_MSTATE = hard;
    }
    else if (buttonEvent(UP))            
    {
      changeCursorPosition(1);                 
      NEXT_MSTATE = easy;
    }
    else if (buttonEvent(LEFT) || buttonEvent(RIGHT))        
    {
      NEXT_MSTATE = easy;
      drawTime = MEDIUM;
      GameInit();
    }
    break;

  case hard:                      
    if (buttonEvent(UP))                 
    {
      changeCursorPosition(2);                  
      NEXT_MSTATE = medium;
    }
    else if (buttonEvent(LEFT) || buttonEvent(RIGHT))         
    {
      NEXT_MSTATE = easy;
      drawTime = HARD;
      GameInit();
    }
    break;
  }
  MSTATE = NEXT_MSTATE;
}

//game logic - control of the snake
void Game()
{
  //change direction of snake accordingly to user input
  switch (STATE)
  {
  case up:
    if (buttonEvent(LEFT))
    {
      NEXT_STATE = left;
      xDir = -1;
      yDir = 0;
    }
    else if (buttonEvent(RIGHT))
    {
      NEXT_STATE = right;
      xDir = 1;
      yDir = 0;
    }
    break;

  case down:
    if (buttonEvent(LEFT))
    {
      NEXT_STATE = left;
      xDir = -1;
      yDir = 0;
    }
    else if (buttonEvent(RIGHT))
    {
      NEXT_STATE = right;
      xDir = 1;
      yDir = 0;
    }
    break;

  case right:
    if (buttonEvent(UP))
    {
      NEXT_STATE = up;
      xDir = 0;
      yDir = -1;
    }
    else if (buttonEvent(DOWN))
    {
      NEXT_STATE = down;
      xDir = 0;
      yDir = 1;
    }
    break;

  case left:
    if (buttonEvent(UP))
    {
      NEXT_STATE = up;
      xDir = 0;
      yDir = -1;
    }
    else if (buttonEvent(DOWN))
    {
      NEXT_STATE = down;
      xDir = 0;
      yDir = 1;
    }
    break;

  }
  STATE = NEXT_STATE;

  if (OutOfBorders())
  {
    GameOver();
    //set program to menu state and returns
    isGamePhase = false;
    firstIter = true;  
    return;
  }

  //call of DRAW function
  //drawTime depends on difficulty setting
  if (millis() % drawTime == 0)
  {
    Draw();
  } 
}

//arduino setup and program menu init
void setup() 
{
  EsploraTFT.begin();
  Serial.begin(9600);
  randomSeed(analogRead(0));
  MSTATE = easy;
  firstIter = true;  
}

//main loop
void loop() 
{
  if(isGamePhase)
  {
    //game phase
    Game();
  }
  else
  {
    //menu navigation phase
    if(firstIter)
    {
      //show menu first
      firstIter = false;
      displayMenu();
    }
    Menu();
  }
}

