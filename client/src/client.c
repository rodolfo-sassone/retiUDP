/*
 ============================================================================
 Name        : client.c
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
#include "protocol.h"

#define MAX_INPUT 512

#define NO_ERROR 0
void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}


void errorHandler(char* e);


void parser(char* input, short* run, short* s, char** token);


int main(int argc, char *argv[])
{

#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		errorHandler("Error at WSAStartup()");
		return 0;
	}
#endif
	char hostname[512] = {"localhost"};		//TODO string's dimension
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
			errorHandler("Indirizzo non valido");
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
		errorHandler("gethostbyname()");
		clearwinsock();
		return -1;
	}
	struct in_addr* addr = (struct in_addr*) h->h_addr;

	int my_socket;
	my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(my_socket<0)
	{
		errorHandler("Creation socket");
		return -1;
	}

	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr = *addr;
	sad.sin_port = htons(port);

	puts("Questa calcolatrice lavora solo con i numeri interi.\n"
			"Scrivi le operazioni in notazione prefissa (es. x 4 2 per 4x2).\n"
			"Per uscire digita =\n");

	short run = 1;
	message msg;
	while(run)
	{
		short status = 1;		//flag: 1 if all right, 0 otherwise
		char input[MAX_INPUT]={""};
		char* token[3];
		puts("Scrivi operazione: ");
		gets(input);

		parser(input, &run, &status, token);

		msg.operator = *token[0];
		msg.num1 = 0;
		msg.num2 = 0;
		msg.result = 0;

		if(status)
		{
			msg.num1 = htonl(atoi(token[1]));
			msg.num2 = htonl(atoi(token[2]));
			msg.result = htonl(msg.result);

			int sended = sendto(my_socket, (char*) &msg, sizeof(msg), 0, (struct sockaddr*) &sad, sizeof(sad));
			if(sended != sizeof(msg))
			{	//TODO vedi un po' se chiudere tutto o no
				errorHandler("Messaggio non inviato correttamente");
				closesocket(my_socket);
				clearwinsock();
				return -1;
			}


			struct sockaddr_in fromAddr;
			int fromAddrSize = sizeof(fromAddr);
			recvfrom(my_socket,(char*) &msg, sizeof(msg), 0, (struct sockaddr*) &fromAddr, &fromAddrSize);
			if(fromAddr.sin_addr.s_addr != sad.sin_addr.s_addr)
			{
				errorHandler("Ricevuto pacchetto da sorgente sconosciuta");
				closesocket(my_socket);
				clearwinsock();
				return -1;
			}

			msg.num1 = ntohl(msg.num1);
			msg.num2 = ntohl(msg.num2);
			msg.result = ntohl(msg.result);

			memset(h,0,sizeof(*h));
			h = gethostbyaddr((char*) &fromAddr.sin_addr, 4, AF_INET);
			if(h == NULL)
			{
				puts("***Error: gethostbyaddr()***");
				closesocket(my_socket);
				clearwinsock();
				return -1;
			}
			char* serverName = h->h_name;
			struct in_addr* serverAddr = (struct in_addr*) h->h_addr;

			printf("Ricevuto risultato dal server %s, ip %s :\n %d %c %d = %f", serverName, inet_ntoa(*serverAddr), msg.num1, msg.operator, msg.num2, msg.result);

		}

	}

	closesocket(my_socket);
	clearwinsock();
	return 0;
}


void parser(char* input, short* run, short* s, char** token)
{
	char separator = ' ';
	char* t;
	int i=0;

	//Tokenization
	t = strtok(input, &separator);

	while(t != NULL && i < 3)
	{
		token[i]=t;
		i++;
		t=strtok(NULL, &separator);
	}

	//Parsing
	if(i != 3)
	{
		if(*token[0] == '=')
		{
			*run = 0;
			*s = 0;
		}
		else
		{
			puts("***Operazione non valida o sintassi errata***\n");
			*s = 0;
		}
	}
	else
	{
		if(strlen(token[0])!=1)
		{
			puts("***Operazione non valida: operatore non valido***\n");
			*s=0;
		}
		else
		{	//Control operator
			if(*token[0] == '+' || *token[0] == '-' || *token[0] == 'x' || *token[0] == '/' || *token[0] == '*')
			{
				int j=0;
				short isANum1 = 1;
				short isANum2 = 1;
				//Control if token1 are digit
				for(j=0;j<strlen(token[1]);j++)
				{
					if(!isdigit(token[1][j]))
						isANum1 = 0;
				}
				if(isANum1)
				{	//Control if token2 are digit
					for(j=0;j<strlen(token[2]);j++)
					{
						if(!isdigit(token[2][j]))
							isANum2 = 0;
					}
					if(isANum2)
					{	//Control division by 0
						if(*token[0] == '/' && atoi(token[2]) == 0)
						{
							puts("***Operazione non valida: impossibile dividere per 0***\n");
							*s = 0;
						}
					}
					else
					{
						puts("***Operazione non valida: operando 2 non valido***\n");
						*s = 0;
					}
				}
				else
				{
					puts("***Operazione non valida: operando 1 non valido***\n");
					*s = 0;
				}

			}
			else
			{
				puts("***Operazione non valida: operatore non valido***\n");
				*s = 0;
			}
		}
	}
}


void errorHandler(char* e)
{
	puts("***Error: %s***");
}
