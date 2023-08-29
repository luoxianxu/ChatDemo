
#ifndef _MYSQLITE_H
#define _MYSQLITE_H

#include <stdio.h>
#include "sqlite3.h"

void CreateOrOpenDataBase(char *db_name,sqlite3 **database);
void CreateTable(sqlite3 *database,char *CreateTableGrammer);
void DataBaseGrammer(sqlite3 *database,char *grammer);
void DataBaseSelectGrammer(sqlite3 *database,char *grammer,int (*callback)(void*,int,char**,char**));
void DataBaseClose(sqlite3 *database);

#endif /*_MYSQLITE_H*/

