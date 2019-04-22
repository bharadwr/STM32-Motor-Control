#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdlib.h>
#include <string.h>
uint32_t dist_sensor1 = 0;
uint32_t dist_sensor2 = 0;
int count = 0;
int distance = 0;
int high_low = 0;

#define COL1 (1 << 0)
#define COL2 (1 << 1)
#define COL3 (1 << 2)
#define COL4 (1 << 3)

#define ROW1 (1 << 4)
#define ROW2 (1 << 5)
#define ROW3 (1 << 6)
#define ROW4 (1 << 7)

#define SAMPLE_TIME_MS 10
#define SAMPLE_COUNT (SAMPLE_TIME_MS)

#define THRESHOLD_TIME_MS 2
#define THRESHOLD (THRESHOLD_TIME_MS)

#define KEY_PRESS_MASK  0b11000111
#define KEY_REL_MASK    0b11100011

void init_lcd(void);
void display1(const char *);
void display2(const char *);
float travelled_distance;
int col = 0;
int row = -1;
int red = 0, blue = 0, grn = 0;
int runflag = 0;
int speed = 0;
int fb = 0;
int lr = 0;
int test_dist;
char char_array[4][4] = { {'1', '2', '3', 'A'},
		{'4', '5', '6', 'B'},
		{'7', '8', '9', 'C'},
		{'*', '0', '#', 'D'} };

int int_array[4][4] =   { {1,2,3,0xa},
		{4,5,6,0xb},
		{7,8,9,0xc},
		{0xf, 0, 0xe, 0xd} };

uint8_t key_samples[4][4]  = { {0}, {0}, {0}, {0} };
uint8_t key_pressed[4][4]  = { {0}, {0}, {0}, {0} };
uint8_t key_released[4][4]  = { {0}, {0}, {0}, {0} };

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

//CODE FROM SENSORS
void ultrasonic_setup_10_11(){
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

	//change to 8 and 9

	//OUTPUT MODE FOR GPIOC           //TRIG PC11
	GPIOC->MODER &= ~(0X3 << (11 * 2)); //ORGINIALLY PB5
	GPIOC->MODER |=  (0X1 << (11 * 2));
	//PUSH-PULL PUSH PULL
	GPIOC->OTYPER &= ~(0X1 << 11);
	//OSPEEDR
	GPIOC->OSPEEDR |= (0X3 << (11 * 2));


	//INPUT MODE FOR GPIOA             //ECHO PC10
	GPIOC->MODER &= ~(0X3 << (10 * 2)); //ORGINALLY PA3
	//A PUPDR
	GPIOC->PUPDR &= ~(0X3 << (10 * 2));
	GPIOC->OSPEEDR |= (0X3 << (10 * 2));
}

//PC3 AND PC4 FOR ULTRA34
void ultrasonic_10_11(){

	unsigned char i;
	unsigned long int j;
	float Ultr_Temp = 0;
	nano_wait(100000000);
	for (i = 0; i < 5; i++)
	{
		GPIOC->BSRR |= GPIO_BSRR_BS_11;
		//DELAY *
		nano_wait(100000);

		GPIOC->BSRR |= GPIO_BSRR_BR_11;

		while((GPIOC->IDR & GPIO_IDR_10) == 0);

		while(GPIOC->IDR & GPIO_IDR_10) {
			//DELAY **
			nano_wait(8000);
			j++;
		}

		Ultr_Temp += 340/2*j*10;
		j=0;
		//DELAY *
		nano_wait(10000);

	}

	int dist = Ultr_Temp/50000;
	dist_sensor1 = dist;
	char buf[20];
	itoa(dist, buf, 10);
	//display2(buf);

	if (dist >= 0){
		//display1(buf);
	}

}

void hall_setup12(){
	//RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

	//change to 8 and 9

	//OUTPUT MODE FOR GPIOC
	GPIOC->MODER &= ~(0X3 << (12 * 2));
	GPIOC->MODER |=  (0x0 << (12 * 2));
	//PUSH-PULL PUSH PULL
	GPIOC->OTYPER &= ~(0X1 << 12);
	//OSPEEDR
	GPIOC->OSPEEDR |= (0X3 << (12 * 2));
	GPIOC->PUPDR &= ~(0X3 << (12 * 2));
	GPIOC->PUPDR |= (0X1 << (12 * 2));
}

void TIM15_IRQHandler(void) {
	TIM15->SR = ~TIM_SR_UIF;
	//display1("Here");
	int circumference = 20; //cm
	char snum[16];
	//display2("testing");

	if (GPIOC->IDR & GPIO_IDR_12) {
		//display1("High");
		high_low = 0;
	}
	else {
		//display1("Low");
		if (high_low == 0)
		{
			travelled_distance += circumference/5;
			if (travelled_distance >= test_dist) {
				runflag = 1;
				TIM15->CR1 &= ~TIM_CR1_CEN;
			}
		}
		high_low = 1;
		//display2("Low");
	}
	itoa(travelled_distance, snum, 10);
	display2(snum);
	ultrasonic_10_11();

	count += 1;

	if ((dist_sensor1 >= 2000 | dist_sensor1 <= 20) & count > 5) {
		display1("Run obstructed");
		display2("Press reset");
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		exit(0);
	}
}

void tim15_init(void){
	RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
	TIM15->PSC = 4800 - 1;
	TIM15->ARR = 100 - 1;
	TIM15->DIER |= TIM_DIER_UIE;
	NVIC->ISER[0] = 1<<TIM15_IRQn;
	TIM15->CR1 |= TIM_CR1_CEN;
}

void F(int distance)
{
	TIM1->CCR1 = 42 + 10 * speed;
	TIM1->CCR2 = 40 + 10 * speed;
	GPIOA->BSRR |= 1<<4 | 1 << 6;
	GPIOA->BRR |= 1<<5 | 1<<7 ;
	tim15_init();
	test_dist = distance;
	runflag = 0;
	travelled_distance = 0;
	while(!runflag);
	TIM15->CR1 &= ~TIM_CR1_CEN;
	runflag = 0;
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void B(int distance)
{
	TIM1->CCR1 = 40 + 10 * speed;
	TIM1->CCR2 = 40 + 10 * speed;
	GPIOA->BSRR |= 1<<5 | 1<<7;
	GPIOA->BRR |= 1<<4 | 1<<6;
	tim15_init();
	test_dist = distance;
	runflag = 0;
	travelled_distance = 0;
	while(!runflag);
	TIM15->CR1 &= ~TIM_CR1_CEN;
	runflag = 0;
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void L(void)
{
	TIM1->CCR1 = 40;
	TIM1->CCR2 = 40;

	GPIOA->BSRR |= 1<<5 | 1<<6;
	GPIOA->BRR |= 1<<4 | 1<<7;

	for(int i=0; i<5; i++)
		nano_wait(237*1000*1000);

	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void R(void)
{
	TIM1->CCR1 = 40;
	TIM1->CCR2 = 40;

	GPIOA->BSRR |= 1<<4 | 1<<7;
	GPIOA->BRR |= 1<<5 | 1<<6;
	for(int i=0; i<5; i++)
		nano_wait(237*1000*1000);
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	GPIOA->BRR |= 1<<4 | 1<<5 | 1<<6 | 1<<7;
}

void update_key_press() {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if ((key_samples[i][j] & KEY_PRESS_MASK) == 0b00000111) {
				key_pressed[i][j] = 1;
				key_samples[i][j] = 0xFF;
			}

			if ((key_samples[i][j] & KEY_REL_MASK) == 0b11100000) {
				key_released[i][j] = 1;
				key_samples[i][j] = 0x00;
			}
		}
	}
}

char get_char_key() {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(key_released[i][j] == 1 && key_pressed[i][j] == 1) {
				key_released[i][j] = 0;
				key_pressed[i][j] = 0;
				return char_array[i][j];
			}
		}
	}

	return '\0';
}

int get_key_pressed() {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(key_released[i][j] == 1 && key_pressed[i][j] == 1) {
				key_released[i][j] = 0;
				key_pressed[i][j] = 0;
				return int_array[i][j];
			}
		}
	}

	return -1;
}

void update_samples(int row) {
	// Update the current column with new values
	for(int i = 0; i < 4; i++) {
		uint8_t curr_history = key_samples[i][col];
		curr_history = curr_history << 1;

		if(row == i)
			curr_history |= 1;

		key_samples[i][col] = curr_history;
	}
}

void get_coordinates() {
	int freq = 0;
	int pos = 3;
	char line1[16] = "X: _, Y: _ ft";
	display1(line1);
	while(pos < 4) {
		char key = get_char_key();
		if(key != '\0') {
			if (key == 'D') {
				if (line1[2] != '-')
					line1[2] = '-';
				else
					line1[2] = ' ';
			}
			else {
				switch(key) {
				case '0' : line1[pos] = key; pos++; break;
				case '1' : line1[pos] = key; pos++; break;
				case '2' : line1[pos] = key; pos++; break;
				case '3' : line1[pos] = key; pos++; break;
				case '4' : line1[pos] = key; pos++; break;
				case '5' : line1[pos] = key; pos++; break;
				case '6' : line1[pos] = key; pos++; break;
				case '7' : line1[pos] = key; pos++; break;
				case '8' : line1[pos] = key; pos++; break;
				case '9' : line1[pos] = key; pos++; break;
				}
			}
		}
		display1(line1);
	}
	pos = 9;
	while(1 && pos < 10) {
		char key = get_char_key();
		if(key != '\0') {
			if (key == 'D') {
				if (line1[9] != '-')
					line1[9] = '-';
				else
					line1[9] = '+';
			}
			else {
				switch(key) {
				case '0' : line1[pos] = key; pos++; break;
				case '1' : line1[pos] = key; pos++; break;
				case '2' : line1[pos] = key; pos++; break;
				case '3' : line1[pos] = key; pos++; break;
				case '4' : line1[pos] = key; pos++; break;
				case '5' : line1[pos] = key; pos++; break;
				case '6' : line1[pos] = key; pos++; break;
				case '7' : line1[pos] = key; pos++; break;
				case '8' : line1[pos] = key; pos++; break;
				case '9' : line1[pos] = key; pos++; break;
				}
			}
		}
		display1(line1);
	}
	fb = 10 * (line1[3] - '0');
	lr = 10 * (line1[9] - '0');
	if (line1[2] == '-')
		fb *= -1;
	if (line1[9] == '-')
		lr *= -1;
}

void get_speed() {
	int freq = 0;
	int pos = 7;
	char line2[16] = "Speed: _ (1 - 3)";
	display2(line2);
	while(pos < 8) {
		char key = get_char_key();
		if(key != '\0') {
			switch(key) {
			case '1' : line2[pos] = key; pos++; break;
			case '2' : line2[pos] = key; pos++; break;
			case '3' : line2[pos] = key; pos++; break;
			}
		}
		display2(line2);
	}
	speed = line2[7] - '0';
}

// This function should enable the clock to port A, configure pins 0, 1, 2 and
// 3 as outputs (we will use these to drive the columns of the keypad).
// Configure pins 4, 5, 6 and 7 to have a pull down resistor
// (these four pins connected to the rows will being scanned
// to determine the row of a button press).
void init_keypad() {
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->MODER &= ~(0b11111111);
	GPIOB->MODER |= 0b01010101;
	GPIOB->MODER &= ~(3 << 8);
	//GPIOB->MODER |= 2 << 8;
	GPIOB->MODER &= ~(3 << 10);
	//GPIOB->MODER |= 2 << 10;
	GPIOB->MODER &= ~(3 << 12);
	//GPIOB->MODER |= 2 << 12;
	GPIOB->MODER &= ~(3 << 14);
	//GPIOB->MODER |= 2 << 14;
	GPIOB->PUPDR &= ~(3 << 8);
	GPIOB->PUPDR |= 2 << 8;
	GPIOB->PUPDR &= ~(3 << 10);
	GPIOB->PUPDR |= 2 << 10;
	GPIOB->PUPDR &= ~(3 << 12);
	GPIOB->PUPDR |= 2 << 12;
	GPIOB->PUPDR &= ~(3 << 14);
	GPIOB->PUPDR |= 2 << 14;
}

// This function should enable clock to timer 3, setup the timer
// so that it triggers TIM3_IRQHandler every 1ms.
void setup_timer3() {
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM3->PSC = 480 - 1;
	TIM3->ARR = 100 - 1;
	TIM3->DIER |= TIM_DIER_UIE; //set update interrupt bit
	TIM3->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] = 1 << TIM3_IRQn;

}

// Use the global variable â€˜colâ€™ provided in the skeleton file,
// increment it by 1. Check if â€˜colâ€™ exceeds 3, if so, reset it to 0.
// This is because we have 4 columns on the keypad and â€˜colâ€™ dictates which
// column is currently being driven. Set the output data register
// of the port A to (1 << col). This drives the corresponding column to â€˜logic 1â€™.
void TIM3_IRQHandler()
{
	int current_row = -1;
	//int why_tho = GPIOB->IDR;

	if((GPIOB->IDR & (1<<4)) == (1<<4))
	{
		current_row = 0;
	}
	if((GPIOB->IDR & (1<<5)) == (1<<5))
	{
		current_row = 1;
	}
	if((GPIOB->IDR & (1<<6)) == (1<<6))
	{
		current_row = 2;
	}
	if((GPIOB->IDR & (1<<7)) == (1<<7))
	{
		current_row = 3;
	}
	update_samples(current_row);
	update_key_press();
	col++;
	if(col > 3)
	{
		col = 0;
	}

	GPIOB->ODR = 1 << col;
	TIM3->SR &= ~TIM_SR_UIF;

}

void run(void)
{
	init_lcd();
	init_keypad();
	setup_timer3();
	display1("* for XY");
	display2("# for speed");

	char indicator = get_char_key();
	int complete1 = 1;
	int complete2 = 1;
	while(complete1 || complete2) {
		// Complete the code here in this loop...
		indicator = get_char_key();
		if (indicator ==  '*')
		{
			get_coordinates();
			complete1 = 0;
		}
		if (indicator ==  '#')
		{
			get_speed();
			complete2 = 0;
		}
	}
	nano_wait(1000000000);
	init_pwm1();
	init_direcX();
	initPA0();
	ultrasonic_setup_10_11();
	hall_setup12();
	display1("Starting.");
	display2("");
	nano_wait(1000000000);
	display1("Starting..");
	nano_wait(1000000000);
	display1("Starting...");
	nano_wait(1000000000);
	char snum[5];
	if (fb > 0) {
		itoa(fb, snum, 10);
		display1("Forward");
		display2(snum);
		F(fb);
	}
	else if (fb < 0) {
		itoa(fb, snum, 10);
		display1("Backward");
		display2(snum);
		R();
		nano_wait(1000000000);
		R();
		nano_wait(1000000000);
		F(-fb);
	}
	nano_wait(10000000000);
	if (lr < 0) {
		itoa(lr, snum, 10);
		display1("Left");
		display2(snum);
		L();
		nano_wait(1000000000);
		if (lr != 0)
			F(-lr);
	}
	else if (lr > 0) {
		itoa(lr, snum, 10);
		display1("Right");
		display2(snum);
		R();
		nano_wait(1000000000);
		if (lr != 0)
			F(lr);
	}
	nano_wait(10000000000);
	testLED();
	display1("Run Finished");
	display2("Press reset");
}

int main(void) {
	run();
}
