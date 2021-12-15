/*
 ============================================================================
 Name        : server.c
 Author      : Rodolfo Pio Sassone
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <string.h>
#include "protocol.h"

#define TRUE 1
#define NAME_DIM 512

#define NO_ERROR 0
void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

void add(message* msg);

void mult(message* msg);

void sub(message* msg);

void division(message* msg);


int main(int argc, char *argv[]) {

	puts("Avviato");

#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif
	char hostname[NAME_DIM] = {"localhost"};
	int port = 60000;

	if(argc == 2)
	{	//Tokenization
		char separator[2] = ":";
		char* t;
		char* token[2];
		int i=0;

		t = strtok(argv[1], separator);

		while(t != NULL && i < 2)
		{
			token[i]=t;
			i++;
			t=strtok(NULL, separator);
		}

		if (i!=2)
		{
			puts("***Error: Indirizzo non valido***");
			puts("Il programma verra' aperto con le impostazioni di default (localhost:60000)");
		}
		else
		{
			strcpy(hostname, token[0]);
			port = atoi(token[1]);
		}
	}

	struct hostent* h = gethostbyname(hostname);
	if(h == NULL)
	{
		puts("***Error: gethostbyname()***");
		clearwinsock();
		return -1;
	}
	struct in_addr* addr = (struct in_addr*) h->h_addr;


	int my_socket;
	my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(my_socket<0)
	{
		printf("***Error: creation socket***\n");
		return -1;
	}

	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family=AF_INET;
	sad.sin_addr = *addr;
	sad.sin_port = htons(port);
	int binded = bind(my_socket,(struct sockaddr*) &sad, sizeof(sad));
	if(binded<0)
	{
		printf("***Error: binding***\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	message msg;
	struct sockaddr_in cad;

	while(TRUE)
	{
		int cadSize = sizeof(cad);

		memset(&cad, 0, cadSize);
		memset(&msg, 0, sizeof(msg));
		recvfrom(my_socket, (char*) &msg, sizeof(msg), 0, (struct sockaddr*) &cad, &cadSize);

		msg.num1 = ntohl(msg.num1);
		msg.num2 = ntohl(msg.num2);

		struct hostent* c = gethostbyaddr((char*) &cad.sin_addr, 4, AF_INET);
		if(c == NULL)
		{
			puts("***Error: gethostbyaddr()***");
			closesocket(my_socket);
			clearwinsock();
			return -1;
		}
		char* serverName = c->h_name;
		struct in_addr* serverAddr = (struct in_addr*) c->h_addr;

		printf("Richiesta operazione \"%c %d %d\" dal client %s, ip %s\n", msg.operator, msg.num1, msg.num2, serverName, inet_ntoa(*serverAddr));

		switch (msg.operator)
		{
		case '+':
			add(&msg);
			break;
		case '*':
		case 'x':
			mult(&msg);
			break;
		case '-':
			sub(&msg);
			break;
		case '/':
			division(&msg);
			break;
		default:
			break;
		}

		msg.num1 = htonl(msg.num1);
		msg.num2 = htonl(msg.num2);

		int sended = sendto(my_socket, (char*) &msg, sizeof(msg), 0, (struct sockaddr*) &cad, sizeof(cad));
		if (sended != sizeof(msg))
		{
			printf("***Error: Invio non riuscito correttamente***\n");
			break;
		}
	}

	closesocket(my_socket);
	clearwinsock();
	return 0;
}

void add(message* msg)
{
	int result = msg->num1 + msg->num2;
	sprintf(msg->result, "%d", result);
}

void mult(message* msg)
{
	int result =  msg->num1 * msg->num2;
	sprintf(msg->result, "%d", result);
}

void sub(message* msg)
{
	int result =  msg->num1 - msg->num2;
	sprintf(msg->result, "%d", result);
}

void division(message* msg)
{
	float result =  (float) msg->num1/msg->num2;
	sprintf(msg->result, "%f", result);
}


