
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <time.h>

#define SIZE 250

void get_ip(char *url, char *ip)
{
	int temp = 0;
	for(int i=7; i<strlen(url); i++)
	{
		if(url[i] == '/')
		{
			ip[temp] = '\0';
			break;
		}
		ip[temp++] = url[i];
	}
}

int get_port(char *url)
{
	int temp;
	for(long int i = strlen(url); i>=0; i--)
	{
		if(url[i] == ':')
		{
			temp = i;
			break;
		}
	}
	temp+=1;
	if(atoi(url+temp) == 0 && strlen(url+temp)>5)
	{
		return 80;
	}
	else
	{
		return atoi(url+temp);
	}
}

int connect_server(char *ip, int port)
{
	int	sockfd ;
	struct sockaddr_in	serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Unable to create socket\n");
		return sockfd;
	}   

	serv_addr.sin_family	= AF_INET;
	int s = inet_aton(ip, &serv_addr.sin_addr);
	if (s == 0) 
	{
        printf("Not in presentation format\n");
        return -1;
    } else if (s < 0) 
	{
        printf("Unable to convert IP address\n");
        return -1;
    }
	serv_addr.sin_port	= htons(port);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) 
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	else
	{
		printf("Connected to the server\n");
	}
	return sockfd;
}

int main()
{
	while(1)
	{
		printf("\nMyOwnBrowser> ");
		long int l = 5;
		char *expr = (char *)malloc(sizeof(char));
		memset(expr,0,sizeof(expr));
		while(1)
		{
			char ch;
			scanf("%c",&ch);
			if(ch == '\n') break;
			l += 1;
			expr = realloc(expr,l*sizeof(char));
			strncat(expr,&ch,1);
		}
		int m = strlen(expr);
		expr[m] = '\0';
		//printf("%s\n", expr);
		//printf("%ld\n",strlen(expr));
		char* delimiter = " ";
		char* str = strtok(expr,delimiter);
		if(strcmp(str,"GET")==0)
		{
			char* url = strtok(NULL,delimiter);
			printf("%s\n", url);
			char *ip = (char *)malloc(20*sizeof(char));
			get_ip(url,ip);
			printf("%s\n", ip);
			int port = get_port(url);
			printf("%d\n",port);
			struct pollfd fdset;
			char buf[4096];
			long int m= 6;
			memset(buf,0,sizeof(buf));
			int sockfd = connect_server(ip,port);
			strcat(buf, "GET ");
			send(sockfd,"GET ",4,0);
			int temp;
			for(long int i = strlen(url); i>=0; i--)
			{
				if(url[i] == ':')
				{
					temp = i;
					break;
				}
			}
			char ch = '\n';
			char path[8096];
			memset(path,0,8096);
			if(atoi(url+temp+1) == 0 && *(url+temp+1)!=0)
			{
				strncpy(path,url+7+strlen(ip),strlen(url)-7-strlen(ip));
				strcat(buf,path);
				send(sockfd,path,strlen(path),0);
				strcat(buf, " HTTP/1.1\n");
				send(sockfd," HTTP/1.1\n",10,0);
				strcat(buf, "Host: ");
				send(sockfd,"Host: ",6,0);
				strcat(buf,ip);
				send(sockfd,ip,strlen(ip),0);
				strcat(buf,":80\n");
				send(sockfd,":80\n",4,0);

			}
			else
			{
				strncpy(path,url+7+strlen(ip),temp-7-strlen(ip));
				strcat(buf,path);
				send(sockfd,path,strlen(path),0);
				strcat(buf, " HTTP/1.1\n");
				send(sockfd," HTTP/1.1\n",10,0);
				strcat(buf, "Host: ");
				send(sockfd,"Host: ",6,0);
				strcat(buf,ip);
				send(sockfd,ip,strlen(ip),0);
				strcat(buf,url+temp);
				send(sockfd,url+temp,strlen(url+temp),0);
				strncat(buf,&ch,1);
				send(sockfd,"\n",1,0);
			}
			
			strcat(buf, "Date: ");
			send(sockfd,"Date: ",6,0);
			time_t current_time = time(NULL);
			struct tm *time_info = gmtime(&current_time);
			char date[80];
			strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", time_info);
			strcat(buf,date);
			send(sockfd,date,strlen(date),0);
			strncat(buf,&ch,1);
			send(sockfd,"\n",1,0);
			strcat(buf, "Accept: ");
			send(sockfd,"Accept: ",8,0);
			int res;
			for(long int i = strlen(path); i>=0; i--)
			{
				if(path[i] == '.')
				{
					res = i;
					break;
				}
			}
			res+=1;
			if(strcmp(path+res,"pdf") == 0)
			{
				strcat(buf,"application/pdf\n");
				send(sockfd,"application/pdf\n",16,0);
			}
			else if(strcmp(path+res,"html") == 0)
			{
				strcat(buf,"text/html\n");
				send(sockfd,"text/html\n",10,0);
			}
			else if(strcmp(path+res,"jpg") == 0)
			{
				strcat(buf,"image/jpeg\n");
				send(sockfd,"image/jpeg\n",11,0);
			}
			else
			{
				strcat(buf,"text/*\n");
				send(sockfd,"text/*\n",7,0);
			}
			strcat(buf,"Accept-Language: en-US,en;q=0.8\n");
			send(sockfd,"Accept-Language: en-US,en;q=0.8\n",32,0);
			strcat(buf, "If-Modified-Since: ");
			send(sockfd,"If-Modified-Since: ",19,0);
			memset(date,0,sizeof(date));
			time_info->tm_mday -= 2;
			time_t new_time = timegm(time_info);
			time_info = gmtime(&new_time);
			strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", time_info);
			strcat(buf,date);
			send(sockfd,date,strlen(date),0);
			strncat(buf,&ch,1);
			send(sockfd,"\n",1,0);
			strcat(buf, "Connection: close\n");
			send(sockfd,"Connection: close\n",18,0);
			printf("%s",buf);
			/*
			int sockfd = connect_server(ip,port);
			send(sockfd,expr,strlen(expr)+1,0);
			fdset.fd = sockfd;
			fdset.events = POLLIN;
			int timeout = 3000;
			if(poll(&fdset,1,timeout) == 0)
			{
				printf("Connection Timed Out\n");
				close(sockfd);
				continue;
			}
			else
			{
				char *buf = (char *)malloc(sizeof(char));
				//
			}*/

		}
		else if(strcmp(str,"PUT") == 0)
		{
			char* url = strtok(NULL,delimiter);
			char* filepath = strtok(NULL,delimiter);
			printf("%s\n", url);
			printf("%s\n", filepath);
			char *ip = (char *)malloc(20*sizeof(char));
			get_ip(url,ip);
			printf("%s\n", ip);
			int port = get_port(url);
			printf("%d\n",port);
			char buf[10000];
			memset(buf,0,sizeof(buf));
			int sockfd = connect_server(ip,port);
			strcat(buf, "PUT ");
			send(sockfd,"PUT ",4,0);
			int temp;
			for(long int i = strlen(url); i>=0; i--)
			{
				if(url[i] == ':')
				{
					temp = i;
					break;
				}
			}

			char ch = '\n';
			char path[9000];
			char c = '/';
			if(atoi(url+temp+1) == 0 && *(url+temp+1)!=0)
			{
				strncpy(path,url+7+strlen(ip),strlen(url)-7-strlen(ip));
				strcat(buf,path);
				send(sockfd,path,strlen(path),0);
				strncat(buf,&c,1);
				send(sockfd,"/",1,0);
				strcat(buf,filepath);
				send(sockfd,filepath,strlen(filepath),0);
				strcat(buf, " HTTP/1.1\n");
				send(sockfd," HTTP/1.1\n",10,0);
				strcat(buf, "Host: ");
				send(sockfd,"Host: ",6,0);
				strcat(buf,ip);
				send(sockfd,ip,strlen(ip),0);
				strcat(buf,":80\n");
				send(sockfd,":80\n",4,0);
			}
			else
			{
				strncpy(path,url+7+strlen(ip),temp-7-strlen(ip));
				strcat(buf,path);
				send(sockfd,path,strlen(path),0);
				strncat(buf,&c,1);
				send(sockfd,"/",1,0);
				strcat(buf,filepath);
				send(sockfd,filepath,strlen(filepath),0);
				strcat(buf, " HTTP/1.1\n");
				send(sockfd," HTTP/1.1\n",10,0);
				strcat(buf, "Host: ");
				send(sockfd,"Host: ",6,0);
				strcat(buf,ip);
				send(sockfd,ip,strlen(ip),0);
				strcat(buf,url+temp);
				send(sockfd,url+temp,strlen(url+temp),0);
				strncat(buf,&ch,1);
				send(sockfd,"\n",1,0);
			}
			strcat(buf, "Date: ");
			time_t current_time = time(NULL);
			struct tm *time_info = gmtime(&current_time);
			char date[80];
			strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", time_info);
			strcat(buf,date);
			strncat(buf,&ch,1);
			strcat(buf, "Accept: text/*\n");	
			strcat(buf,"Accept-Language: en-US,en;q=0.8\n");
			strcat(buf,"Content-language: en-US\n");
			FILE *fp = fopen(filepath, "rb");
			if (fp == NULL) 
			{
				perror("Error opening file");
				continue;
			}

			// Get the size of the file
			fseek(fp, 0, SEEK_END);
			long file_size = ftell(fp);
			printf("%ld\n",file_size);
			rewind(fp);

			// Allocate memory for the buffer
			char *body = (char*)malloc(sizeof(char) * file_size);
			if (body == NULL) 
			{
				perror("Error allocating memory");
				fclose(fp);
				continue;
			}

			// Read the file data
			size_t result = fread(body, 1, file_size, fp);
			if (result != file_size) 
			{
				perror("Error reading file");
				free(body);
				fclose(fp);
				continue;
			}

			// Add the buffer to the request body
			// ...

			// Clean up
			
			body[file_size-1] = '\0';
			//printf("%s",body);
			//printf("%ld",strlen(body));
			fclose(fp);

			strcat(buf,"Content-length: ");
			char length[20];
			sprintf(length,"%ld",file_size);
			strcat(buf,length);
			strncat(buf,&ch,1);
			strcat(buf,"Content-type: ");
			int res = 0;
			for(long int i = strlen(filepath); i>=0; i--)
			{
				if(filepath[i] == '.')
				{
					res = i;
					break;
				}
			}
			res+=1;
			if(strcmp(filepath+res,"pdf") == 0) strcat(buf,"application/pdf\n");
			else if(strcmp(filepath+res,"html") == 0) strcat(buf,"text/html\n");
			else if(strcmp(filepath+res,"jpg") == 0) strcat(buf,"image/jpeg\n");
			else strcat(buf,"text/*\n");
			strcat(buf, "Connection: close\n");
			strncat(buf,&ch,1);
			strcat(buf,body);
			strncat(buf,&ch,1);
			printf("%s",buf);
			/*
			int sockfd = connect_server(ip,port);
			send(sockfd,expr,strlen(expr)+1,0);
			struct pollfd fdset;
			fdset.fd = sockfd;
			fdset.events = POLLIN;
			int timeout = 3000;
			if(poll(&fdset,1,timeout) == 0)
			{
				printf("Connection Timed Out\n");
				close(sockfd);
				continue;
			}
			else
			{

			}
			*/
		}
		else if(strcmp(str,"QUIT") == 0)
		{
			break;
		}
		else
		{
			printf("Only GET, PUT, QUIT Command-line operators are supported\n");
			continue;
		}
	}
	/*
	// Opening a socket is exactly similar to the server process 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}


	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	else
	{
		printf("Connected to the server\n");
	}
		
	for(i=0; i < 50; i++) buf[i] = '\0';
	while(1)
	{
		char temp[50];
		memset(temp,0,sizeof(temp));
		int x= recv(sockfd,temp,50,0);
		if(x < 0) 
		{
			printf("Unable to read from socket\n");
			close(sockfd);
			exit(0);
		}
		else if(x == 0) 
		{
			printf("Connection closed by Server\n");
			close(sockfd);
			exit(0);
		}
		strcat(buf, temp);
		//printf("%s\n", out);
		if(temp[x - 1] == '\0') break;
	}
	printf("%s", buf);
	for(i=0; i < 50; i++) buf[i] = '\0';
	scanf("%[^\n]%*c", buf);
	send(sockfd, buf, strlen(buf) + 1, 0);
	for(i=0; i < 50; i++) buf[i] = '\0';
	
		
	close(sockfd);
	*/
	return 0;

}