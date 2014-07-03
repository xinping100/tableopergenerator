#include <sstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <cppconn/statement.h>
#include <cppconn/sqlstring.h>
#include "tablehumanoper.h"
#include "dbsql.h"
#include "dbserver.h"

table_human_oper::table_human_oper()
	: _stmt(nullptr), _result(nullptr)
{
	_stmt = dbserver::instance()->get_statement();
	if (!_stmt)
	{
		std::cout << "table_human_oper _stmt is nullptr." << std::endl;
		assert(false);
	}
}

void table_human_oper::load()
{
	std::ostringstream os;
	os << "call table_human_load ( ); ";
	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute_query(_stmt, str, &_result);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper load success." << std::endl; 
	else
		std::cout << "table_human_oper load fail." << std::endl; 
}

void table_human_oper::save(const table_human& table)
{
	char buf[128] = {};
	snprintf(buf, sizeof(buf), "call table_human_update (6, %ld, '%s', %d, %d, %d, %d, %d); ", table._guid_k, table._name_q, table._age, table._sex, table._level, table._equip._id, table._equip._power); 
	std::ostringstream os;
	os << buf;
	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute(_stmt, str);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper save success." << std::endl; 
	else
		std::cout << "table_human_oper save fail." << std::endl; 
}

void table_human_oper::query_by_guid_k(long val)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf), "call table_human_query_by_guid_k(val); ");
	std::ostringstream os;
	os << buf;	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute_query(_stmt, str, &_result);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper query_by_guid_k success." << std::endl; 
	else
		std::cout << "table_human_oper query_by_guid_k fail." << std::endl; 
}

void table_human_oper::delete_by_guid_k(long val)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf), "call table_human_delete_by_guid_k(val); ");
	std::ostringstream os;
	os << buf;	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute_query(_stmt, str, &_result);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper delete_by_guid_k success." << std::endl; 
	else
		std::cout << "table_human_oper delete_by_guid_k fail." << std::endl; 
}

void table_human_oper::query_by_name_q(char* val)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf), "call table_human_query_by_name_q(val); ");
	std::ostringstream os;
	os << buf;	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute_query(_stmt, str, &_result);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper query_by_name_q success." << std::endl; 
	else
		std::cout << "table_human_oper query_by_name_q fail." << std::endl; 
}

void table_human_oper::delete_by_name_q(char* val)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf), "call table_human_delete_by_name_q(val); ");
	std::ostringstream os;
	os << buf;	sql::SQLString str;
	str.append(os.str());
	int ret = dbsql::execute_query(_stmt, str, &_result);
	if (ret == dbsql::execute_success)
		std::cout << "table_human_oper delete_by_name_q success." << std::endl; 
	else
		std::cout << "table_human_oper delete_by_name_q fail." << std::endl; 
}

