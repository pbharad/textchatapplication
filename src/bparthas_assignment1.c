/**
 * @bparthas_assignment1
 * @author  Bharadwaj Parthasarathy <bparthas@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/global.h"
#include "../include/logger.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>
#define STDIN 0
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int port_number;
struct clientlist{
	int id;
	int port;
	char *ip_address[20];
	char *hostname[100];
	int socket;
	int is_logged;
	char *blocked_list[100];
	char *buffer_msg[1024];

};
fd_set readfs;  	
int client_count = 0;
int client_count_store = 0;
char *server_ip[20];
int server_port;
struct clientlist client_list[5];
struct clientlist client_list_store[5];
int sock_flag = 0;	
bool validate(char *ip, char*port);
int max_fd;
int compare(const void *s1, const void *s2)
    {
      struct clientlist *e1 = (struct clientlist *)s1;
      struct clientlist *e2 = (struct clientlist *)s2;
      return e1->port > e2->port;
    }
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));
	
	/*Start Here*/
	if(argv[2] != NULL)
	{
		port_number = atoi(argv[2]);
	}
	if(strcmp(argv[1],"c") == 0)
	{
		run_as_client(argv[2]);
	}
	else if(strcmp(argv[1],"s") == 0)
	{
		run_as_server(argv[2]);
	}
	return 0;
}

// server implementation
 
int run_as_server(char *port)
{
	struct sockaddr_storage client_addr;
	int server_socket;
	int client_socket;
	struct sockaddr_in address;
	int addrlen;
	server_port = atoi(port);	
	if((server_socket = socket(AF_INET,SOCK_STREAM,0)) == 0)
	{
		//printf("ABError \r \n");
		return;
 	}
	
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(port));
	server_address.sin_addr.s_addr = INADDR_ANY;
	addrlen = sizeof(address);
	
	//bind to configured ip and port
	
	if(bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		//printf("Error binding \r \n");
		return;
	}
	
	if(listen(server_socket,5) < 0)
	{
		//printf("Listening Error \r \n");
		return;
	}
	
	while(1)
	{
		max_fd = server_socket;
		char command[100];
		FD_ZERO(&readfs);
		FD_SET(server_socket,&readfs);
		FD_SET(STDIN,&readfs);
		for(int i=0;i<client_count;i++)
		{
			FD_SET(client_list[i].socket,&readfs);
			if(client_list[i].socket > max_fd)
			{
				max_fd = client_list[i].socket;
			}
		}
		int activity = select(max_fd +1,&readfs,NULL,NULL,NULL);
		if(activity < 0)
		{
			//printf("ADError \r \n");
		}
		if(FD_ISSET(STDIN, &readfs))
		{
			handleCommand();  
		}
		if(FD_ISSET(server_socket, &readfs))
		{
			if ((client_socket = accept(server_socket,(struct sockaddr_in *)&address, (socklen_t*)&addrlen)) == 0)
			{
             
			//	printf("ACErrori \r \n");
				return;
			}
			//printf("SOCKET number : %d \r \n",client_socket);
			char host[100];
			char service[120];		
			int st = getnameinfo((struct sockaddr*)&address, sizeof address, host, sizeof host, service, sizeof service, 0);
			push_to_clientlist(&address,host,client_socket);
			socklen_t len;
			char ipstr[INET6_ADDRSTRLEN];
			int port;
			len = sizeof client_addr;
			struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
			getpeername(client_socket, (struct sockaddr*)&client_addr, &len);

			char *message[100];
			memset(message,0,100);
			strcpy(message,"LOGIN##");
			processMessage(message,client_socket,"");	
			///int as = send(client_socket,(char *)&client_list[0],sizeof(client_list)+1,0);
		}
		for(int i=0;i<client_count;i++)
		{	//printf("JJJJJJJ \r \n");
			if(FD_ISSET(client_list[i].socket,&readfs))
			{
				char *message[1024];
				memset(message,0,1024);
				recv(client_list[i].socket,message,sizeof(message),0);
				processMessage(message,client_list[i].socket,client_list[i].ip_address);	
			}
		}
	}
	close(server_socket);
	return 0;
}


void processMessage(char *message[1024], int socket, char *ip_address[20])
{
	char *tokenPtr;
        tokenPtr = strtok(message, "##");//strcpy(tokenPtr,strtok(message, "##"));
        int i=0;
        char *action[10];
        while(tokenPtr != NULL)
        {
                action[i] = tokenPtr;//strcpy(action[i],tokenPtr);
                tokenPtr = strtok(NULL, "##");//strcpy(tokenPtr,strtok(NULL, "##"));
                i++;
        }
	if(strcmp(action[0],"REFRESH") == 0 || strcmp(action[0], "PORT") == 0){
		char *list_msg[1024];
		memset(list_msg,0,1024);
		if(strcmp(action[0],"PORT") == 0){
			sprintf(list_msg,"LOGIN__");
		}else{
		sprintf(list_msg,"%s__",action[0]);}
		qsort(client_list,client_count, sizeof(struct clientlist), compare);
		for(int i=0;i<client_count;i++)
		{
			if(client_list[i].is_logged == 1)
			{
				char *msg[500];
				memset(msg,0,500);	
				int port;
				if(strcmp(action[1],client_list[i].hostname) == 0)
				{
					port = atoi(action[2]);
					client_list[i].port = port;

				}else{port = client_list[i].port;}
				sprintf(msg, "%s##%s##%d##%d##%d",client_list[i].hostname,client_list[i].ip_address,port,client_list[i].socket,client_list[i].is_logged);
				if(client_count > 1){
					strcat(msg,"__");
				}
				strcat(list_msg,msg);
			}
		}
		//cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
		int len = strlen(list_msg);
		sendall(socket,list_msg,&len);
	}
	if(strcmp(action[0], "SEND") == 0)
	{
		char *msg[1024];
		char *buffer_msg[1024];
		int dest_client;
		dest_client = findDestinationSocket(action[1]);
		sprintf(msg, "SEND__%s##%s##%s",ip_address,action[1],action[2]);
		int len = strlen(msg);
		for(int i=0;i<client_count;i++)
		{
			if(strcmp(client_list[i].ip_address,ip_address) == 0)
			{


				if(client_list[i].is_logged == 0)
				{
					sprintf(buffer_msg,"%s##",action[2]);	
				//	printf("GHGHGHGHG %s \r \n",buffer_msg);
					strcat(client_list[i].buffer_msg,buffer_msg);
				}
				else
				{
					sendall(dest_client,msg,&len);
					cse4589_print_and_log("[RELAYED:SUCCESS]\n");
					cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_address,action[1], action[2]);	
        				cse4589_print_and_log("[RELAYED:END]\n");

				}
			/*	if(isStringPresent(client_list[i].blocked_list,action[1],","))
				{
					printf("THe client has blocked \r \n");
				}
				else	
				{*/
					/*sendall(dest_client,msg,&len);
					cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_address,action[1], action[2]);	

				}*/
			}
			
		}
	}
	if(strcmp(action[0], "LOGOUT") == 0){
		char *host[50];
		memset(host,0,50);
		strcpy(host,action[1]);
		for(int i=0;i<client_count;i++)
		{	
			if(strcmp(action[1],client_list[i].hostname) == 0)
			{
				client_list[i].is_logged = 0;
			}
		}
	}
	if(strcmp(action[0], "EXIT") == 0){
		for(int i=0;i<client_count;i++)
		{	
			if(strcmp(action[1],client_list[i].hostname) == 0)
			{
				client_count--;
				memset(&client_list[i],0,sizeof (struct clientlist));
			}
		}
		
	}
	if(strcmp(action[0],"BROADCAST") == 0)
	{	char *msg[1024];
		memset(msg,0,1024);
		char *ip[20];
		memset(ip,0,20);
		for(int i=0;i<client_count;i++){
			if(strcmp(action[1], client_list[i].hostname) == 0)
			{	
				strcpy(ip,client_list[i].ip_address);
				break;
				
			}
		}	
		for(int i=0;i<client_count;i++)
		{	
			if(strcmp(action[1], client_list[i].hostname) == 0)
			{	
				continue;
			}
			else
			{
                                sprintf(msg,"SEND__%s##255.255.255.255##%s",ip,action[2]);
                                int len = strlen(msg);  
                                sendall(client_list[i].socket,msg,&len);
			}	
		}
		cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n", ip, action[2]); 		
		cse4589_print_and_log("[RELAYED:END]\n");
	}
	if(strcmp(action[0],"BLOCK") == 0)
	{		
		for(int i=0;i<client_count;i++)
		{
			char *msg[10];
			memset(msg,0,10);	
			if(client_list[i].socket == socket)
			{
				if(strlen(client_list[i].blocked_list) == 0)
				{
					sprintf(msg,"%s",action[1]);
				}
				else
				{
					sprintf(msg,"%s,%s",client_list[i].blocked_list,action[1]);
				}
				strcpy(client_list[i].blocked_list,msg);	
				break;
			}
		}
	}
	if(strcmp(action[0],"BUFFER") == 0)
	{
		for(int i=0;i<client_count;i++)
		{
			if(strcmp(action[1],client_list[i].hostname))
			{
				client_list[i].is_logged = 1;
				char *msg[1024];
				memset(msg,0,1024);
				sprintf(msg,"BUFFER__%s",client_list[i].buffer_msg);
				int len = strlen(msg);
				sendall(client_list[i].socket,msg,&len);
		//		printf("BUFFFFFER MSG : %s \r \n",msg);
				
			}
		}
	}
	
}

int isStringPresent(char *mainStr, char *toFind, char *delimiter)
{
	char *tokenPtr;
        tokenPtr = strtok(mainStr, delimiter);//strcpy(tokenPtr,strtok(message, "##"));
        int i=0;
        char *action[10];
        while(tokenPtr != NULL)
        {
                action[i] = tokenPtr;//strcpy(action[i],tokenPtr);
		if(strcmp(action[i],toFind) == 0)
		{
			return 1;
		}
                tokenPtr = strtok(NULL, delimiter);//strcpy(tokenPtr,strtok(NULL, "##"));
                i++;
        }
	return 0;

}
 
int findDestinationSocket(char *ip[20])
{	
	for(int i=0;i<client_count;i++)
	{
		if(strcmp(client_list[i].ip_address, ip) == 0)
		{
			return client_list[i].socket;
		}
	}

}
//From beej
			
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int run_as_client(char *port)
{
	char messge[256];
	int sock;
	sock = socket(AF_INET,SOCK_STREAM,0);
	/*struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(5499);
	server_address.sin_addr.s_addr = inet_addr("128.205.36.33");
        */
	while(1)
	{
		char command[100];
		FD_ZERO(&readfs);
		FD_SET(STDIN,&readfs);
		if(sock_flag == 1)
		{
			FD_SET(sock,&readfs);	
		}
		
		int activity = select(sock+1,&readfs,NULL,NULL,NULL);
		if(activity < 0)
		{
			//printf("AAError \r \n");
		}
		if(FD_ISSET(STDIN, &readfs))
		{
		//	printf("CLIENT SOCKET : %d \r \n",sock);
			handleClientCommand(sock);	
		}
		if(FD_ISSET(sock, &readfs))
		{
			char response[1024];
			memset(response,0,1024);
			int rec = recv(sock, &response, sizeof(response), 0);
   			char *tokenPtr;
			//printf("\n \n RES ::: %s \r \n",response);
  			tokenPtr = strtok(response, "__");
   			int i=0;
   			char *action[5];
			memset(action,0,5);
   			while(tokenPtr != NULL)
   			{
				action[i] = tokenPtr;
				tokenPtr = strtok(NULL, "__");
				i++;
  			}
			if(strcmp(action[0],"SEND") == 0)
			{
				char *tokens[10];
   				char *t;
  				char tstr[20];
				memset(tstr,0,20);	
				strcpy(tstr,action[1]);
				t = strtok(tstr,"##");
   				int j=0;
				while(t != NULL)
   				{
					tokens[j] = t;
					t = strtok(NULL, "##");
					j++;
  				}
			        cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
				cse4589_print_and_log("msg from:%s\n[msg]:%s\n", tokens[0], tokens[2]);	
                		cse4589_print_and_log("[RECEIVED:END]\n");	
			}	
			
			if(strcmp(action[0],"REFRESH") == 0 || strcmp(action[0],"LOGIN") == 0)
			{	
				if(strcmp(action[0],"REFRESH") == 0)
				{
						cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
				}
				for(int i=1;i<=5;i++)
				{	
					if(action[i] == NULL){break;}
					char *tokens[10];
   					char *t;
  					char tstr[20];
					memset(tstr,0,20);	
					strcpy(tstr,action[i]);
					t = strtok(tstr,"##");
   					int j=0;
					while(t != NULL)
   					{
						tokens[j] = t;
						t = strtok(NULL, "##");
						j++;
  					}
						//printf("NUUUUUU : %s \r \n",tokens[1]);
					//	client_list_store[i-1].id = atoi(tokens[0]);
						strcpy(client_list_store[i-1].hostname,tokens[0]);
						strcpy(client_list_store[i-1].ip_address,tokens[1]);
						client_list_store[i-1].port = atoi(tokens[2]);
						client_list_store[i-1].socket = atoi(tokens[3]);
						client_list_store[i-1].is_logged = atoi(tokens[4]);	
						client_count_store++;
					
					if(strcmp(action[0],"REFRESH") == 0)
					{	
						cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i,tokens[0],tokens[1],atoi(tokens[2]));
					}
				}
				if(strcmp(action[0],"REFRESH") == 0)
				{
						cse4589_print_and_log("[%s:END]\n", action[0]);
				}
			}
		}
	}
	return 0;
}

char *tokenGenerator(char *tokens)
{
		
}

void display_list()
{	
	char *action = "LIST";
	cse4589_print_and_log("[%s:SUCCESS]\n", action);
	for(int i=0;i<client_count;i++)
	{
		if(client_list[i].is_logged == 1)
		{
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, client_list[i].hostname, client_list[i].ip_address, client_list[i].port);
		}

	}
	cse4589_print_and_log("[%s:END]\n",action);
}

void push_to_clientlist(struct sockaddr_in *address,char host[50],int client_socket)
{
	//printf("IIIIII \r \n");	
	client_list[client_count].id = client_count + 1;
	strcpy(client_list[client_count].hostname,host);
	strcpy(client_list[client_count].ip_address,inet_ntoa(address->sin_addr));
	client_list[client_count].port = ntohs(address->sin_port);
	client_list[client_count].socket = client_socket;
	client_list[client_count].is_logged = 1;	
	client_count++;
	//display_list();		
}

int handleCommand()
{
	char *command[100];
	memset(command,0,100);
   	gets(command);
	char *tokenPtr;
  	tokenPtr = strtok(command, " ");
	
   	int i=0;
   	char *action[10];
	memset(action,0,10);
   	while(tokenPtr != NULL)
   	{
		action[i] = tokenPtr;
		tokenPtr = strtok(NULL, " ");
		i++;
  	}
	
   	switchCommands(action);
	return 0;
}

int handleClientCommand(int sock)
{
  	char command[256];
	memset(command,0,256);
   	gets(command);
	char *new_command[256];
	memset(new_command,0,256);
	strcpy(new_command,command);
  	char *tokenPtr;
   	tokenPtr = strtok(command, " ");
   	int i=0;
   	char *action[100];
	int length = strlen(command);
   	while(tokenPtr != NULL)
   	{
		action[i] = tokenPtr;
		tokenPtr = strtok(NULL, " ");
		i++;
   	}
   	if(strcmp(action[0],"AUTHOR") == 0)
	{
		printAuthor(action);
   	}
   	else if(strcmp(action[0],"IP") == 0)
	{
		printIP(action);
   	}
   	else if(strcmp(action[0],"PORT") == 0)
	{
		printPort(action[0]);
   	}
		
   	else if(strcmp(action[0],"LIST") == 0)
	{
	cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
	qsort(client_list_store,client_count_store, sizeof(struct clientlist), compare);
	for(int i=0;i<client_count_store;i++)
	{
		if(client_list_store[i].is_logged == 1)
		{
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, client_list_store[i].hostname, client_list_store[i].ip_address, client_list_store[i].port);
		}

	}
	cse4589_print_and_log("[%s:END]\n", action[0]);
		//char *buf = "LIST##";
		//int len = strlen(buf);
		//int res = sendall(sock,buf,&len);
	}
	else if(strcmp(action[0],"REFRESH") == 0){
		char *buf = "REFRESH##";
		int len = strlen(buf);
		int res = sendall(sock,buf,&len);
		
	}
   	else if(strcmp(action[0],"LOGIN") == 0)
	{
 		//if(validate(action[1],action[2])){
 			int client_flag = 0;
	//		printf("LOGINNNNNNN  : %d \r \n",client_count_store);	
 			for(int i=0;i<client_count_store;i++)
			{
				char *hostname[20];
				memset(hostname,0,20);
    				gethostname(hostname, sizeof hostname);
				if(hostname == client_list_store[i].hostname)
				{
					//printf("NEEED \r \n");
					client_list_store[i].is_logged = 1;
					char *msg[20];
					memset(msg,0,20);
					sprintf(msg,"BUFFER##%d",client_list_store[i].socket);
					int len = strlen(msg);
					sendall(sock,msg,&len);	
					return;
				}
			}
				struct sockaddr_in server_address;
        			server_address.sin_family = AF_INET;
        			server_address.sin_port = htons(atoi(action[2]));
        			server_address.sin_addr.s_addr = inet_addr(action[1]);
 				int status = connect(sock, (struct sockaddr *) &server_address, sizeof(server_address));
				cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
				cse4589_print_and_log("[%s:END]\n", action[0]);
				char *msg[20];
				memset(msg,0,20);	
				char *hostname[20];
				memset(hostname,0,20);
    				gethostname(hostname, sizeof hostname);
				sprintf(msg,"PORT##%s##%d",hostname,port_number);
				int len = sizeof(msg);
				sendall(sock,msg,&len);
				sock_flag = 1;
			
			//FD_SET(sock,&readfs);
			//display_list();
		//}
   	}
   	else if(strcmp(action[0],"SEND") == 0)
	{
		char *buf[1024];
		memset(buf,0,1024);
		char *send_msg[1024];
		memset(send_msg,0,1024);

		for(int j=2;j<i;j++)
		{
			strcat(send_msg,action[j]);
			strcat(send_msg," ");	
		}
	
		int position = action[2] - action[0];
		//sprintf(send_msg,"%s",new_command+position);
		sprintf(buf,"SEND##%s##%s",action[1],send_msg);
		int len = strlen(buf);
				int res = sendall(sock,buf,&len);	
				cse4589_print_and_log("[%s:SUCCESS]\n",action[0]);
                		cse4589_print_and_log("[%s:END]\n", action[0]);
		//if(validate_ip(action[1])){
			/*int flag = 0;
			if(!validate_ip(action[1])){
				cse4589_print_and_log("[%s:ERROR]\n",action[0]);
                		cse4589_print_and_log("[%s:END]\n", action[0]);
				return;
			}
			for(int k=0;k<client_count_store;k++)
			{

				if(strcmp(action[1],client_list_store[k].ip_address) == 0){
					flag =1;
					break;
				}	
			}
			if(flag == 1){
				int res = sendall(sock,buf,&len);
				cse4589_print_and_log("[%s:SUCCESS]\n",action[0]);
                		cse4589_print_and_log("[%s:END]\n", action[0]);
			}else{	
				cse4589_print_and_log("[%s:ERROR]\n",action[0]);
                		cse4589_print_and_log("[%s:END]\n", action[0]);
							
			}*/
		//}else{
		//		cse4589_print_and_log("[%s:ERROR]\n",action[0]);
                //		cse4589_print_and_log("[%s:END]\n", action[0]);
		
		//}
                return;
  	}
	else if(strcmp(action[0], "BROADCAST") == 0){
		char *msg[1024];
		memset(msg,0,1024);
		char *new_send[1024];
		memset(new_send,0,1024);
		char *hostname[20];
		memset(hostname,0,20);
    		gethostname(hostname, sizeof hostname);
		for(int j=1;j<i;j++)
                 {
			strcat(new_send,action[j]);
                         strcat(new_send," ");
                 }

		sprintf(msg,"BROADCAST##%s##%s",hostname,new_send);
	        int len = strlen(msg);
		sendall(sock,msg,&len);
		cse4589_print_and_log("[%s:SUCCESS]\n",action[0]);
                cse4589_print_and_log("[%s:END]\n", action[0]);
                return;

	}
	else if(strcmp(action[0], "LOGOUT") == 0)
	{
	//	printf("YYYYYYY \r \n");
		char *msg[100];
		memset(msg,0,100);
    		char hostname[128];
    		gethostname(hostname, sizeof hostname);
		sprintf(msg,"LOGOUT##%s",hostname);
		int len = strlen(msg);
		if(sendall(sock,msg,&len) == 0){
		//	printf("NEW COUNT %s \r \n");
			for(int i=0;i<client_count_store;i++)
			{
			if(strcmp(hostname,client_list_store[i].hostname) == 0)
			{
				client_list_store[i].is_logged = 0;
			}
		}
					
		}
		
	}
	else if(strcmp(action[0],"BLOCK") == 0)
	{
		char *ip[50];
		memset(ip,0,50);
		sprintf(ip,"BLOCK##%s",action[1]);
		int len  = strlen(ip);
		sendall(sock,ip,&len);
	}
	return 0;
}

bool validate(char *ip, char *port_num)
{
	int port = atoi(port_num);
 	if(strcmp(ip,server_ip) !=  0)
	{
	//	 printf("Invalid IP \r \n");
		 return false;
 	}
 	if(port != server_port)
	{
	//	printf("Invalid port \r \n");
		return false;
	}
	return true;
}

void switchCommands(char *action[10])
{
	if(action[0] == NULL) {return;}	
	if(strcmp(action[0],"AUTHOR") == 0)
	{
		printAuthor(action);
	}
  	else if(strcmp(action[0],"IP") == 0)
	{
		printIP(action);
  	}
  	else if(strcmp(action[0],"PORT") == 0)
	{
		printPort(action[0]);
 	}
  	else if(strcmp(action[0],"LIST") == 0)
	{
		qsort(client_list,client_count, sizeof(struct clientlist), compare);
		display_list();
  	}
}

//Referred Beej guide 
int printIP(char *action[10])
{
  	int sockfd;  
    	struct addrinfo hints, *servinfo, *p;
   	struct sockaddr_in *h;
    	int rv;
    	char *ip[100];
    	char hostname[128];
    	gethostname(hostname, sizeof hostname);
    	memset(&hints, 0, sizeof hints);
    	hints.ai_family = AF_INET; 
    	hints.ai_socktype = SOCK_STREAM;
 	if ( (rv = getaddrinfo( hostname , NULL , &hints , &servinfo)) != 0) 
    	{
        	return 1;
    	}
 
        for(p = servinfo; p != NULL; p = p->ai_next) 
        {
         	h = (struct sockaddr_in *) p->ai_addr; 
		strcpy(ip , inet_ntoa( h->sin_addr ) );
        }
                                      
        freeaddrinfo(servinfo); 
	cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
	cse4589_print_and_log("IP:%s\n", ip);
	cse4589_print_and_log("[%s:END]\n", action[0]);
	return 0;
}

int printAuthor(char *action[10])
{
	cse4589_print_and_log("[%s:SUCCESS]\n", action[0]);
	char *your_ubit_name = "bparthas";
	cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", your_ubit_name );
	cse4589_print_and_log("[%s:END]\n", action[0]);
 	return 0;
}

int printPort(char *action)
{
	cse4589_print_and_log("[%s:SUCCESS]\n", action);
	cse4589_print_and_log("PORT:%d\n", port_number);
	cse4589_print_and_log("[%s:END]\n", action);
}
int validate_ip (char *ip)
{
    struct sockaddr_in str;
    return (inet_pton(AF_INET, ip, &(str.sin_addr)));
}
