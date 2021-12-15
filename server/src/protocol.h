/*
 * protocol.h
 *
 *  Created on: 14 dic 2021
 *      Author: Rodolfo Pio Sassone
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define RESULT_SIZE 100

typedef struct
{
	char operator;
	int num1;
	int num2;
	char result[RESULT_SIZE];
}message;


#endif /* PROTOCOL_H_ */
