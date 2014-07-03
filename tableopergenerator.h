#pragma once

#include <map>
#include <vector>
#include <string>

enum data_type
{
	type_int,
	type_uint,
};

class fstream;
class table_oper_generator
{
	struct column_info
	{
		column_info()
			: _length(0)
		{}
		std::string _type;
		std::string _name;
		int _length;
	};

	typedef std::string table_name_t;
	typedef std::string struct_name_t;
	typedef std::vector<column_info> row_info_t;

public:
	table_oper_generator(const char* filename);
	void run();
	void read();
	void generate();
	void generate_header(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void generate_procedure(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void generate_cpp(std::fstream& f, const table_name_t& name, const row_info_t& row);

	void parse(char* buf, struct_name_t& structname, table_name_t& tablename, row_info_t& row);
	int parse_column(char* line, column_info& col);
	void parse_struct_name(char* line, struct_name_t& name);
	void parse_table_name(char* line, table_name_t& name);
	void parse_enum(char* line);
	void parse_column_name();

private:
	void create_table(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void insert_record(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void update_record(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void query_record(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void delete_record(std::fstream& f, const table_name_t& name, const row_info_t& row);
	void load_table(std::fstream& f, const table_name_t& name);
	const char* _filename;
	std::map<table_name_t, row_info_t> _tablemap;
	std::map<struct_name_t, row_info_t> _structmap;
	std::map<std::string, int> _enummap;
};
