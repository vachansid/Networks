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
#define SIZE 8192

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
char* recvf(int sockfd,char* buf,int* clen)
{
      int bytes_recv=0;
      int chunk_size=100;
	  char *content;
      while(1)
      { 
          char* chunks=malloc(chunk_size*sizeof(char));
          memset(chunks,0,chunk_size);
          int t=recv(sockfd,chunks,chunk_size,0);
          if(t<0) return NULL;
          strcat(buf,chunks);
		  bytes_recv+=t;
		  if((content=seperate_content(buf))!=NULL)
		  {
			  *clen=bytes_recv-strlen(buf)-2;
			  break;
		  }
		  if(buf[bytes_recv-1]=='\0') break;
          if(t==0) break;
      }
     return content;  
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
int headercheck(char **headers,int n,int gorp,char *resp,long long int* len)
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
long long int min(long long int a,long long int b)
{
	if(a>b) return b;
	else return a;
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
        
			FILE *fp; 
            
			char *req=malloc(SIZE*sizeof(char));
			memset(req,0,SIZE);
			int clen;
			char* content=recvf(newsockfd,req,&clen);
			int headerCount;
			printf("Request recieved:\n");
			printf("%s\n",req);
			char** headers=split_into_lines(req,&headerCount,11);
				
			char *file_path=malloc(sizeof(char));
			int p=commandtype(headers[0],file_path);
			printf("Response sent:\n");
			long long int content_length;
			int t;
			if(p==0)   //if it is a get request
			{
				struct tm lastmodified;
				fp=fopen(file_path,"rb");
				strcpy(req,file_path);
				t=headercheck(headers,headerCount,0,req,&content_length);
				if(t==1)
				{
					strcat(req,"\nContent-length: ");
					fseek(fp, 0L, SEEK_END);
					long long int file_size = ftell(fp);
					rewind(fp);
					char length[20];
					sprintf(length,"%lld",file_size);
					strcat(req,length);
					strcat(req,"\n\n");
					send(newsockfd,req,strlen(req),0);
					printf("%s",req);

					memset(req,0,SIZE);
					int read_size;
					long long int x=file_size;
					while((read_size=fread(req,1,SIZE,fp))>0)
					{
						send(newsockfd,req,read_size,0);
						x-=read_size;
						printf("%lld\n",x);
						memset(req,0,SIZE);
					}
					fclose(fp);
				}
			
			}
			else if(p==1)
			{ 
				struct tm lastmodified;
				fp=fopen(file_path,"wb");
				t=headercheck(headers,headerCount,1,file_path,&content_length);
				if(t==1)
				{
					content_length-=clen;
					fwrite(content,1,clen,fp);
					while(content_length>0)
					{
						memset(req,0,8192);
						int bytes=recv(newsockfd,req,8192,0);
						fwrite(req,1,bytes,fp);
						content_length-=bytes;
					}
					memset(req,0,8192);
					strcat(req,"HTTP/1.1 200 OK\nCache-Control: no-store\n");
					printf("%s",req);
					fclose(fp);
					send(newsockfd,req,strlen(req)+1,0);
				}      
			}
			else if(p==404)
			{
				memset(req,0,strlen(req)+1);
				strcat(req,"HTTP/1.1 404 Not Found\n");
				strcat(req,"Content-type: text/plain\n");
				strcat(req,"Content-length: \n\n");
				strcat(req,"No such file or directory");
				send(newsockfd,req,strlen(req)+1,0);
				printf("%s",req);
			}
			else if(p==400)
			{
				memset(req,0,strlen(req)+1);
				strcat(req,"2HTTP/1.1 400 Bad Request\n");
				strcat(req,"Content-type: text/plain\n");
				strcat(req,"Content-length: \n\n");
				strcat(req,"The request was malformed. Please check the syntax and try again.");
				send(newsockfd,req,strlen(req)+1,0);
				printf("%s",req);
			}
			if(t==400)
			{
				memset(req,0,strlen(req)+1);
				strcat(req,"1HTTP/1.1 400 Bad Request\n");
				strcat(req,"Context-type: text/plain\n");
				strcat(req,"Context-length: \n\n");
				strcat(req,"The request was malformed. Please check the syntax and try again.");
				send(newsockfd,req,strlen(req)+1,0);
				printf("%s",req);
			}
			else if(t==403)
			{
				memset(req,0,strlen(req)+1);
				strcat(req,"HTTP/1.1 403 Forbidden\n");
				strcat(req,"Context-type: text/plain\n");
				strcat(req,"Context-length: \n\n");
				strcat(req,"The file is last modified before 2 days");
				send(newsockfd,req,strlen(req)+1,0);
				printf("%s",req);
			}
			
		
			printf("\nClosing with the client\n");
			close(newsockfd);
			exit(0);
		}

	}
	return 0;
}
			
