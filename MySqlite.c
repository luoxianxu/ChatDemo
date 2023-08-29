
#include "MySqlite.h"

/*

//卡号   姓名   楼栋单元   房间号   是否在小区内
#define TableName			"CREATE TABLE Rfid_Table("  \
							"ID INTEGER PRIMARY KEY NOT NULL," \
							"Name char(20) not NULL ," \
							"Build_Number  int NOT NULL," \
							"Room_Number int NOT NULL," \
							"Status char(5) CHECK(Status=='YES' OR Status=='NO'));"

#define InsertTable			"insert into Rfid_Table values(12487184,'张三',1,1,'YES');"

*/

//创建或打开数据库
void CreateOrOpenDataBase(char *db_name,sqlite3 **database){
	int ret = 0;
	char *zErrMsg;
	ret = sqlite3_open(db_name,database);
	if(ret){
		printf("database open error:%s\n",sqlite3_errmsg(*database));
		return;
	}
	else printf("database open successful\n");
}

//创建表
void CreateTable(sqlite3 *database,char *CreateTableGrammer){
	int ret = 0;
	char *zErrMsg;
	printf("hello\n");
	printf("grammer = %s\n",CreateTableGrammer);
	ret = sqlite3_exec(database,CreateTableGrammer,NULL,NULL,&zErrMsg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "Table created successfully\n");
	}
}

//数据库语法 except Select
void DataBaseGrammer(sqlite3 *database,char *grammer){
	int ret = 0;
	char *zErrMsg;
	ret = sqlite3_exec(database,grammer,NULL,NULL,&zErrMsg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "SQL successfully\n");
	}
}

//数据库查找语法
void DataBaseSelectGrammer(sqlite3 *database,char *grammer,int (*callback)(void*,int,char**,char**)){
	int ret = 0;
	char *zErrMsg;
	ret = sqlite3_exec(database,grammer,callback,NULL,&zErrMsg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "Table Select successfully\n");
	}
}

//数据库关闭语法
void DataBaseClose(sqlite3 *database){
	sqlite3_close(database);
}

