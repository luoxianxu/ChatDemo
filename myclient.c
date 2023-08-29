#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h> //IPPROTO_TCP
#include <sys/un.h>     //sockaddr_un
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>
#include <pthread.h>

int ClientSocketFd;
    
void *Thread_Of_ReadFunc(void *arg){
	int ret = 0;
    	char rece_buf[500];
	while(1){
		ret = read(ClientSocketFd,rece_buf,sizeof(rece_buf));
		if(ret == 0){//套接字断开连接
			printf("你已经和服务器断开连接了\n");
			close(ClientSocketFd);
			return NULL;
			
		}
	    	if(ret > 0){
	    		printf("接收到的数据为：%s\n",rece_buf);
	    		ret = 0;
	    	}
	}
}

void *Thread_Of_Func(void *arg){
	int act = 0;
	int ret = 0;
	
	char information[200] = {0};
	char username[20] = {0};
	char send_data[100] = {0};
				
	printf("************************\n");
	printf("**1 向全体用户发送信息**\n");
	printf("**2 向某个用户发送信息**\n");
	printf("******3 断开连接*******\n");
	printf("************************\n");
	while(1){
		printf("请输入你接下来的操作：\n");
		scanf("%d",&act);
		switch(act){
			//向全体用户发送信息
			case 1:
				printf("请输入你要发送的信息：\n");
				scanf("%s",send_data);
				sprintf(information,"username:all:infromation:%s",send_data);
				printf("将要发送信息为：%s\n",information);
				ret = send(ClientSocketFd,information,sizeof(information),MSG_DONTWAIT);
				if(ret == -1){
				    perror("send error");
				}
				break;
			//向某个用户发送信息
			case 2:
				printf("请输入你要发送的用户：\n");
				scanf("%s",username);
				printf("请输入你要发送的信息：\n");
				scanf("%s",send_data);
				sprintf(information,"username:%s:infromation:%s",username,send_data);
				printf("将要发送信息为：%s\n",information);
				ret = send(ClientSocketFd,information,sizeof(information),MSG_DONTWAIT);
				if(ret == -1){
				    perror("send error");
				}
				break;
			//断开连接
			case 3:
				break;
			default:
				printf("没有该项操作\n");
				break;
		}
	}
}

int main(int argc,char **argv){
    ClientSocketFd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(ClientSocketFd == -1){
        perror("socket failured");
        return -1;
    }
    struct sockaddr_in socket_addr;
    memset(&socket_addr,0,sizeof(socket_addr));
    socket_addr.sin_port = htons(atoi(argv[2]));
    socket_addr.sin_addr.s_addr = inet_addr(argv[1]);
    socket_addr.sin_family = AF_INET;

    while(connect(ClientSocketFd,(struct sockaddr *)&socket_addr,sizeof(socket_addr)) == -1){
        perror("connect error");
        sleep(1);
    }
    printf("connect successful\n");
    
    printf("******************\n");
    printf("******1 注册  ****\n");
    printf("******2 登陆  ****\n");
    printf("******************\n");
    printf("请输入选择：\n");
    int sel = 0;
    char name[20];
    char passwd[20];
    char send_buf[200];
    scanf("%d",&sel);
    if(sel == 1){
    	printf("请输入注册的用户名：\n");
    	scanf("%s",name);
    	printf("请输入注册的密码：\n");
    	scanf("%s",passwd);
    	sprintf(send_buf,"username:%s:passwd:%s:register",name,passwd);
    	send(ClientSocketFd,send_buf,100,MSG_DONTWAIT);	
    }
    printf("清开始登陆：\n");
    printf("请输入用户名：\n");
    scanf("%s",name);
    printf("请输入密码：\n");
    scanf("%s",passwd);
    sprintf(send_buf,"username:%s:passwd:%s",name,passwd);
    send(ClientSocketFd,send_buf,sizeof(send_buf),MSG_DONTWAIT);
    int i=5;
    
    pthread_t thread_of_read;
    pthread_create(&thread_of_read,NULL,Thread_Of_ReadFunc,NULL);
    
    pthread_t thread_of_func;
    pthread_create(&thread_of_func,NULL,Thread_Of_Func,NULL);
    
    while(1);
    close(ClientSocketFd);
    return 0;
}

