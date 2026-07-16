#include "header.h"

typedef struct stmp_struct
{
	int sfd;
	SSL_CTX *ctx;
	SSL *ssl;
	struct addrinfo *res;
	char buf[500];
}SMTP;

typedef struct sensor_data
{
	char date[20];
	char time[20];
	float temp;
	int load;
	int light;
}SD;

typedef struct alert_info
{
	char body[500];
	char temp_val[50];
	char load_val[50];
	char stat[60];
}alert_info;

int smtp_connect(SMTP *p)
{
	p->sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(p->sfd < 0)
	{
		perror("socket");
		return -1;
	}
	struct addrinfo hint;
	char *cp;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family   = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	getaddrinfo("smtp.gmail.com", "587", &hint, &(p->res));
	if(connect(p->sfd, p->res->ai_addr, p->res->ai_addrlen)<0)
	{
		perror("connect");
		return -1;
	}

	memset(p->buf, 0, sizeof(p->buf));
	recv(p->sfd, p->buf,sizeof(p->buf)-1, 0);
	printf("smtp_connect() success\n");
	return 0;

}

int smtp_starttls(SMTP *p)
{
	char *cp = "EHLO mysystem\r\n";
	send(p->sfd, cp, strlen(cp), 0);
	memset(p->buf, 0, sizeof(p->buf));
	recv(p->sfd, p->buf, sizeof(p->buf)-1, 0);

	cp = "STARTTLS\r\n";
	send(p->sfd, cp, strlen(cp), 0);

	memset(p->buf, 0, sizeof(p->buf));
	recv(p->sfd, p->buf, sizeof(p->buf)-1, 0);

	SSL_library_init();

	const SSL_METHOD *method;
	method = TLS_client_method();

	p->ctx = SSL_CTX_new(method);

	p->ssl = SSL_new(p->ctx);

	SSL_set_fd(p->ssl,p->sfd);

	int ret=SSL_connect(p->ssl);
	if(ret != 1)
	{
		ERR_print_errors_fp(stderr);
		return -1;
	}
	printf("smtp_starttls() success\n");
	return 0;
}

int smtp_auth(SMTP *p)
{
	char *cp="EHLO mysystem\r\n";
	if(SSL_write(p->ssl, cp, strlen(cp))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);

	cp="AUTH LOGIN\r\n";
	if(SSL_write(p->ssl, cp, strlen(cp))<=0)
	{

		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);

	cp = "palnatisai24@gmail.com";
	unsigned char out[100];	
	int len = EVP_EncodeBlock(out,(unsigned char *)cp,strlen(cp));
	strcat(out,"\r\n");
	if(SSL_write(p->ssl, out, strlen(out))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);

	cp="ecgonypkzfwcfdib";
	len = EVP_EncodeBlock(out,(unsigned char *)cp,strlen(cp));
	strcat(out,"\r\n");
	if(SSL_write(p->ssl, out, strlen(out))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);
	printf("smtp_auth() success\n");
	return 0;
}
int smtp_send_mail(SMTP *p,char *s)
{
	char *cp="MAIL FROM:<palnatisai24@gmail.com>\r\n";
	if(SSL_write(p->ssl,cp,strlen(cp))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);

	cp="RCPT TO:<naveen.gogula98@gmail.com>\r\n";
	if(SSL_write(p->ssl,cp,strlen(cp))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);

	cp="DATA\r\n";
	if(SSL_write(p->ssl,cp,strlen(cp))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);
	if(SSL_write(p->ssl,s,strlen(s))<=0)
	{
		return -1;
	}
	memset(p->buf,0,sizeof(p->buf));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);
	return 0;
}

void smtp_quit(SMTP *p)
{
	char *cp="QUIT\r\n";
	SSL_write(p->ssl,cp,strlen(cp));
	SSL_read(p->ssl,p->buf,sizeof(p->buf)-1);
	printf("%s\n",p->buf);
}
void smtp_cleanup(SMTP *p)
{
	SSL_shutdown(p->ssl);
	SSL_free(p->ssl);
	SSL_CTX_free(p->ctx);
	freeaddrinfo(p->res);
	close(p->sfd);
}

int socket_init(int *id,char **port)
{
	id[0]=socket(AF_INET,SOCK_STREAM,0);
	if(id[0]<0)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in serverid,clientid;
	serverid.sin_family=AF_INET;
	serverid.sin_port=htons(atoi(port[1]));
	serverid.sin_addr.s_addr=inet_addr(port[2]);
	if(bind(id[0],(struct sockaddr *)&serverid,sizeof(serverid))<0)
	{
		perror("bind");
		return -1;
	}
	if(listen(id[0],10)<0)
	{
		perror("listen");
		return -1;
	}
	int size;
	id[1]=accept(id[0],(struct sockaddr *)&clientid,&size);
	if(id[1]<0)
	{
		perror("accept");
		return -1;
	}
	printf("socket_init() success\n");
	return 0;
}
int terminos_init()
{
	struct termios tty;
	memset(&tty,0,sizeof(tty));
	int tfd=open("/dev/ttyUSB1",O_RDWR|O_NOCTTY);
	if(tfd<0)
	{
		perror("open");
		return -1;
	}

	tcgetattr(tfd,&tty);
	cfsetispeed(&tty,B9600);
	cfsetospeed(&tty,B9600);
	tty.c_cflag&=~PARENB;
	tty.c_cflag&=~CSTOPB;
	tty.c_cflag&=~CSIZE;
	tty.c_cflag|=CS8;
	tty.c_cflag|=CREAD;
	tty.c_cflag|=CLOCAL;
	tty.c_iflag=0;
	tty.c_oflag=0;
	tty.c_lflag=0;
	tty.c_cc[VMIN]=1;
	tty.c_cc[VTIME]=0;
	tcsetattr(tfd,TCSANOW,&tty);
	printf("terminos_init() success\n");
	return tfd;
}
int smtp_reconnect(int *sockid, char **argv, SMTP *smtp)
{
	//clearing the old info
	close(sockid[1]);
	close(sockid[0]);
	smtp_quit(smtp);
	smtp_cleanup(smtp);

	//socket server init
	if(socket_init(sockid,argv)<0)
	{
		printf("socket init()\n");
		return -1;
	}
	//smtp client init
	if(smtp_connect(smtp)<0)
	{
		printf("Error: smtp_connect()\n");
		return -1;
	}
	if(smtp_starttls(smtp)<0)
	{
		return -1;
	}

	if(smtp_auth(smtp)<0)
	{
		return -1;
	}
	printf("smtp_reconnect() success\n");
	return 0;
}


int main(int argc,char ** argv)
{
	if(argc!=3){
		printf("USAGE: ./test port ip\n");
		return 0;
	}

	SMTP smtp;
	char tbuf[100],premsg[500];
	int sockid[2];
	char ch;
	int i=0,flag=0,alflag=0,alstat=0;
	SD olddata,newdata;
	alert_info alert_mail,alert_msg;
	float loadrate[5]={0},temprate[5]={0};
	float loadtime=6,temptime=6,avgload=0,avgtemp=0;
	int tflag=0;
	int a1=0,a2=0;
	int first=0;

	memset(&alert_mail,0,sizeof(alert_info));
	memset(&alert_msg,0,sizeof(alert_msg));
	memset(&olddata,0,sizeof(olddata));

	//termios init
	int tfd=terminos_init();
	printf("tfd=%d\n",tfd);
	if(tfd<0)
	{
		return 0;
	}


	//logging .csv file init
	int logid=open("data_log.csv",O_WRONLY|O_APPEND|O_CREAT,0664);
	if(logid<0)
	{
		perror("open");
		return 0;
	}
	struct stat finfo;
	stat("data_log.csv",&finfo);
	if(finfo.st_size==0)
		write(logid,"DATE,TIME,TEMP,LOAD,LIGHT,ALERT\n",33);
	printf(".csv file created\n");

	//socket server init
	if(socket_init(sockid,argv)<0)
	{
		printf("socket init()\n");
		return -1;
	}

	//smtp client init
	if(smtp_connect(&smtp)<0)
	{
		printf("Error: smtp_connect()\n");
		return -1;
	}
	if(smtp_starttls(&smtp)<0)
	{
		return -1;
	}
	if(smtp_auth(&smtp)<0)
	{
		while(smtp_reconnect(sockid,argv,&smtp)!=0)
		{
			sleep(5);
		}
	}
	memset(tbuf,0,sizeof(tbuf));

	while(1)
	{
		if(read(tfd,&ch,1)<=0)
		{
			perror("read");
			continue;
		}
		tbuf[i++]=ch;
		if(ch=='\n')
		{
			tbuf[i-2]='\0';
			tbuf[i-1]='\0';
			flag=1;
		}
		if(flag) //reading completed
		{
			flag=0;
			sscanf(tbuf,"%s %s %f %d %d",newdata.date,newdata.time,&newdata.temp,&newdata.load,&newdata.light);
			for(i=0;tbuf[i];i++)
			{
				if(tbuf[i]==' ')
				{
					tbuf[i]=',';
				}
			}
			tbuf[i]=',';
			tbuf[i+1]='\0';
			i=0;
			//skipping 1st reading 
			if(first==0)
			{
				first=1;
				olddata=newdata;
				continue;
			}

			//predective alert system calculation
			if(olddata.temp!=newdata.temp||olddata.load!=newdata.load||olddata.light!=newdata.light)
			{

				float newloadrate=newdata.load-olddata.load;
				float newtemprate=newdata.temp-olddata.temp;
				if(tflag<5)
				{
					avgload+=newloadrate;   //change in load per 1sec
					avgtemp+=newtemprate;   //change in temp per 1sec
					loadrate[a1]=newloadrate;
					temprate[a1]=newtemprate;
					tflag++;
					printf("first five %1.f %1.f\n",avgload,avgtemp);
				}
				else
				{
					//remove 1st change add new change
					avgtemp=(avgtemp-temprate[a1])+newtemprate;   
					avgload=(avgload-loadrate[a1])+newloadrate;

					temprate[a1]=newtemprate;
					loadrate[a1]=newloadrate;

					//claculate time to reach treshold
					if(avgtemp>0)
						temptime=(80-newdata.temp)/(avgtemp/5.0);
					if(avgload>0)
						loadtime=(80-newdata.load)/(avgload/5.0);       

					int pre=0;
					char loadstr[100]="";
					char tempstr[100]="";
					if(temptime<=5 ){
						memset(tempstr,0,sizeof tempstr);
						sprintf(tempstr,"Temperature Raising\nIn %.1f seconds Reach 80\n",temptime);	
						pre=1;
					}	
					if(loadtime<=5){
						memset(loadstr,0,sizeof loadstr);
						sprintf(loadstr,"Load Increasing\nIn %.1f seconds Reach 80\n",loadtime);	
						pre=1;
					}
					if(pre && newdata.load<80 && newdata.temp<80 && tflag>=5){
						sprintf(premsg,"ALERT!!!!\n%s%s",tempstr,loadstr);
						write(sockid[1],premsg,strlen(premsg));
						pre=0;
					}
				}
				a1++;
				if(a1>=5)
					a1=0;

			}
			//alert msg and mail 
			if(olddata.temp!=newdata.temp||olddata.load!=newdata.load||olddata.light!=newdata.light)
			{
				printf("DATA : %s %s %f %d %d\n",newdata.date,newdata.time,newdata.temp,newdata.load,newdata.light);
				strcpy(alert_mail.stat,"Alert Status:\r\n");
				strcpy(alert_msg.stat,"Alert Status:\n");

				if(newdata.temp>=80)
				{
					alflag=1;
					strcat(tbuf,"Temp ");
					sprintf(alert_mail.temp_val,"Temperature: %.1f\r\n",newdata.temp);
					sprintf(alert_msg.temp_val,"Temperature: %.1f\n",newdata.temp);
					strcat(alert_mail.stat,"TEMP HIGH\r\n");
					strcat(alert_msg.stat,"TEMP HIGH\n");
				}
				else
				{
					strcpy(alert_mail.temp_val,"");
					strcpy(alert_msg.temp_val,"");
				}

				if(newdata.load>=80)
				{
					alflag=1;
					strcat(tbuf,"Load ");
					sprintf(alert_mail.load_val,"Load: %d\r\n",newdata.load);
					sprintf(alert_msg.load_val,"Load: %d\n",newdata.load);
					strcat(alert_mail.stat,"LOAD HIGH\r\n");
					strcat(alert_msg.stat,"LOAD HIGH\n");
				}
				else
				{
					strcpy(alert_mail.load_val,"");
					strcpy(alert_msg.load_val,"");
				}

				if(!alflag)
				{
					strcat(tbuf,"Normal");
				}
				if(alstat&&(!alflag))
				{
					write(sockid[1],"NORMAL\n",8);
					alstat=0;
				}

				strcat(tbuf,"\n");
				write(logid,tbuf,strlen(tbuf));
				if(alflag)
				{
					//for mail
					sprintf(alert_mail.body,"Subject: Machine Alert !!!\r\n\r\n"
							"Date:- %s Time:- %s\r\n\r\n"
							"%s\r\n"
							"%s%s"
							"\r\nRegards,\r\n"
							"Monitoring System"
							"\r\n.\r\n",newdata.date,newdata.time,alert_mail.stat,alert_mail.temp_val,alert_mail.load_val);
					//for client msg
					sprintf(alert_msg.body,"Date:- %s Time:- %s\n"
							"\n%s\n"
							"%s%s",newdata.date,newdata.time,alert_msg.stat,alert_msg.temp_val,alert_msg.load_val);
					if(smtp_send_mail(&smtp,alert_mail.body)<0)
					{
						while(smtp_reconnect(sockid,argv,&smtp)!=0)
						{
							sleep(5);
						}
						smtp_send_mail(&smtp,alert_mail.body);
					}
					write(sockid[1],alert_msg.body,strlen(alert_msg.body));
					printf("client msg sent via socket and mail\n");
					alstat=1;
				}
				alflag=0;
			}
			olddata=newdata;
			memset(tbuf,0,sizeof(tbuf));
		}
	}
	close(sockid[1]);
	close(sockid[0]);
	close(logid);
	smtp_quit(&smtp);
	smtp_cleanup(&smtp);
}
