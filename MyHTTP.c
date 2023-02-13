#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>
			/* THE SERVER PROCESS */

int recvf(int sockfd,char* buf,int len,int flag)
{
      int bytes_sent=0;
      int chunk_size=100;
      while(1)
      { 
          char* chunks=malloc(chunk_size*sizeof(char));
          memset(chunks,0,chunk_size);
		  //printf("1");
          int t=recv(sockfd,chunks,chunk_size,flag);
          if(t<0) return t;
          strcat(buf,chunks);
          bytes_sent+=t;
          if(bytes_sent+chunk_size>len) break;
          if(t==0||chunks[t-1]=='\0') break;
      }
     return bytes_sent;  
}
char** split_into_lines(char *request, int *num_lines,int n) {

  char **lines = (char**)malloc( n* sizeof(char*));
  for (int i = 0; i < n; i++) {
    lines[i] = (char*)malloc(80 * sizeof(char));
  }
  char *line = strtok(request, "\n");
  int i = 0;
  while (line != NULL) {
    strcpy(lines[i], line);
    i++;
    line = strtok(NULL, "\n");
  }
  *num_lines = i;
  return lines;   
}

char* seperate_content(char *request)
{
  const char *delimiter = "\n\n";
  char *token;
  char *p = request;
  
  token = strstr(p, delimiter);
  if(token==NULL) return NULL;
  *token = '\0';
   p = token + strlen(delimiter);
  return p;
}
int commandtype(char *str,char *file_path)
{
	char *token=strtok(str," ");
	int ret;
	if(strcmp(token,"GET")==0) ret=0;
	else if(strcmp(token,"PUT")) ret=1;
	else return 400;

	token=strtok(NULL," ");
	if(token==NULL) return 400;
	FILE *fp;
	if(ret==0) fopen(token,"rb"); 
	else fopen(token,"wb");
	strcpy(file_path,token);
	token=strtok(NULL," ");
	if(token==NULL) return 400;
	if(strcmp(token,"HTTP/1.1")==0)
	{
		if(fp==NULL) return 404;     ///printf("No such file or directory\n");
		else return 0; 
	} 
	token=strtok(NULL," ");
	if(token!=NULL) 
	{
		printf("Error in request syntax\n");
		return 400;
	}
	close(fp);
    return ret;
}
int timecheck(char *file_path,char *token)
{
    setlocale(LC_ALL, "");
	//token=token+strlen(token)+1;
	struct stat file_info;
	if (stat(file_path, &file_info) == 0) 
	{
		struct tm *last_modified=gmtime(&file_info.st_mtime);
		struct tm  timesent;
		if(strptime(token, "%a, %d %b %Y %T GMT", &timesent))
		{
			time_t t1=mktime(&timesent);
			time_t t2=mktime(last_modified);
			if(t2>=t1) 
			{
				memset(file_path,0,8193);
				strftime(file_path,sizeof(file_path),"%a, %d %b %Y %T GMT",last_modified);
				return 1;
			}
			else return 403;
		}
		else return 400;
   }
   else return 403;
}
int headercheck(char **headers,int n,int gorp,char *resp)
{
	 char date[50];
	 char type[20];
	 memset(date,0,50);
	 memset(type,0,20);
     for(int i=1;i<n;i++)
	 {
		char *token=strtok(headers[i]," ");
        if(strcmp(token,"Date:")==0)
		{
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue;
		}
		else if(strcmp(token,"Host:")==0)
		{   
            token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
		else if(strcmp(token,"Connection:")==0)
		{ 
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
		else if(gorp==0 && strcmp(token,"Accept:")==0)
		{
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			strcpy(type,token);
			//printf("%s\n",type);
			continue; 
		}
		else if(gorp==0 && strcmp(token,"Accept-Language:")==0)
		{
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
		else if(gorp==0 && strcmp(token,"If-Modified-Since:")==0)
		{
            token=strtok(NULL," ");
            if(token==NULL) return 400;
			int res=timecheck(resp,token); //token will be updated
            if(res!=1) 
			{
				return res;
			}
			strcpy(date,resp);
			continue;
		}
		else if(gorp==1 && strcmp(token,"Content-language:")==0)
		{   
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
		else if(gorp==1 &&strcmp(token,"Content-length:")==0)
		{
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
		else if(gorp==1 && strcmp(token,"Content-type:")==0)
		{   
			token=strtok(NULL," ");
			if(token==NULL) return 400;
			continue; 
		}
        else continue;
	 }
     
	 memset(resp,0,8192);
	 strcat(resp,"HTTP/1.1 200 OK\nCache-control: no-store\nContent-language:en-us\nLast-modified: ");
     strcat(resp,date);
     strcat(resp,"\nContent-type: ");
	 strcat(resp,type);
	 strcat(resp,"\nExpires: ");
	 memset(date,0,50);
	 time_t t=time(NULL);
	 struct tm *time_info=gmtime(&t);
	 time_info->tm_mday += 3;
	 t = timegm(time_info);
	 time_info = gmtime(&t);
	 strftime(date, sizeof(date), "%a, %d %b %Y %T GMT", time_info);
	 strcat(resp,date);
	 return 1;
}
int main(int argv,char *argc[])
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(atoi(argc[1]));

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); 
	int k=0;
	while (1) {

		
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		if (fork() == 0) {

			close(sockfd);
			printf("Connected to client\n");
            int h=0;
			int completed=0;
			char** headers;
            while(!completed)
            {
                 char req[8193];
                 memset(req,0,8193);
                 int rbytes=recvf(newsockfd,req,8192,0);
				 //printf("%d\n",rbytes);
				 if(req[rbytes-1]=='\0') 
				 {
					completed=1;
				 }
                 int headerCount;
				 char *content;
				 if(h==0) content=seperate_content(req); 
				 if(h==1) continue;
				 if(content) h=1; 
				 headers=split_into_lines(req,&headerCount,11);
				 printf("Request recieved:\n");
				 for(int i=0;i<headerCount;i++) printf("%s\n",headers[i]);
				 printf("\n");
				 memset(req,0,8193);
				 FILE *fp; 
				 
				 int p=commandtype(headers[0],req);
				 printf("Response sent:\n");
				 if(p==0)   //if it is a get request
				 {
					int t;
					struct tm lastmodified;
					fp=fopen(req,"rb");
		            t=headercheck(headers,headerCount,0,req);
                    if(t==1)
					{
						strcat(req,"\n\n");
						//send(newsockfd,req,strlen(req),0);
						
						printf("%s",req);
                        memset(req,0,8193);
						while (fgets(req,8193,fp) != NULL)
						{
							//send(newsockfd,req,strlen(req),0);
							printf("%s",req);
							memset(req,0,8192);
							
						}
					}
					else if(t==400)
					{
						memset(req,0,8192);
						strcat(req,"HTTP/1.1 400 Bad Request\n");
						strcat(req,"Context-type: text/plain\n");
						strcat(req,"Context-length: \n\n");
						strcat(req,"The request was malformed. Please check the syntax and try again.");
                        printf("%s",req);
					}
					else if(t==403)
					{
					    memset(req,0,8192);
						strcat(req,"HTTP/1.1 403 Forbidden\n");
						strcat(req,"Context-type: text/plain\n");
						strcat(req,"Context-length: \n\n");
						strcat(req,"The file is last modified before 2 days");
						printf("%s",req);
					}
				 }
				 else if(p==1)
				 { 
                    int t;
					struct tm lastmodified;
					fp=fopen(req,"wb");
		            t=headercheck(headers,headerCount,0,req);
                    if(t==1)
					{
						memset(req,0,8192);
						strcat(req,"HTTP/1.1 200 OK\nCache-Control: no-store\n");
						printf("%s",req);
						fputs(content,fp);
						send(newsockfd,req,strlen(req),0);
					}      
				 }
                 else if(p==404)
				 {
                      memset(req,0,8192);
					  strcat(req,"HTTP/1.1 404 Not Found\n");
					  strcat(req,"Context-type: text/plain\n");
					  strcat(req,"Context-length: \n\n");
					  strcat(req,"No such file or directory");
					  printf("%s",req);
				 }
				 else if(p==400)
				 {
					memset(req,0,8192);
					strcat(req,"HTTP/1.1 400 Bad Request\n");
					strcat(req,"Context-type: text/plain\n");
					strcat(req,"Context-length: \n\n");
					strcat(req,"The request was malformed. Please check the syntax and try again.");
					printf("%s",req);
				 }
				
				
            }
			printf("\nClosing with the client\n");
			close(newsockfd);
			exit(0);
		}

	}
	return 0;
}
			
