#include"header.h"
#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define YELLOW  "\033[1;35m"
#define RESET "\033[0m"
int main(int argc,char **argv)
{
	if(argc!=2)
	{
		printf("USAGE: ./client <port no>\n");
		return 0;
	}
	int sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0)
	{
		perror("SOCKET");
		return 0;
	}
	struct sockaddr_in serverId;
	serverId.sin_family=AF_INET;
	serverId.sin_port=htons(atoi(argv[1]));
	serverId.sin_addr.s_addr=inet_addr("127.0.0.1");
	int size=sizeof(serverId);
	if(connect(sfd,(struct sockaddr *)&serverId,size)<0)
	{
		perror("CONNECT");
		return 0;
	}
	char star[]="*******************************";
	char str[500];
	while(1)
	{
		memset(str,0,500);
		if(read(sfd,str,499)<=0)
		{
			continue;
		}
		system("clear");
		if(strcmp(str,"NORMAL\n")==0)
		{
			printf(BLUE);
			printf("%s\n",star);
			printf("         ALERT STATUS\n");
			printf("%s\n",star);
			printf("            NORMAL\n");
			printf("%s\n",star);
			printf(RESET);
		}
		else if(strcmp(str,"ALERT!!!!")){
			printf(YELLOW);
			printf("%s\n",str);
			printf(RESET);
		}
		else
		{
			printf(RED);
			printf("%s\n",star);
			printf("         ALERT STATUS\n");
			printf("%s\n",star);
			printf("%s",str);
			printf("%s\n",star);
			printf(RESET);
		}
	}
	close(sfd);
}

