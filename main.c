#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> //IPPROTO_TCP
#include <sys/un.h>     //sockaddr_un
#include <arpa/inet.h>  //inet_addr
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>	//线程

#include "MySqlite.h"

#define DB_Name				"user_db"
#define CreateTableGrammer	"create table user_table(username char(20) PRIMARY KEY,passwd char(20),status int);"		//使用username作为主键
#define InsertTableGrammer1	"insert into user_table values('zhangsan','123456',0);"
#define InsertTableGrammer2	"insert into user_table values('lisi','234567',0);"
#define InsertTableGrammer3	"insert into user_table values('wangwu','345678',0);"
#define InsertTableGrammer4	"insert into user_table values('zhaoliu','456789',0);"

//用于在添加用户节点时对链表进行上锁
pthread_spinlock_t Node_spin;

//用于记录用于的登录状态     其他值表示用户不存在  1表示已登录   0表示未登录   
int status = 3;

//用于存储客户端套接字
typedef struct FdNode{
	char *username;
	char *passwd;
	int fd;					//通信套接字
	struct FdNode *next;	//指向下一个通信套接字
}node;

struct FdNode *head;
struct FdNode *s;		//head->next为头节点，第一个为空

//存储用户登录信息的数据库
sqlite3 *mydb;

//数据库查询回调函数
int SelectCallBackFunc(void *data,int col_count,char **col_values,char **col_name){
	if(col_count > 0){
		if(strcmp(col_values[2],"1")==0){
				status = 1;
				printf("%s\n",col_values[2]);
			}
			else if(strcmp(col_values[2],"0")==0){
				status = 0;
				printf("%s\n",col_values[2]);
			}
			
		}
		else status = 2;
	return 0;
}

//接收信息处理
void information_handle(char *data){
	char *information;
	char *name;
	char *rec_date;
	int flag = 0;
	int name_flag = 0;
	//分割
	information = strtok(data,":");			//以冒号分割用户数据
	while(information!=NULL){
//		printf("信息为：%s\n",information);
		if(flag == 1){
			//寻找到用户名
			if(strcmp(information,"all")==0){
				name_flag = 1;
				name = information;
			}
			else{
				name_flag = 2;
				name = information;
			}
			flag = 0;
		}
		else if(flag == 2){
			//获取得到信息
			rec_date = information;
			flag = 0;
			
		}
		if(strcmp(information,"username")==0)
			flag = 1;
		if(strcmp(information,"infromation")==0)
			flag = 2;
		information = strtok(NULL,":");
	}
	
	printf("接收到的用户名为：%s\n",name);
	printf("接收到的用户信息为：%s\n",rec_date);
	
}
//线程接收函数
void *thread_rec_Func(void *arg){
	int fd = *(int *)arg;
	printf("fd = %d\n",fd);
	int ret = 0;
	char buf[200] = {0};
	char user[500];
	char *username;
	char user_information[100] = {0};
	char *information;
	char *name;
	char *passwd;
	int flag = 0;
	
	loop:
	read(fd,user_information,100);//阻塞等待客户端将用户信息发送  用户名以及密码
	printf("用户信息为：%s\n",user_information);
	//将用户名和密码提取出来
	information = strtok(user_information,":");			//以冒号分割用户数据 分割用户名和密码
	while(information!=NULL){
		if(flag == 1){
			name = information;
			flag = 0;
		}
		else if(flag == 2){
			passwd = information;
			flag = 0;
		}
		if(strcmp(information,"register")==0){
			flag = 3;
		}
		else if(strcmp(information,"username")==0)
			flag = 1;
		if(strcmp(information,"passwd")==0)
			flag = 2;
		information = strtok(NULL,":");
	}
	if(flag == 3){		//注册用户
		char insert_grammer[200];
		sprintf(insert_grammer,"insert into user_table values('%s','%s',0);",name,passwd);
		printf("插入语句为：%s\n",insert_grammer);
		DataBaseSelectGrammer(mydb,insert_grammer,SelectCallBackFunc);
		flag = 0;
		goto loop;
	}
	char select_grammer[200];
	sprintf(select_grammer,"select * from user_table where username = '%s' and passwd = '%s';",name,passwd);
	DataBaseSelectGrammer(mydb,select_grammer,SelectCallBackFunc);
	
	if(status == 0){
		printf("登陆成功\n");
	}
	else if(status == 1){
		printf("用户已登录\n");
		close(fd);				//断开连接，关闭套接字
		return NULL;			//结束线程
	}
	else{
		printf("该用户不存在\n");
		close(fd);				//断开连接，关闭套接字
		return NULL;			//结束线程
	}
	status = 3;
	
	char update_grammer[200];
	sprintf(update_grammer,"update user_table set status = 1 where username = '%s' and passwd = '%s';",name,passwd);
	DataBaseGrammer(mydb,update_grammer);
	
	//将用户信息存储进链表中
	struct FdNode *Node = (struct FdNode *)malloc(sizeof(struct FdNode));
	Node->username = name;
	Node->passwd = passwd;
	Node->fd = fd;
	Node->next = NULL;
	
	//上锁
	pthread_spin_lock(&Node_spin);
	s->next = Node;
	s=s->next;
	//解锁
	pthread_spin_unlock(&Node_spin);
	flag = 1;
	int twice = 0;
	int name_flag = 0;
	char *rec_date;
	while(1){
		ret = read(fd,buf,sizeof(buf));
		if(ret == 0){
			//如果客户端断开连接，read函数会返回0
			printf("%d断开连接了\n",fd);
			//更新用户的登录状态
			char update_grammer[200];
			sprintf(update_grammer,"update user_table set status = 0 where username = '%s' and passwd = '%s';",name,passwd);
			DataBaseGrammer(mydb,update_grammer);
			//从链表中删除该节点
			struct FdNode *Pre;
			for(struct FdNode *p = head;p!=NULL;p=p->next){
				if(p->fd == fd){
					Pre->next = p->next;
					break;
				}
				Pre = p;
			}
			close(fd);
			return NULL;
		}
		if(ret > 0){
			if(twice==0){
				twice = 1;
				continue;
			}
			printf("fd = %d\n",fd);
			printf("接收到的信息为：%s\n",buf);
			
			information = strtok(buf,":");			//以冒号分割用户数据
			while(information!=NULL){
				if(flag == 1){
					//寻找到用户名
					if(strcmp(information,"all")==0){
						name_flag = 1;
						name = information;
					}
					else{
						name_flag = 2;
						name = information;
					}
					flag = 0;
				}
				else if(flag == 2){
					//获取得到信息
					rec_date = information;
					flag = 0;
				}
				if(strcmp(information,"username")==0)
					flag = 1;
				if(strcmp(information,"infromation")==0)
					flag = 2;
				information = strtok(NULL,":");
			}
			
			if(name_flag == 1){
				for(struct FdNode *p = head->next;p!=NULL;p=p->next){
					if(p->fd != fd){
						printf("将要发送信息的fd为：%d\n",p->fd);
						send(p->fd,rec_date,sizeof(rec_date),MSG_DONTWAIT);
					}
				}
			}
			else if(name_flag == 2){
				for(struct FdNode *p = head->next;p!=NULL;p=p->next){
					if(strcmp(p->username,name)==0){
						printf("1:%s 2:%s\n",p->username,name);
						send(p->fd,rec_date,sizeof(rec_date),MSG_DONTWAIT);
						break;
					}
				}
			}
			name_flag = 0;
		}
	}
}

void *thread_print_Func(void *arg){
	while(1){
		for(struct FdNode *p = head->next;p!=NULL;p=p->next){
			printf("用户名：%s\n",p->username);
			printf("密码：%s\n",p->passwd);
			printf("fd：%d\n\n",p->fd);
		}
		sleep(5);
	}
}

int main(int argc,char **argv){
	if(argc < 3){
		printf("请按要求输入IP地址和端口号：\n");
		return -1;
	}
	//链表头初始化
	printf("PID = %d",getpid());
	head = (struct FdNode *)malloc(sizeof(struct FdNode));
	s=head;
	//创建数据库用于用户登录
	CreateOrOpenDataBase(DB_Name,&mydb);
	CreateTable(mydb,CreateTableGrammer);		//创建表
	DataBaseGrammer(mydb,InsertTableGrammer1);
	DataBaseGrammer(mydb,InsertTableGrammer2);
	DataBaseGrammer(mydb,InsertTableGrammer3);
	DataBaseGrammer(mydb,InsertTableGrammer4);
	
	pthread_spin_init(&Node_spin,0);		//自旋锁初始化
	int serverfd = 0;
	int ret = 0;
	serverfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);	//ipv4  数据流  TCP
	if(serverfd == -1){
		//printf("socket error\n");
		perror("socket error:");
		return -1;
	}
	else printf("socket successful\n");
	
	
	struct sockaddr_in ServerSockAddr;
	memset(&ServerSockAddr,0,sizeof(struct sockaddr_in));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr(argv[1]);
	ServerSockAddr.sin_port = htons(atoi(argv[2]));
	
	//设置服务器端的端口号，用于客户端的连接
	ret = bind(serverfd,(struct sockaddr *)&ServerSockAddr,sizeof(ServerSockAddr));
	if(ret == -1){
		//printf("bind error\n");
		perror("bind error:");
		close(serverfd);
		return -1;
	}
	else printf("bind successful\n");
	
	//开始监听
	ret = listen(serverfd,50);
	if(ret == -1){
		perror("listen error");
		close(serverfd);
		return -1;
	}
	else printf("listen successful\n");

	//打印链表中的用户信息
	pthread_t thread_of_print;
	pthread_create(&thread_of_print,NULL,thread_print_Func,(void *)&ret);

	int ServerSockAddr_len = sizeof(ServerSockAddr);
	while(1){
		ret = accept(serverfd,(struct sockaddr *)&ServerSockAddr, &ServerSockAddr_len);
		if(ret != -1){
			printf("有客户端连接了\n");
			//创建从客户端接受信息的线程
			pthread_t thread_of_rec;
			pthread_create(&thread_of_rec,NULL,thread_rec_Func,(void *)&ret);
			
		}
	}
	return 0;
}