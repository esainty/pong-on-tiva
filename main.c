#include "lcd.h"         // look in here for the LCD functions

//Systick
#define NVIC_ST_BASE ((unsigned long *) 0xE000E000)
#define NVIC_ST_CTRL ((unsigned long *) 0xE000E010)
#define NVIC_ST_RELOAD ((unsigned long *) 0xE000E014)
#define RELOAD_VALUE ((unsigned long) 0x2625A0)
#define NVIC_ST_CURRENT ((unsigned long *) 0xE000E018)

//System
#define RCGCGPIO ((unsigned long *) 0x400FE608)
#define PRGPIO ((unsigned long *) 0x400FEA08)
#define RCGCADC ((unsigned long *)0x400FE638)
#define SYSCTL_PRADC ((unsigned long *)0x400FEA38)
	
//GPIOB
#define GPIOB_BASE ((unsigned long *) 0x40005000)
#define GPIOB_DATA ((unsigned long *) 0x400053FC)
#define GPIOB_DIR ((unsigned long *) 0x40005400)
#define GPIOB_AFSEL ((unsigned long *) 0x40005420)
#define GPIOB_DR8R ((unsigned long *) 0x40005508)
#define GPIOB_DEN ((unsigned long *) 0x4000551C)
#define GPIOB_AMSEL ((unsigned long *) 0x40005528)
#define GPIOB_IM	((unsigned long *) 0x40005410)

//ADC
#define ADC0_ADCPP ((unsigned long *)0x40038FC0)
#define ADC1_ADCPP ((unsigned long *)0x40039FC0)
#define ADC0_ACTSS ((unsigned long *)0x40038000)
#define ADC1_ACTSS ((unsigned long *)0x40039000)
#define ADC0_EMUX ((unsigned long *)0x40038014)
#define ADC1_EMUX ((unsigned long *)0x40039014)
#define ADC0_SSMUX ((unsigned long *)0x400380A0)
#define ADC1_SSMUX ((unsigned long *)0x400390A0)
#define ADC0_SSCTL ((unsigned long *)0x400380A4)
#define ADC1_SSCTL ((unsigned long *)0x400390A4)
#define ADC0_FIFO ((unsigned long *)0x400380A8)
#define ADC1_FIFO ((unsigned long *)0x400390A8)
#define ADC0_IM ((unsigned long *)0x40038008)
#define ADC1_IM ((unsigned long *)0x40039008)
#define ADC0_ISC ((unsigned long *)0x4003800C)
#define ADC1_ISC ((unsigned long *)0x4003900C)

//Interrupts
#define NVIC_EN0 ((unsigned long *)0xE000E100)
#define NVIC_EN1 ((unsigned long *)0xE000E104)

//Function headers
void setupSystick(void);
void setupGPIOF(void);
void setupGPIOB(void);
void setupADC(void);
void systick_ISR(void);
void setupEnvironment(void);
void potentiometer_ISR(void);
void drawHalfway(void);
void drawPaddles(void);
void clearPaddles(void);
void clearPaddle(int paddle, int offset);
int drawPaddle(int paddle, int offset);
void drawBall(void);
void drawScores(void);
void clearBall(void);
void roundOver(void);
void newRound(void);
void gameOver(void);
void moveBall(void);
void movePaddles(void);
void detectContact(void);
void fillScreen(void);
void clearScreen(void);
void displayWinner(void);
void borderAnimation(void);

//Global variables
unsigned int halfway[28] = {0,1,2,3,10,11,12,13,20,21,22,23,30,31,32,33,40,41,42,43,50,51,52,53,60,61,62,63};
int paddleA = 24;
int paddleB = 24;
int ball[] = {30, 62}; //y, x 
//-1 for moving up. 1 for moving down.
int directionX;
int directionY;
short roundFlag = 0;
short scoreA;
short scoreB;


/********************* MAIN ****************************/
int main(void) {
	//Initialise subsystems
	InitLCD();
	setupGPIOB();
	setupADC();
	setupSystick();
	//Initialise variables
	setupEnvironment();
	while (1) __asm("nop") ; // end spin (should never pass here);
}  // end main

void setupSystick(void) {
	//Initialise Systick with the reload value defined within RELOAD_VALUE.
	*NVIC_ST_CTRL = *NVIC_ST_CTRL & ~0x1;
	*NVIC_ST_RELOAD = RELOAD_VALUE;
	*NVIC_ST_CURRENT = 0;
	*NVIC_ST_CTRL = *NVIC_ST_CTRL | 0x6;
	//Enable
	*NVIC_ST_CTRL = *NVIC_ST_CTRL | 0x1;
}

void setupGPIOB(void) {
	//Set up Port B pins 4 and 5.
	*RCGCGPIO = *RCGCGPIO | 0x2;
	while ((*PRGPIO & 0x2) != 0x2) 
		__asm("NOP");
	//Set as input.
	*GPIOB_DIR = *GPIOB_DIR & ~0x30;
	*GPIOB_AFSEL = *GPIOB_AFSEL | 0x30;
	//Disable digital.
	*GPIOB_DEN = *GPIOB_DEN & ~0x30;
	//Enable analog.
	*GPIOB_AMSEL = *GPIOB_AMSEL | 0x30;
}

void setupADC(void) {
	//Setup ADC0 and ADC1 to use Input 10 and 11 respectively.
	*RCGCADC |= 3;
	while ((*SYSCTL_PRADC & 0x3) != 3) {
		__asm ("NOP");
	}
	*ADC0_ADCPP = 0x1;
	*ADC1_ADCPP = 0x1;
	*ADC0_ACTSS &= ~0x8;
	*ADC1_ACTSS &= ~0x8;
	*ADC0_EMUX |= 0xF << 12;
	*ADC1_EMUX |= (0xF << 12);
	*ADC0_SSMUX |= 0xA;
	*ADC1_SSMUX |= 0xB;
	*ADC0_SSCTL |= 0x2;
	*ADC1_SSCTL |= 0x2;
	//Enable
	*ADC0_ACTSS |= 0x8;
	*ADC1_ACTSS |= 0x8;
}

void setupEnvironment(void) {
	//Baseline internal variables to initial values.
	directionX = 1;
	directionY = 1;
	scoreA = 0;
	scoreB = 0;
	drawHalfway();
	movePaddles();
	drawBall();
	drawScores();
}

void systick_ISR(void) {
	movePaddles();
	if (roundFlag > 0) {
		roundOver();
	} else {
		moveBall();
	}
}

void drawHalfway(void) {
	//Draw the halfway marker. 
	int i;
	for (i = 0; i < 28; i++) {
		DrawPixel(63, halfway[i]);
	}
}

void drawScores(void) {
	char charScoreA = scoreA + 0x30;
	char charScoreB = scoreB + 0x30;
	CursorPos(9, 0);
	PutcharLCD(charScoreA);
	CursorPos(12, 7);
	PutcharLCD(charScoreB);
}

void drawPaddles(void){
	//Draw both paddles. Both 10 pixels from their respective edge.
	paddleA = drawPaddle(paddleA, 8);
	paddleB = drawPaddle(paddleB, 116);
}

void clearPaddles(void) {
	//Erase both paddles. Both 10 pixels from their respective edge.
	clearPaddle(paddleA, 8);
	clearPaddle(paddleB, 116);
}

int drawPaddle(int paddle, int offset) {
	int i;
	int j;
	//Check if paddle outside permitted bounds. Reset otherwise.
	if (paddle > 48) {
		paddle = 48;
	} else if (paddle < 0) {
		paddle = 0;
	}
	//Draw paddle.
	for (i = 0; i < 15; i++) {
		for (j = 0; j < 2; j++) {
			DrawPixel(offset + j, paddle + i);
		}
	}
	//Sustain bounds changes.
	return paddle;
}

void clearPaddle(int paddle, int offset) {
	int i;
	int j;
	//Draw paddle.
	for (i = 0; i < 15; i++) {
		for (j = 0; j < 2; j++) {
			ClearPixel(offset + j, paddle + i);
		}
	}
}

void drawBall(void) {
	int i;
	int j;
	//Check if ball inside permitted bounds. Reset otherwise.
	if (ball[0] == 0) {
		directionY = 1;
	} else if (ball[0] == 61) {
		directionY = -1;
	} else if (ball[1] == 0) {
		roundFlag = 1;
	} else if (ball[1] == 125) {
		roundFlag = 2;
	}
	//Draw 3x3 ball at location ball[1], ball[0].
	if (roundFlag ==0) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				DrawPixel(ball[1] + i, ball[0] + j); 
			}
		}
	}
}

void clearBall(void) {
	//Delete pixels in a 3x3 square from the ball's position.
	int i;
	int j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			ClearPixel(ball[1] + i, ball[0] + j); 
		}
	}
	//Redraw halfway mark if destruction occurs.
	if (ball[1] > 60 && ball[1] < 64) {
		drawHalfway();
	}
}

void detectContact(void) {
	//Checks ball x position against paddles (no overlap permitted). Done first to avoid unnecessary branches.
	if (ball[1] == 10 || ball[1] == 113) {
		//Checks ball y position against paddles.
		if ((ball[0] < paddleA + 15 && ball[0] > paddleA - 3) || (ball[0] < paddleB + 15 && ball[0] > paddleB - 3)) {
			//If both conditions true, then ball has contacted a paddle and the direction should be reversed.
			directionX *= -1;
		}
	}
}

void moveBall(void) {
	//Remove old ball.
	clearBall();
	//Update ball coordinates.
	ball[0] += directionY;
	ball[1] += directionX;
	//Draw new ball.
	drawBall();
	//Check if the ball is in contact with paddles.
	detectContact();
}

void movePaddles(void) {
	//Generate new value for paddleA and paddleB according to ADC input values.
	int ADC0 = *ADC0_FIFO;
	int ADC1 = *ADC1_FIFO;
	//Erase old paddles.
	clearPaddles();
	//Update paddles.
	paddleA = (ADC0 / 83);
	paddleB = (ADC1 / 83);
	//Draw updated paddles.
	drawPaddles();
}

void roundOver(void) {
	//Disable interrupts.
	*NVIC_ST_CTRL = *NVIC_ST_CTRL & ~0x1;
	//Increment score.
	if (roundFlag == 1) {
		scoreB++;
	} else if (roundFlag == 2) {
		scoreA++;
	}
	//Check for game over condition.
	if (scoreA > 5 || scoreB > 5) {
		gameOver();
	}
	//Reset roundFlag.
	roundFlag = 0;
	//New round.
	newRound();
	//Re-enable interrupts.
	*NVIC_ST_CTRL = *NVIC_ST_CTRL | 0x1;
}

void newRound() {
	int i;
	//Run border animation 10 times as a delay.
	for (i = 0; i < 10; i++) {
		borderAnimation();
	}
	//Reset values.
	ball[0] = 30;
	ball[1] = 62;
	directionX *= -1;
	paddleA = 24;
	paddleB = 24;
	//Draw initial screen objects.
	drawHalfway();
	drawPaddles();
	drawBall();
	drawScores();
}

void gameOver(void) {
	//Disable interrupts.
	*NVIC_ST_CTRL = *NVIC_ST_CTRL & ~0x1;
	//Screen fill animation.
	fillScreen();
	clearScreen();
	displayWinner();
	fillScreen();
	clearScreen();
	//Reset variables to baseline values.
	setupEnvironment();
	//Enable interrupts.
	*NVIC_ST_CTRL = *NVIC_ST_CTRL ^ 0x1;
}

void fillScreen() {
	//Function to individually fill the screen with pixels in a spiral fashion.
	int x = 1;
	int y = 1;
	int i;
	int posX = 63;
	int posY = 31;
	int incrementer = 0;
	int fallback = 0;
	int boundFlag = 0;
	
	//Draw starting pixel;
	DrawPixel(posX, posY);
	//While the X pointer is less than the max screen width;
	while (posX < 127) {
		fallback++;
		//Boundflag indicates if the X row fills are still necessary.
		if (boundFlag == 0) {
			//Incrementer should only be increased while the distance to travel per spiral side is increasing.
			incrementer++;
			//Draw spiral side in the X direction.
			for (i = 0; i < incrementer; i++) {
				posX += x;
				DrawPixel(posX, posY);
			}
			//Invert the x modifier to traverse in the opposite direction next loop.
			x *= -1;
			if (posY == 63) {
				boundFlag = 1;
				incrementer--;
			}
			//If the boundflag is set and the pointer is under halfway, switch to opposite side of spiral.
		} else if (posX < 63) {
			posX += fallback;
			DrawPixel(posX, posY);
			//If the boundflag is set and the pointe ris over halfway, switch to opposite side of spiral.
		} else if (posX > 63) {
			posX -= fallback;
			DrawPixel(posX, posY);
		}
		//Draw spiral side in the Y direction.
		for (i = 0; i < incrementer; i++) {
			posY += y;
			DrawPixel(posX, posY);
		}
		//Invert the y modifier to traverse in the opposite direction next loop.
		y *= -1;
	} 
}

void clearScreen(void) {
	//Function to individually clear the screen of pixels in a spiral fashion.
	int x = 1;
	int y = 1;
	int i;
	int posX = 63;
	int posY = 31;
	int incrementer = 0;
	int fallback = 0;
	int boundFlag = 0;
	
	//Clear starting pixel;
	ClearPixel(posX, posY);
	//While the X pointer is less than the max screen width;
	while (posX < 127) {
		fallback++;
		//Boundflag indicates if the X row clears are still necessary.
		if (boundFlag == 0) {
			//Incrementer should only be increased while the distance to travel per spiral side is increasing.
			incrementer++;
			//Clear spiral side in the X direction.
			for (i = 0; i < incrementer; i++) {
				posX += x;
				ClearPixel(posX, posY);
			}
			//Invert the x modifier to traverse in the opposite direction next loop.
			x *= -1;
			if (posY == 63) {
				boundFlag = 1;
				incrementer--;
			}
			//If the boundflag is set and the pointer is under halfway, switch to opposite side of spiral.
		} else if (posX < 63) {
			posX += fallback;
			ClearPixel(posX, posY);
			//If the boundflag is set and the pointe ris over halfway, switch to opposite side of spiral.
		} else if (posX > 63) {
			posX -= fallback;
			ClearPixel(posX, posY);
		}
		//Clear spiral side in the Y direction.
		for (i = 0; i < incrementer; i++) {
			posY += y;
			ClearPixel(posX, posY);
		}
		//Invert the y modifier to traverse in the opposite direction next loop.
		y *= -1;
	}
}

void displayWinner(void) {
	//Display the winner to the screen.
	char message[8] = {'W','I','N','N','E','R',':',' ',};
	int i;
	CursorPos(6, 4);
	for (i = 0; i < 8; i++) {
		PutcharLCD(message[i]);
	}
	if (scoreA > 5) {
		PutcharLCD('A');
	} else if (scoreB > 5) {
		PutcharLCD('B');
	}
	//Border animation for delay.
	for (i = 0; i < 10; i++) {
		borderAnimation();
	}
}

void borderAnimation(void) {
	int i;
	int j;
	int x = 1;
	int y = 1;
	int posX = 0;
	int posY = 0;
	//Draw a line around the outside of the screen.
	for (j = 0; j < 2; j++) {
		for (i = 0; i < 127; i++) {
			DrawPixel(posX, posY);
			posX += x;
		}
		//Reverse x direction.
		x *= -1;
		for (i = 0; i < 63; i++) {
			DrawPixel(posX, posY);
			posY += y;
		}
		//Reverse y direction.
		y *= -1;
	}
	//Erase the line.
	for (j = 0; j < 2; j++) {
		for (i = 0; i < 127; i++) {
			ClearPixel(posX, posY);
			posX += x;
		}
		x *= -1;
		for (i = 0; i < 63; i++) {
			ClearPixel(posX, posY);
			posY += y;
		}
		y *= -1;
	}
}


