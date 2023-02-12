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
			/* THE SERVER PROCESS */

/*int search_in_file(char *buf)
{
	char *line=malloc(26*sizeof(char));
	FILE *fp;
	fp = fopen("users.txt","r");
	int f=0;
	if(fp==NULL) 
	{
		printf("Error in opening the file\n");
		return 0;
	}
	else
	{
		while(fscanf(fp, "%s", line)!=EOF)
		{
           // printf("%s ",line);
			if(strcmp(line,buf)==0) 
			{
				//printf("Match");
				f=1;
				break;
			}
		}
        
	}
	fclose(fp);
	return f;
}*/
int recvf(int sockfd,char* buf,int len,int flag,char c)
{
      int bytes_sent=0;
      int chunk_size=100;
      while(1)
      { 
          char* chunks=malloc(chunk_size*sizeof(char));
          memset(chunks,0,chunk_size);
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
    lines[i] = (char*)malloc(70 * sizeof(char));
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
int main()
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
	serv_addr.sin_port		= htons(20000);

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
        k++;
		if (fork() == 0) {

			close(sockfd);	
            while(1)
            {
                 char req[8192];
                 memset(req,0,8192);
                 int rbytes=recvf(newsockfd,req,8192,0,'\n');
                 //if req_line[rbytes-1]=='\0' completely recieved else msg not completed
                 int noflines;
                 char **headers=split_into_lines(req,&noflines,11);
                 
                 
                 
                 
            }
			close(newsockfd);
			exit(0);
		}

	}
	return 0;
}
			
