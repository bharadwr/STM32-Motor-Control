/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"

#define ROWS 6
#define COLS 6
#define NodeROWS 36
#define NodeCOLS 5

bool chkOpenNode(int, int);
bool chkClosedNode(int, int);

int speed = 1;
int forward = 10;
int lr = 10;
int globalDir;

int directions[36 * 4] = {0};

int path[ROWS][COLS] =
{
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0}
};

int obstacles[ROWS][COLS] =
{
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0},
		{0,0,0,0,0,0}
};

bool di[8][8];

int start[2] = {0,0};
int end[2] = {1,1};
int curr[2] = {0,0};

//int open[][];
int open[NodeROWS][NodeCOLS];

int openNodes;
//int closed[][];
int closed[NodeROWS][NodeCOLS];

int closedNodes;



void nano_wait(int t) {
	asm("       mov r0,%0\n"
			"repeat:\n"
			"       sub r0,#83\n"
			"       bgt repeat\n"
			: : "r"(t) : "r0", "cc");
}


int main(void)
{
	init_pwm1();
	//init_pwm2();

	init_direcX();



	initPA0();

	testRun();
	//globalDir = 1;



	for(;;);
}

void fullRun()
{
	while(true)
	{
		if((GPIOA->IDR & 0x1 ) == 0)
			break;

		nanowait(250*100*1000);

		initGPIO();
		PathFinder();
		FindPath();



		for(int i=0; i<8;i++)
		{
			for(int j= 0; j < 8; j++)
			{
				di[i][j] = false;
			}

		}

		for(int i=0; i<6;i++)
		{
			for(int j= 0; j < 6; j++)
			{
				if(path[i][j] == 1)
					di[i+1][j+1] = true;
			}
		}

		getDirections(di, curr[0], curr[1]);

		int count = 0;

		while(directions[count]!=0)
		{
			// 1 = forward, 2 = left, 3 = right, 4 = back
			switch(directions[count])
			{
			case 1:
			{
				F();
				break;
			}
			case 2:
			{
				L();
				break;
			}
			case 3:
			{
				R();
				break;
			}
			case 4:
			{
				L();
				L();
				F();
				L();
				L();
				break;
			}
			}


			count++;
		}


		//get directions
		//follow directions
		//turn around
		//		L();
		//		L();
		//		if (globalDir == 1) {
		//			globalDir = 2;
		//		}


		testLED();
		nanowait(250*100*1000);
	}
}

void testRun()
{
	F();
	B();
	L();
	R();
	testLED();
	/*
	 * if (forward > 0)
	 * 	F(forward);
	 * if (lr > 0)
	 *	L(lr);
	 * if (lr < 0)
	 *  R(lr);
	 */
}

void testLED(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER |=  0x00040000;
	GPIOC->MODER |=  0x00010000;

	GPIOC->ODR |=  1<<8;
}

void initPA0()
{
	GPIOA -> MODER &= ~0x11;
}


void init_direcX(void)
{
	GPIOA -> MODER &= ~(3<<8);
	GPIOA -> MODER |= 1<<8;
	GPIOA -> MODER &= ~(3<<10);
	GPIOA -> MODER |= 1<<10;
	GPIOA -> MODER &= ~(3<<12);
	GPIOA -> MODER |= 1<<12;
	GPIOA -> MODER &= ~(3<<14);
	GPIOA -> MODER |= 1<<14;

	GPIOA->BRR |= 1<<1|1<<2|1<<3|1<<4;
}


void init_pwm1(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	GPIOA -> MODER &= ~(3<<16);
	GPIOA -> MODER |= 2<<16;
	GPIOA -> MODER &= ~(3<<18);
	GPIOA -> MODER |= 2<<18;
	//GPIOA -> MODER &= ~(3<<20);
	//GPIOA -> MODER |= 2<<20;

	GPIOA->AFR[1] &= ~0xff;
	GPIOA->AFR[1] |= 0x22;

	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	TIM1->CR1 &= ~TIM_CR1_DIR;//upcounter
	TIM1->PSC = 48000 - 1;
	TIM1->ARR = 100 - 1;

	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;


	TIM1->BDTR |= TIM_BDTR_MOE;

	TIM1->CCMR1 &= ~TIM_CCMR1_OC1M_0;
	TIM1->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

	TIM1->CCMR1 &= ~TIM_CCMR1_OC2M_0;
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;

	TIM1->CCER |= TIM_CCER_CC1E;
	TIM1->CCER |= TIM_CCER_CC2E;

	TIM1->CR1 |= TIM_CR1_CEN;
}


void init_pwm2(void)
{
	//RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	GPIOA -> MODER &= ~(3<<2);
	GPIOA -> MODER |= 2<<2;
	GPIOA -> MODER &= ~(3<<4);
	GPIOA -> MODER |= 2<<4;


	GPIOA->AFR[0] &= ~0xff0;
	GPIOA->AFR[0] |= 0x220;

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	TIM2->CR1 &= ~TIM_CR1_DIR;//upcounter
	TIM2->PSC = 48000 - 1;
	TIM2->ARR = 100 - 1;

	TIM2->CCR2 = 0;
	TIM2->CCR3 = 0;


	//TIM1->BDTR |= TIM_BDTR_MOE;

	TIM2->CCMR1 &= ~TIM_CCMR1_OC2M_0;
	TIM2->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;

	TIM2->CCMR2 &= ~TIM_CCMR2_OC3M_0;
	TIM2->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;

	TIM1->CCER |= TIM_CCER_CC2E;
	TIM1->CCER |= TIM_CCER_CC3E;

	TIM1->CR1 |= TIM_CR1_CEN;
}

void F(int distance)
{
	TIM1->CCR1 = 49 + 10 * speed;
	TIM1->CCR2 = 49 + 10 * speed;

	GPIOA->BSRR |= 1<<5 | 1<<7;
	GPIOA->BRR |= 1<<4 | 1<<6;

	for(int i=0; i<10; i++) {
		/*if (read the hall value is not 0)
		 *	increment distance
		 *if (distance >= F/B distance)
		 *	break
		*/
		nano_wait(250*1000*1000);//enter time to go forward
	}
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void B(void)
{
	TIM1->CCR1 = 49 + 10 * speed;
	TIM1->CCR2 = 49 + 10 * speed;

	GPIOA->BSRR |= 1<<4 | 1 << 6;
	GPIOA->BRR |= 1<<5 | 1<<7 ;

	for(int i=0; i<10; i++) {
		/*if (read the hall value is not 0)
		 *	increment distance
		 *if (distance >= F/B distance)
		 *	break
		*/
		/* if (ir is high)
		 * 	exit(0);
		 */
		nano_wait(250*1000*1000);//enter time to go forward
	}

	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void R(void)
{
	TIM1->CCR1 = 75;
	TIM1->CCR2 = 75;

	GPIOA->BSRR |= 1<<5 | 1<<6;
	GPIOA->BRR |= 1<<4 | 1<<7;

	for(int i=0; i<10; i++)
		nano_wait(250*1000*1000);//enter time to go forward
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void L(void)
{
	TIM1->CCR1 = 75;
	TIM1->CCR2 = 75;

	GPIOA->BSRR |= 1<<4 | 1<<7;
	GPIOA->BRR |= 1<<5 | 1<<6;
	for(int i=0; i<10; i++)
		nano_wait(250*1000*1000);//enter time to go forward
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}




void getDirections(bool board [8][8], int x, int y)
{

	//	int * directions = malloc(sizeof(int) * 36 * 4); // 1 = forward, 2 = left, 3 = right, 4 = back
	//memset(directions, 0, sizeof(int) * 36 * 4 );
	//directions = {0};
	int i;
	int j;
	int idx = 0;
	for (int i = 0; i < 36; ++i) {
		if(board[y][x]) { //this if is not needed
			if (board[y][x - 1] && globalDir != 4) { //check left
				if(globalDir == 1) {
					directions[idx] = 2;
					idx++;
					directions[idx] = 1;
				}

				else if(globalDir == 2) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}
				else if(globalDir == 3) {
					directions[idx] = 1;
				}
				else {
					directions[idx] = 3;
					idx++;
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}

				x = x - 1;
				globalDir = 3;
				idx++;
			}

			else if (board[y][x + 1] && globalDir != 3) { //check right
				if(globalDir == 1) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}

				else if(globalDir == 2) {
					directions[idx] = 2;
					idx++;
					directions[idx] = 1;
				}
				else if(globalDir == 3) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}
				else {
					directions[idx] = 1;
				}

				x = x + 1;
				globalDir = 4;
				idx++;
			}

			else if (board[y-1][x] && globalDir != 2) { //check above you
				if(globalDir == 1) {
					directions[idx] = 1;
				}

				else if(globalDir == 2) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}
				else if(globalDir == 3) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}
				else {
					directions[idx] = 2;
					idx++;
					directions[idx] = 1;
				}

				y = y - 1;
				globalDir = 1;
				idx++;
			}

			else if (board[y+1][x] && globalDir != 1) { //check below you
				if(globalDir == 1) {
					directions[idx] = 3;
					idx++;
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}

				else if(globalDir == 2) {
					directions[idx] = 1;
				}
				else if(globalDir == 3) {
					directions[idx] = 2;
					idx++;
					directions[idx] = 1;
				}
				else {
					directions[idx] = 3;
					idx++;
					directions[idx] = 1;
				}

				y = y + 1;
				globalDir = 2;
				idx++;
			}

			// else {
			// 	break;
			// }

		}
	}

	for (int i = x; i < 6; i++) {
		for (int j = y; j < 6; j++) {
			if (board[i][j] == 0) {

			}
		}
	}


	return;// directions;
}

void initGPIO(void)
{
	RCC->AHBENR = RCC_AHBENR_GPIOBEN;

	GPIOB->MODER &= ~0xffffff; //clearing pins PB0 - 11
	GPIOB->MODER |= 0x555000; //making pins PB0 - 5 inputs and PB6 - 11 outputs

	//row0
	GPIOB -> ODR |= 1 << 6; //turn on pb6
	obstacles[0][0] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][0] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][0] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][0] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][0] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][0] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 6); //turn off pb6

	//row1
	GPIOB -> ODR |= 1 << 7; //turn on pb7
	obstacles[0][1] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][1] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][1] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][1] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][1] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][1] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 7); //turn off pb7

	//row2
	GPIOB -> ODR |= 1 << 8; //turn on pb8
	obstacles[0][2] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][2] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][2] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][2] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][2] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][2] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 8); //turn off pb8

	//row3
	GPIOB -> ODR |= 1 << 9; //turn on pb9
	obstacles[0][3] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][3] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][3] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][3] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][3] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][3] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 9); //turn off pb9

	//row4
	GPIOB -> ODR |= 1 << 10; //turn on pb10
	obstacles[0][4] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][4] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][4] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][4] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][4] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][4] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 10); //turn off pb10

	//row5
	GPIOB -> ODR |= 1 << 11; //turn on pb11
	obstacles[0][5] = GPIOB -> IDR & GPIO_IDR_0;
	obstacles[1][5] = GPIOB -> IDR & GPIO_IDR_1;
	obstacles[2][5] = GPIOB -> IDR & GPIO_IDR_2;
	obstacles[3][5] = GPIOB -> IDR & GPIO_IDR_3;
	obstacles[4][5] = GPIOB -> IDR & GPIO_IDR_4;
	obstacles[5][5] = GPIOB -> IDR & GPIO_IDR_5;
	GPIOB -> ODR &= ~(1 << 11); //turn off pb11

}

int abs(int a)
{
	int ret = a;
	if(a < 0)
		ret = -a;

	return ret;
}


void PathFinder()
{


	openNodes = 0;
	closedNodes = 0;

	// start[2];
	start[0] = 1;
	start[1] = 2;

	//curr[2];
	curr[0] = start[0];
	curr[1] = start[1];

	//end[2];
	end[0] = 4;
	end[1] = 4;

	for(int i = 0; i <36;i++)
	{
		for(int j = 0; j<5;j++)
		{
			open[i][j] = 0;
			closed[i][j] = 0;

		}
	}



}


void FindPath()
{
	int g = 0;
	int h = abs(end[0] - curr[0]) + abs(end[1] - curr[1]);
	int f = g + h;
	addClosedNode(curr[0], curr[1],f,g,h );

	int currpass = 0;
	int passes = 5;
	//System.out.println("end = " +end[0]+","+end[1]);

	//forward tracking
	while(curr[0]!= end[0] || curr[1]!= end[1])
		//while(currpass < passes)
	{

		//4 adjacent tiles
		if(curr[0] - 1 >= 0)
		{
			if(obstacles[curr[0] - 1][curr[1]] ==0)
			{
				int g1 = g+1;
				int h1 = abs(end[0] - (curr[0] - 1)) + abs(end[1] - curr[1]);
				int f1 = g1+h1;

				if(!chkOpenNode(curr[0] - 1, curr[1]) && !chkClosedNode(curr[0] - 1, curr[1]))
					addOpenNode(curr[0] - 1, curr[1],f1,g1,h1);
			}
		}

		if(curr[0] + 1 < 6)
		{
			if(obstacles[curr[0] + 1][curr[1]] == 0)
			{
				int g1 = g+1;
				int h1 = abs(end[0] - (curr[0] + 1)) + abs(end[1] - curr[1]);
				int f1 = g1+h1;

				if(!chkOpenNode(curr[0] + 1, curr[1]) && !chkClosedNode(curr[0] + 1, curr[1]))
					addOpenNode(curr[0] + 1, curr[1],f1,g1,h1);
			}
		}

		if(curr[1] - 1 >= 0)
		{
			if(obstacles[curr[0]][curr[1] - 1] == 0)
			{
				int g1 = g+1;
				int h1 = abs(end[0] - curr[0]) + abs(end[1] - (curr[1] - 1));
				int f1 = g1+h1;

				if(!chkOpenNode(curr[0], curr[1] - 1) && !chkClosedNode(curr[0], curr[1] - 1))
					addOpenNode(curr[0], curr[1] - 1,f1,g1,h1);
			}
		}

		if(curr[1] + 1 < 6)
		{
			if(obstacles[curr[0]][curr[1] + 1] == 0)
			{
				int g1 = g+1;
				int h1 = abs(end[0] - curr[0]) + abs((end[1] - curr[1]) + 1);
				int f1 = g1+h1;

				if(!chkOpenNode(curr[0] , curr[1] + 1) && !chkClosedNode(curr[0] , curr[1] + 1))
					addOpenNode(curr[0] , curr[1] + 1,f1,g1,h1);
			}
		}

		//Update current node


		int index = openNodes - 1;
		for(int i = openNodes - 2; i>=0;i--)
		{
			if(open[i][2] < open[index][2])
				index = i;
		}

		curr[0] = open[index][0];
		curr[1] = open[index][1];
		f = open[index][2];
		g = open[index][3];
		h = open[index][4];

		rmOpenNode(curr[0],curr[1]);
		addClosedNode(curr[0],curr[1],f,g,h);

		//path[curr[0]][curr[1]] = true;

		currpass++;
		//System.out.println("curr = " +curr[0]+","+curr[1]);
		//System.out.println("openNodes = " + openNodes);

	}//end of forward tracking


	//	System.out.println("closed = " + closedNodes);
	//	System.out.println("B g = " + g);
	//	System.out.println("closed f = " + closed[closedNodes - 2][2]);


	//backtracking
	path[curr[0]][curr[1]] = true;


	//int backindex = closedNodes - 2;

	// currpass = 0;
	//passes = 7;


	//while(currpass < passes)
	while(curr[0]!= start[0] || curr[1]!= start[1])
	{
		for(int v = closedNodes - 2; v >= 0; v--)
		{

			if(closed[v][3] == g - 1 &&  abs(curr[0] - closed[v][0]) <= 1 && abs(curr[1] - closed[v][1])<=1 )
			{
				curr[0] = closed[v][0];
				curr[1] = closed[v][1];
				g = closed[v][3];

				path[curr[0]][curr[1]] = true;
				break;
			}
		}

		//backindex--;
		//currpass++;

	}


}


void addOpenNode(int j, int i, int f, int g, int h)
{

	openNodes++;


	open[openNodes - 1][0] = j;
	open[openNodes - 1][1] = i;
	open[openNodes - 1][2] = f;
	open[openNodes - 1][3] = g;
	open[openNodes - 1][4] = h;





}

void rmOpenNode(int j, int i)
{


	bool flag = true;

	for(int k = 0; k < openNodes ; k++)
	{
		if(flag)
		{
			if(open[k][0] == j && open[k][1] == i)
				flag = false;


		}
		else
		{
			open[k-1][0] = open[k][0];
			open[k-1][1] = open[k][1];
			open[k-1][2] = open[k][2];
			open[k-1][3] = open[k][3];
			open[k-1][4] = open[k][4];

		}
	}

	openNodes--;


}

bool chkOpenNode(int j, int i)
{
	bool ret = false;
	for(int k = 0; k < openNodes ; k++)
	{
		if(open[k][0] == j && open[k][1] == i)
			ret = true;

	}

	return ret;

}


void addClosedNode(int j, int i, int f, int g, int h)
{
	closedNodes++;


	closed[closedNodes - 1][0] = j;
	closed[closedNodes - 1][1] = i;
	closed[closedNodes - 1][2] = f;
	closed[closedNodes - 1][3] = g;
	closed[closedNodes - 1][4] = h;


}


void rmClosedNode(int j, int i)
{

	bool flag = true;

	for(int k = 0; k < closedNodes ; k++)
	{
		if(flag)
		{
			if(closed[k][0] == j && closed[k][1] == i)
				flag = false;


		}
		else
		{
			closed[k-1][0] = closed[k][0];
			closed[k-1][1] = closed[k][1];
			closed[k-1][2] = closed[k][2];
			closed[k-1][3] = closed[k][3];
			closed[k-1][4] = closed[k][4];

		}
	}

	closedNodes--;
}


bool chkClosedNode(int j, int i)
{
	bool ret = false;
	for(int k = 0; k < closedNodes ; k++)
	{
		if(closed[k][0] == j && closed[k][1] == i)
			ret = true;

	}

	return ret;

}



