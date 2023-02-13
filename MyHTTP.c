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
#define SIZE 300
int recvf(int sockfd,char* buf,int len,int flag,int pdf)
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
          if(t==0) break;
		  if(pdf==1) continue;
		  if(chunks[t-1]=='\0') break;
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
	else if(strcmp(token,"PUT")==0) ret=1;
	else return 400;

	token=strtok(NULL," ");
	if(token==NULL) return 400;
	FILE *fp;
	if(ret==0) fp=fopen(token,"rb"); 
	else fp=fopen(token,"wb");
	//file_path=realloc(file_path,(strlen(token)+50)*sizeof(char));
	strcat(file_path,token);
	token=strtok(NULL," ");
	if(token==NULL) return 400;
	if(strcmp(token,"HTTP/1.1")==0)
	{
		if(fp==NULL) return 404;     ///printf("No such file or directory\n");
		else 
		{
            token=strtok(NULL," ");
			if(token!=NULL) 
			{
				printf("Error in request syntax\n");
				return 400;
			}
			return ret; 
		}
	} 
	
	fclose(fp);
    return ret;
}
int timecheck(char *file_path,char *token)
{
    setlocale(LC_ALL, "");
	token=token+strlen(token)+1;
	struct stat file_info;
	if (stat(file_path, &file_info) == 0) 
	{
		time_t last=file_info.st_mtime;
		struct tm *last_modified=gmtime(&last);
		struct tm  timesent;
		if(strptime(token, "%a, %d %b %Y %T GMT", &timesent))
		{
			time_t t1=mktime(&timesent);
			time_t t2=mktime(last_modified);
			if(t2>=t1) 
			{
				memset(file_path,0,strlen(file_path)+1);
				strftime(file_path,50,"%a, %d %b %Y %T GMT",last_modified);
				return 1;
			}
			else return 403;
		}
		else return 400;
   }
   else return 403;
}
int headercheck(char **headers,int n,int gorp,char *resp,int* len)
{
	 char date[50];
	 char type[20];
	 memset(date,0,50);
	 memset(type,0,20);
	 int len;
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
			
			int res=timecheck(resp,token); //token will be updated
            if(res!=1) return res;
			strcpy(date,resp);
			printf("%s\n",date);
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
			*len=atoi(token);
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
	 
	// else *n=-1;
     if(gorp==0)
	 {
	 memset(resp,0,SIZE-1);
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
	 }
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
            int content_started=0;
			int completed=0;
			char** headers;
			char *content;
			FILE *fp; 
            while(!completed)
            {
                 char *req=malloc(SIZE*sizeof(char));
                 memset(req,0,SIZE);
                 int rbytes=recvf(newsockfd,req,SIZE-1,0,0);   //printf("%d\n",rbytes);
				 if(req[rbytes-1]=='\0') completed=1; 
                 int headerCount;
				 if(content_started) 
				 {
                    fputs(req,fp);
					//printf("%s\n",req);
					free(req);
					continue;
				 }
				 if(!content_started) content=seperate_content(req);
				 
				headers=split_into_lines(req,&headerCount,11);
				printf("Request recieved:\n");
				for(int i=0;i<headerCount;i++) printf("%s\n",headers[i]);
				printf("\n");
				
				if(content!=NULL) content_started = 1;
				
				char *file_path=malloc(sizeof(char));
				int p=commandtype(headers[0],file_path);
				printf("Response sent:\n");
				int content_length;
				if(p==0)   //if it is a get request
				{
					int t;
					struct tm lastmodified;
					fp=fopen(file_path,"rb");
					strcpy(req,file_path);
		            t=headercheck(headers,headerCount,0,req,&content_length);
                    if(t==1)
					{
						strcat(req,"\nContent-length: ");
                        fseek(fp, 0L, SEEK_END);
			            long file_size = ftell(fp);
			            rewind(fp);
						char length[20];
			            sprintf(length,"%ld",file_size);
			            strcat(req,length);
						strcat(req,"\n\n");
						//send(newsockfd,req,strlen(req),0);
						
						printf("%s",req);
                        memset(req,0,strlen(req)+1);
						while (fgets(req,SIZE,fp) != NULL)
						{
							//send(newsockfd,req,strlen(req),0);
							printf("%s",req);
							memset(req,0,strlen(req)+1);
							
						}
					}
					else if(t==400)
					{
						memset(req,0,strlen(req)+1);
						strcat(req,"1HTTP/1.1 400 Bad Request\n");
						strcat(req,"Context-type: text/plain\n");
						strcat(req,"Context-length: \n\n");
						strcat(req,"The request was malformed. Please check the syntax and try again.");
                        printf("%s",req);
					}
					else if(t==403)
					{
					    memset(req,0,strlen(req)+1);
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
					fp=fopen(file_path,"wb");
		            t=headercheck(headers,headerCount,1,file_path,&content_length);
                    if(t==1)
					{
						//printf("%s\n",content);
						fputs(content,fp);
                        memset(req,0,strlen(req)+1);
						strcat(req,"HTTP/1.1 200 OK\nCache-Control: no-store\n");
						printf("%s",req);
						//send(newsockfd,req,strlen(req),0);
					}      
				 }
                 else if(p==404)
				 {
                      memset(req,0,strlen(req)+1);
					  strcat(req,"HTTP/1.1 404 Not Found\n");
					  strcat(req,"Context-type: text/plain\n");
					  strcat(req,"Context-length: \n\n");
					  strcat(req,"No such file or directory");
					  printf("%s",req);
				 }
				 else if(p==400)
				 {
					memset(req,0,strlen(req)+1);
					strcat(req,"2HTTP/1.1 400 Bad Request\n");
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
			
