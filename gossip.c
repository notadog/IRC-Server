
/*
	Pretty much an IRC server.
	Done for a Data Transmission course
	version: 08/07/2015
*/

/*Bibliotecas de uso geral*/
#include <stdio.h>			/*entrada e saída de dados para o terminal*/
#include <string.h>			/*copiar strings de um lado pro outro*/
#include <stdlib.h>			/*atoi*/
#include <stdbool.h>			/*tipo bool*/
#include <time.h>			/*pega os timestamps*/
#include "Getch.c"			/*timed getchar(), se não responder antes do timeout retorna EOF */

/*Bibliotecas de rede*/
#include <arpa/inet.h>			/* in_addr e htons*/
#include <fcntl.h>			/*desbloquear o soquete*/

/*Constantes do programa*/
#define 	BUFFERSIZE	1024		
#define		PORTA_DEFAULT	10101		/*porta que eu prefiro usar*/
#define		PORTA_MINIMA	1000		/*li que o S.O. quer pegar as portas menores, nao sei se tenho que limitar aqui*/
#define		PORTA_MAXIMA	65535		/*2 bytes para endereço, não permito acesso à uma porta maior */
#define		LOCALHOST	"127.0.0.1"	/*LocalHost*/
#define		NUMCONEXOES	1		/*Quantidade de conversas em paralelo*/
#define		tickTime	10		/*De tick em tick dê uma olhada se tem algo no buffer*/

/*Comportamentos de cliente e servidor*/
void servidor (int porta);
void cliente (int porta);

/*Funções mistas*/
bool valida_cli (int argc, char **argv);
bool eh_servidor (char *string);
int porta_da_fofoca (int argc, char *string);
void relata_erros (int codigo);
struct sockaddr_in caracteristicas_endereco (int porta);

/*variáveis globais*/


int main(int argc, char *argv[]) {

    /*Se a cli não passar o teste de validade, nem execute*/
	if (!valida_cli(argc, argv))
		return 0;

	if (eh_servidor(argv[1])){
		printf ("sou servidor na porta %d\n", porta_da_fofoca(argc, argv[2]));

		servidor(porta_da_fofoca(argc, argv[2]));
	}
	else {
		printf ("sou cliente na porta %d\n", porta_da_fofoca(argc, argv[2]));

		cliente(porta_da_fofoca(argc, argv[2]));
	}

    return 0;
}

/*Informa se pela CLI foi informado se essa aplicação será cliente ou servidor*/
bool eh_servidor (char *string) {
	return !strcmp(string, "-S");
}

/*Retorna a porta informada pela CLI*/
int porta_da_fofoca (int argc, char *string) {

	if (argc == 2)
		return PORTA_DEFAULT;

	return atoi(string);
}


void servidor (int porta) {

	int listenfd = 0, soquete = 0, i=0, n=0;
	char sendBuffer[BUFFERSIZE], receiveBuffer[BUFFERSIZE], c;
	struct time *t;
	time_t now;
	struct sockaddr_in serv_addr;

	/*inicialização de memória*/
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuffer, '0', sizeof(sendBuffer));

	/*abre o socket*/
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(porta);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	/*espera um primeiro contato*/
	soquete = accept(listenfd, (struct sockaddr*)NULL, NULL);

	/*evita o bloqueio do socket*/
	if ((n = fcntl (soquete, F_GETFL)) < 0 || fcntl (soquete, F_SETFL, n | O_NONBLOCK) < 0) {
		printf ("Problemas no desbloqueio do meio\n.");
		return;
	}

	while (1) {

		/*tenta pegar um caractere do usuário*/
		if ( (c=tgetche(1)) != EOF ){
			sendBuffer[i++] = c;

			/*suporte à backspace*/
			if (c == '\b' && i>0)
				sendBuffer[--i] = '\0';

			/*enter envia a mensagem*/
			if (c == '\n'){
				/*envia a mensagem*/
				sendBuffer[i] = '\0';
				write(soquete, sendBuffer, strlen(sendBuffer)+1);
				sleep(1);

				/*prepara para um novo envio*/
				memset(sendBuffer, 0,sizeof(sendBuffer));
				i=0;
			}
		}

		/*olha se tem algo pra ler no buffer*/
		if (n=read(soquete, receiveBuffer, sizeof(receiveBuffer)) > 0) {
			now = time(NULL);			
			t = localtime(now);		
			printf ("%s > " asctime(t));
			printf ("%s", receiveBuffer);

			/*se receber o oi, responda*/
			if (!strcmp("HELLO SRV\n", receiveBuffer)){
				strcpy(sendBuffer, "HELLO CLT\n\0");
				write(soquete, sendBuffer, strlen(sendBuffer)+1);
			}

			/*se receber o tchau, feche*/
			if (!strcmp("BYE SRV\n", receiveBuffer)){
				strcpy(sendBuffer, "BYE CLT\n\0");
				write(soquete, sendBuffer, strlen(sendBuffer)+1);
				close(soquete);
				return;
			}
		}
	}
}

void cliente (int porta){
    int soquete = 0, n = 0;
    char receiveBuffer[BUFFERSIZE], sendBuffer[BUFFERSIZE], c, i=0;
    struct sockaddr_in serv_addr;
	struct tm *t;
	time_t now;

    memset(receiveBuffer, '\0',sizeof(receiveBuffer));

    if((soquete = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return;
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta);

    if(inet_pton(AF_INET, LOCALHOST, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return;
    }

	if( connect(soquete, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error : Connect Failed \n");
		return;
	}

	/*evita o bloqueio do socket*/
	if ((n = fcntl (soquete, F_GETFL)) < 0 || fcntl (soquete, F_SETFL, n | O_NONBLOCK) < 0) {
		printf ("Problemas no desbloqueio do meio\n.");
		return;
	}

	/*manda um oi*/
	strcpy(sendBuffer, "HELLO SRV\n\0");
	write(soquete, sendBuffer, 1+strlen(sendBuffer));

	while (true) {

		/*tenta pegar um caractere do usuário*/
		if ( (c=tgetche(1)) != EOF ){
			sendBuffer[i++] = c;

			/*suporte à backspace*/
			if (c == '\b')
				sendBuffer[i--] = '\0';

			/*enter envia a mensagem*/
			if (c == '\n'){
				/*envia a mensagem*/
				sendBuffer[i] = '\0';
				write(soquete, sendBuffer, 1+strlen(sendBuffer));
				sleep(1);

				/*prepara para um novo envio*/
				memset(sendBuffer, 0,sizeof(sendBuffer));
				i=0;
			}
		}

		/*olha se tem algo pra ler no buffer*/
		if (n=read(soquete, receiveBuffer, sizeof(receiveBuffer)) > 0) {
			now = time(NULL);			
			*t = localtime(now);		
			printf ("%s > " asctime(t));

			/*se receber o tchau, é hora de dar tchau*/
			if (!strcmp("BYE CLT\n", receiveBuffer)){
				strcpy(sendBuffer, "BYE SRV\n\0");
				write(soquete, sendBuffer, 1+strlen(sendBuffer));
				return;
			}
		}
	}
}

/*Validação da cli: command line interface. True se tudo certo e False se deu algum problema*/
bool valida_cli (int argc, char **argv) {
	int temp;

	/*Problemas de quantidade de argumento*/
	if(argc<2 || argc>4)
		relata_erros(1);

	/*O primeiro argumento tem que ser ou 'host' ou 'server'.*/
	if ( strcmp(argv[1],"-host") && strcmp(argv[1],"host") && strcmp(argv[1],"-h") &&
		strcmp(argv[1],"-server") && strcmp(argv[1],"server") && strcmp(argv[1],"-s") )
		relata_erros(2);

	/*O segundo argumento eh opcional à priori vou usar ports de 1000 ateh 65k*/
	if (argc>2) {
		temp = atoi(argv[2]);
		if (temp <= PORTA_MINIMA || temp >= PORTA_MAXIMA) {
			relata_erros(3);
			return false;
		}
	}

	return true;
}

void relata_erros (int codigo) {

	switch (codigo){

		case 1:
			fprintf (stderr, "O programa funciona com dois/tres argumentos:\n");
			fprintf (stderr, " > 'server' ou 'host' definindo servidor/cliente [obrigatorio]\n");
			fprintf (stderr, " > Enredeco IP\n");
			fprintf (stderr, " > Porta [opcional]\n");
			break;

		case 2:
			fprintf (stderr, "O primeiro argumento tem que ser '-S' ou host.\n");
			break;

		case 3:
			fprintf (stderr, "A porta de acesso tem que ser um numero positivo nao nulo. Pelo menos eu acho.\n");
			break;

		default:
			;
	}
	exit(codigo);
}
