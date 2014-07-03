#pragma once

#include "tablehuman.h"

namespace sql
{
	class Statement;
	class ResultSet;
};

class table_human_oper
{
public:
	table_human_oper();
	table_human_oper(const table_human&) = delete;
	table_human_oper& operator= (const table_human&) = delete;
	void load();
	void save(const table_human& param);
	void query_by_guid_k(long val);
	void delete_by_guid_k(long val); 
	void query_by_name_q(char* val);
	void delete_by_name_q(char* val);
	const table_human& get_table_human() const;

private:
	table_human _table; 
	sql::Statement* _stmt;
	sql::ResultSet* _result;
};