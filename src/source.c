#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#define PORT 80
int main(int argc, char **argv)
{
	//Create Socket
	int status;
	int sockID = socket(AF_INET, SOCK_STREAM, 0);
	//
	if (sockID < 0)
	{
		printf("Create Socket Fail!!!\n");
		return -1;
	}
	else
	{
		printf("Create Socket Success\n");
	}
	//Remove http://
	char* DomainPtr=strstr(argv[1],":");
	char* Domain;
	if(DomainPtr==NULL)
	{
		Domain=argv[1];
	}
	else
	{
		Domain=DomainPtr+3;
	}
	//Host name& File Name
	char Request[]="";
	strcpy(Request,Domain);
	Domain=strtok(Domain,"/");//Domain Name
	char* RePtr;
	RePtr=strtok(Request,"/");
	RePtr=strtok(NULL,"/");
	char file[1024]="GET /";//File Name
	while(RePtr!=NULL)
	{
		strcat(file,RePtr);
		strcat(file,"/");
		RePtr=strtok(NULL,"/");
	}
	//Domain Name--->IP
	struct hostent* host = gethostbyname(Domain);
	if(host==NULL) { printf("Wrong URL\n"); return -1;}
	char* IP=(inet_ntoa (*((struct in_addr *) host->h_addr_list[0])));
	//
	struct sockaddr_in addrport;
	addrport.sin_family = AF_INET;
	addrport.sin_port = htons(PORT);
	addrport.sin_addr.s_addr = inet_addr(IP);
	printf("%s\n", IP);
	//Connect to Server
	if (status = connect(sockID, (struct sockaddr *) &addrport, sizeof(addrport)) < 0)
	{
		printf("Connect Failed!!!\n");
		return -1;
	}
	else
	{
		printf("Connect Successful\n");
	}
	//Create HTTP Request
	char buffer[1024] = "";
	char msg[1024] = " HTTP/1.1\r\nHost: ";
	strcat(msg,Domain);
	strcat(msg,"\r\n\r\n");
	strcat(file,msg);
	//Send HTTP Request
	printf("%s\n",file);
	int count;
	if (count = send(sockID, file, sizeof(msg), 0) < 0)
	{
		printf("Send Fail!!!\n");
		return -1;
	}
	else
	{
		printf("Sent\n");
	}
	//Receive HTTP Response
	if (count = recv(sockID, buffer, sizeof(buffer) - 1, 0) < 0)
	{
		printf("Receive Fail!!!\n");
		return -1;
	}
	else
	{
		printf("Received\n");
	}
	//Read Header: if Content-Length--->// if chunked--->
	char *p1, *p2;
	int flag = 0;
	char header[1024];
	strcpy(header, buffer);
	p1 = strtok(header, "\r\n");
	while(flag==0)
	{
		while (p1 != NULL)
		{
			if (strstr(p1, "Content-Length") != NULL)
			{
				flag = 1;
				break;
			}
			if (strstr(p1, "chunked") != NULL)
			{
				flag = 2;
				break;
			}
			p1 = strtok(NULL, "\r\n");
		}
		if(flag==0)
		{
			count = recv(sockID, buffer, sizeof(buffer) - 1, 0);
			strcpy(header, buffer);
			p1 = strtok(header, "\r\n");
		}
	}
	//Content-Length
	if (flag == 1)
	{	//Split Content-Length
		p1 = strtok(p1, " ");
		p1 = strtok(NULL, " ");
		int Csize = atoi(p1);
		printf("Content-Length: %d\n", Csize);
		//Point to Body
		p2 = strstr(buffer, "\r\n\r\n")+4;
		FILE *fp = fopen(argv[2], "w");
		if(Csize<512)
		{	
			char p3[512]="";
			strncpy(p3,p2,Csize);
			fprintf(fp, p3);
		}
		else
		{	
			fprintf(fp, p2);//Skip \r\n\r\n
		}
		int size = 0;
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		printf("Current File Size: %d\n", size);
		int i = Csize - size;//So bytes con lai cua Body
		//1023: Skip byte ? (00000002)
		if(i>0)
		{
			for (i; i > 0; i = i - 1023)
			{
				if (i > 1024)
				{	
					memset(buffer, 0, strlen(buffer));
					count = recv(sockID, buffer, sizeof(buffer)-1, 0);
					printf("1023\n", i);
					fprintf(fp, buffer);
				}
				else
				{
					memset(buffer, 0, strlen(buffer));
					count = recv(sockID, buffer, i, 0);
					printf("%d\n", i);
					fprintf(fp, buffer);
				}
		}
			}
		fclose(fp);
	}
	//chunked
	if (flag == 2)
	{
		while(1)//Find Body
		{

			p2 = strstr(buffer, "\r\n\r\n");
			if(p2!=NULL) { break;}
			memset(buffer, 0, strlen(buffer));
			count = recv(sockID, buffer, sizeof(buffer), 0);
		}
		FILE *fp = fopen(argv[2], "w");
		p2 = strstr(buffer, "\r\n\r\n")+4;//Skipp \r\n\r\n
		//
		int index=0, ChunkSize=0, end =0;
		char* chunk;
		while(1)
		{	
			chunk=strtok(p2,"\r\n");
			while(1)
			{
				if(chunk==NULL) {break;}
				if(index%2==0)
				{
					ChunkSize=(int)strtol(chunk, NULL, 16);
					if(ChunkSize==0) {end=1; break;}
					printf("Trailer: %s<=>%d\n",chunk,ChunkSize);
					index++;
				}
				else
				{
					if(chunk=="0") {end=1; break;}
					fprintf(fp, chunk);printf("Minus: %d\n",strlen(chunk));
					fprintf(fp,"\n");
					if(ChunkSize-strlen(chunk)==0)
					{
						index++;
					}
					else
					{
						ChunkSize=ChunkSize-strlen(chunk);printf("Remain: %d\n",ChunkSize);
					}
				}printf("*Next Token\n");
				chunk=strtok(NULL,"\r\n");
			}
			if(end==1) { printf("END!!!\n"); break;}
			memset(buffer, 0, strlen(buffer));
			count = recv(sockID, buffer, sizeof(buffer), 0);
			p2=buffer;printf("*Next Buffer\n");printf("%s\n",p2);
		}
	}
	//Close Connection
	if ((status = close(sockID)) < 0)
	{
		printf("ERROR WHEN CLOSING\n");
	}
	else
	{
		printf("\nCLOSED\n");
	}
	return 0;
}
