#include <fstream>
#include <string.h>
#include <exception>
#include <assert.h>
#include <iostream>
#include <string>
#include "tableopergenerator.h"

static std::string sstruct = "struct";
static std::string stable = "table";
static std::string senum = "enum";
static const char* delim = " \t;";
static const char* sep = "= ;";
static const char* arrsep = "[]";
static const char* colsep = "\t_;";
static std::string keytail = "_k";
static std::string querytail = "_q";

static std::map<std::string, std::string> datatypemap =
{
	{ "int", "INT" },
	{ "unsigned int", "INT" },
	{ "long", "BIGINT" },
	{ "unsigned long", "BIGINT" },
	{ "long long", "BIGINT" },
	{ "unsigned long long", "BIGINT" },
	{ "float", "FLOAT" },
	{ "double", "DOUBLE" },
	{ "char", "VARCHAR" },
	{ "char*", "VARCHAR" },
};

static std::map<std::string, std::string> datatypesignmap =
{
	{ "int", "%d" },
	{ "unsigned int", "%d" },
	{ "long", "%ld" },
	{ "unsigned long", "%ld" },
	{ "long long", "%ld" },
	{ "unsigned long long", "%ld" },
	{ "float", "%f" },
	{ "double", "%lf" },
	{ "char", "%s" },
	{ "char*", "%s" },
};

static void trim(std::string& s)
{
	unsigned int index = s.find_first_not_of("\t ;");
	s.erase(0, index);

	index = s.find_last_not_of("\t ;");
	if (index != std::string::npos)
		s.erase(index + 1);
}

static void trim(char*& s)
{
	std::string str = s;
	trim(str);
	memcpy(s, str.c_str(), str.length() + 1);
}

static char* copy_part(char* dst, const char* src, size_t sz)
{
	memset(dst, 0, strlen(dst));
	memcpy(dst, ++src, sz);
	return dst;
}

static bool is_primary_key(const std::string& s)
{
	std::string::size_type pos = s.find(keytail);
	if ((pos != std::string::npos) && pos == (s.length() - 2))
		return true;
	return false;
}

static bool has_alpha(char* c)
{
	while (*c)
	{
		if (isalpha(*c))
			return true;
		++c;
	}
	return false;
}

enum ret_enum
{
	invalid = 0,
	struct_begin = 1,
	struct_end = 2,
	column = 3,
};

table_oper_generator::table_oper_generator(const char* filename)
	: _filename(filename)
{
	
}

void table_oper_generator::run()
{
	read();
	parse_column_name();
	generate();
}

void table_oper_generator::read()
{
	try
	{
		std::fstream f;
		f.open(_filename);
		if (!f.is_open())
		{
			std::cout << "f open failed. filename = " << _filename << std::endl;
			assert(false);
		}

		char buf[1024] = {};
		struct_name_t structname;
		table_name_t tablename;
		row_info_t row; 
		while (f.getline(buf, sizeof(buf), '\n').good())
		{
			if (buf[0] == '#' || strcmp(buf, "") == 0)
				continue;

			parse(buf, structname, tablename, row);
			memset(buf, 0, sizeof(buf));
		}
		f.close();
	}
	catch (std::exception& e)
	{
		std::cout << "read exception occurs: " << e.what() << std::endl;
	}
}

void table_oper_generator::parse(char* buf, struct_name_t& structname, table_name_t& tablename, row_info_t& row)
{
	std::string line = buf;
	if (line.find(senum) != std::string::npos || line.find("{") != std::string::npos)
		return;
	if (line.find(sstruct) != std::string::npos && line.find(stable) != std::string::npos)
		parse_table_name(buf, tablename);
	else if (line.find(sstruct) != std::string::npos)
		parse_struct_name(buf, structname);
	else
	{
		if (line.find_first_of('=') != std::string::npos)
		{
			parse_enum(buf);
			return;
		}

		column_info col;
		int ret = parse_column(buf, col);
		if (ret == column)
		{
			row.push_back(col);
		}
		else if (ret == struct_end)
		{
			if (structname.length() > 0)
			{
				_structmap.insert(std::make_pair(structname, row));
				structname.clear();
				row.clear();
			}
			else if (tablename.length() > 0)
			{
				_tablemap.insert(std::make_pair(tablename, row));
				tablename.clear();
				row.clear();
			}
		}
	}
}

int table_oper_generator::parse_column(char* line, column_info& col)
{
	char* name = strstr(line, "_");
	char* type = line;
	if (name)
	{
		*(name-1) = 0;
		char* extra = strstr(name, ";");
		if (extra)
			*extra = 0;
		trim(name);
	}
		
	if (type)
		trim(type);

	if (strcmp(type, "{") == 0)
		return struct_begin;
	else if (strcmp(type, "}") == 0)
		return struct_end;
	else
	{
		if (name)
			col._name = std::string(name);
		if (type)
			col._type = std::string(type);
		return column;
	}
}

void table_oper_generator::parse_struct_name(char* line, struct_name_t& name)
{
	char* nexttoken = nullptr;
#ifdef _LINUX_
	const char* s = strtok_r(line, delim, &nexttoken);
	name = strtok_r(nullptr, delim, &nexttoken);
#else
	const char* s = strtok_s(line, delim, &nexttoken);
	name = strtok_s(nullptr, delim, &nexttoken);
#endif
	trim(name);
}

void table_oper_generator::parse_table_name(char* line, table_name_t& name)
{
	char* nexttoken = nullptr;
#ifdef _LINUX_
	const char* s = strtok_r(line, delim, &nexttoken);
	name = strtok_r(nullptr, delim, &nexttoken);
#else
	const char* s = strtok_s(line, delim, &nexttoken);
	name = strtok_s(nullptr, delim, &nexttoken);
#endif
	trim(name);
}

void table_oper_generator::parse_enum(char* line)
{
	char* nexttoken = nullptr;
#ifdef _LINUX_
	std::string name = strtok_r(line, sep, &nexttoken);
	const char* val = strtok_r(nullptr, sep, &nexttoken);
#else
	std::string name = strtok_s(line, sep, &nexttoken);
	const char* val = strtok_s(nullptr, sep, &nexttoken);
#endif
	trim(name);
	int i = atoi(val);
	_enummap.insert(std::make_pair(name, i));
}

void table_oper_generator::parse_column_name()
{
	for (auto& a : _tablemap)
	{
		row_info_t& row = a.second;
		for (auto& r : row)
		{
			if (r._name.find('[') == std::string::npos)
				continue;

			const char* crawname = r._name.c_str();
			char* rawname = new char[strlen(crawname)]();
			memcpy(rawname, crawname, strlen(crawname));
			char* nexttoken = nullptr;
#ifdef _LINUX_
			char* cname = strtok_r(rawname, arrsep, &nexttoken);
			char* clen = strtok_r(nullptr, arrsep, &nexttoken);
#else
			char* cname = strtok_s(rawname, arrsep, &nexttoken);
			char* clen = strtok_s(nullptr, arrsep, &nexttoken);
#endif

			unsigned int len = 0;
			if (clen)
			{
				if (has_alpha(clen))
				{
					for (auto& e : _enummap)
					{
						if (e.first == std::string(clen))
						{
							len = e.second;
							break;
						}
					}
				}
				else
					len = atoi(clen);
			}

			r._length = len;
			r._name.clear();
			r._name = cname;
			r._type.append("*");
			delete rawname;
		}
	}
}

void table_oper_generator::generate()
{
	for (auto a = _tablemap.begin(); a != _tablemap.end(); ++a)
	{
		table_name_t name = a->first;
		row_info_t row = a->second;
		std::string header = name;
		std::string fname;
		std::string::size_type pos = 0;
		while ((pos = header.find("_")) != std::string::npos)
			header.erase(pos, 1);
		fname = header;
		header += "oper.h";

		std::fstream headerf;
		headerf.open(header.c_str(), std::ios_base::out);
		if (headerf.is_open())
			generate_header(headerf, name, row);
		else
		{
			std::cout << "header file open failed. filename = " << header.c_str() << std::endl;
			headerf.close();
			assert(false);
		}

		headerf.close();

		std::string procedure = fname + "procedure.sql";
		std::fstream proceduref;
		proceduref.open(procedure.c_str(), std::ios_base::out);
		if (proceduref.is_open())
			generate_procedure(proceduref, name, row);
		else
		{
			std::cout << "header file open failed. filename = " << procedure.c_str() << std::endl;
			proceduref.close();
			assert(false);
		}
		proceduref.close();

		std::string cpp = fname + "oper.cpp";
		std::fstream cppf;
		cppf.open(cpp.c_str(), std::ios_base::out);
		if (cppf.is_open())
			generate_cpp(cppf, name, row);
		else
		{
			std::cout << "header file open failed. filename = " << cpp.c_str() << std::endl;
			cppf.close();
			assert(false);
		}
		cppf.close();
	}
}

void table_oper_generator::generate_header(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "#pragma once\n";
	f << "\n";
	table_name_t cname = name;
	cname.erase(cname.find('_'), 1);
	f << "#include \"" << cname.c_str() << ".h\"\n";
	f << "\n";
	f << "namespace sql\n";
	f << "{\n";
	f << "\tclass Statement;\n";
	f << "\tclass ResultSet;\n";
	f << "};\n";
	f << "\n";
	f << "class " << name.c_str() << "_oper\n";
	f << "{\n";
	f << "public:\n";
	f << "\t" << name.c_str() << "_oper();\n";
	f << "\t" << name.c_str() << "_oper(const " << name.c_str() << "&) = delete;\n";
	f << "\t" << name.c_str() << "_oper& operator= (" << "const " << name.c_str() << "&) = delete;\n";
	f << "	void load();\n";
	f << "	void save(const " << name.c_str() << "& param);\n";
	
	std::string key;
	std::string ktype;
	std::string query;
	std::string qtype;
	for (auto a = row.begin(); a != row.end(); ++a)
	{
		unsigned int bpos = a->_name.find_last_of('_');
		std::string tail = a->_name.substr(bpos);
		if (tail == keytail)
		{
			key = a->_name;
			ktype = a->_type;
		}		
		else if (tail == querytail)
		{
			query = a->_name;
			qtype = a->_type;
		}	
	}

	if (key.length() > 0)
	{
		f << "	void query_by" << key << "(" << ktype << " val);\n";
		f << "	void delete_by" << key << "(" << ktype <<" val); \n";
	}
		
	if (query.length() > 0)
	{
		f << "	void query_by" << query << "(" << qtype << " val);\n";
		f << "	void delete_by" << query << "(" << qtype << " val);\n";
	}

	f << "\tconst " << name.c_str() << "& get_" << name.c_str() << "() const;\n";
	f << "\n";
	f << "private:\n";
	f << "\t" << name.c_str() << " _table; \n";
	f << "	sql::Statement* _stmt;\n";
	f << "	sql::ResultSet* _result;\n";
	f << "};";
	f.flush();
}

void table_oper_generator::generate_procedure(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_update;\n";
	f << "create PROCEDURE " << name.c_str() << "_update (columncount INT, ";
	unsigned int index = 0;
	char buf[64] = {};
	for (auto& a : row)
	{
		bool bend = index < row.size() - 1 ? false : true;
		if (datatypemap.find(a._type) != datatypemap.end())
		{
			f << copy_part(buf, a._name.c_str(), a._name.length()) << " " << datatypemap[a._type].c_str();
			if (datatypemap[a._type] == std::string("VARCHAR"))
			{
				f << "(" << a._length << ")";
			}
		}
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{
					f << copy_part(buf, c._name.c_str(), c._name.length()) << " " << datatypemap[c._type].c_str();
					if (datatypemap[c._type] == std::string("VARCHAR"))
						f << "(" << c._length << ")";
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "] format is wrong. " << std::endl;
				return;
			}
		}

		if (!bend)
			f << ", ";
		else
			f << ")\n";
		++index;
	}

	f << "BEGIN\n";
	f << "\tDECLARE colcount int DEFAULT 0;\n";
	f << "\tDECLARE colid int DEFAULT 0;\n";
	f << "\n";
	f << "\tselect count(*) into colcount from information_schema.columns where table_schema='xpdb'and table_name='" << name.c_str() << "';\n";

	f << "\tIF colcount = 0 THEN\n";
	f << "\t";
	create_table(f, name, row);
	f << "\t";
	insert_record(f, name, row);

	f << "\tELSEIF colcount <> columncount THEN\n";
	f << "\t\tDROP TABLE " << name.c_str() << "; \n";
	f << "\t";
	create_table(f, name, row);
	f << "\t";
	insert_record(f, name, row);

	f << "\tELSE\n";
	const char* colname = row.at(0)._name.c_str();
	f << "\t\tSELECT " << colname << " INTO colid from " << name.c_str();
	f << " WHERE " << colname << " = " << copy_part(buf, colname, strlen(colname)) << "; \n";
	f << "\t\tIF colid > 0 THEN\n";
	f << "\t\t";
	update_record(f, name, row);
	f << "\t\tELSE\n";
	f << "\t\t";
	insert_record(f, name, row);
	f << "\t\tEND IF;\n";
	f << "\tEND IF;\n";

	f << "END;\n";
	f << "\n";

	query_record(f, name, row);
	delete_record(f, name, row);
	load_table(f, name);
	f.flush();
}

void table_oper_generator::generate_cpp(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "#include <sstream>\n";
	f << "#include <string>\n";
	f << "#include <iostream>\n";
	f << "#include <stdio.h>\n";
	f << "#include <assert.h>\n";
	f << "#include <cppconn/statement.h>\n";
	f << "#include <cppconn/sqlstring.h>\n";
	std::string::size_type pos = 0;
	std::string hname = name;
	while ((pos = hname.find("_")) != std::string::npos)
		hname.erase(pos, 1);

	f << "#include \"" << hname.c_str() << "oper.h\"\n";
	f << "#include \"dbsql.h\"\n";
	f << "#include \"dbserver.h\"\n";
	f << "\n";
	
	std::string classname = name.c_str();
	classname.append("_oper");
	const char* cclassname = classname.c_str();
	f << cclassname << "::" << cclassname << "()\n";
	f << "\t" << ": _stmt(nullptr), _result(nullptr)\n";
	f << "{\n";
	f << "\t_stmt = dbserver::instance()->get_statement();\n";
	f << "\tif (!_stmt)\n";
	f << "\t{\n";
	f << "\t\tstd::cout << \"" << cclassname << " _stmt is nullptr.\" << std::endl;" << "\n";
	f << "\t\tassert(false);\n";
	f << "\t}\n";
	f << "}\n";
	f << "\n";

	f << "void " << cclassname << "::load()\n";
	f << "{\n";
	f << "\tstd::ostringstream os;\n";
	f << "\t" << "os << \"call " << name.c_str() << "_load" << " ( ); \";\n";
	f << "\t" << "sql::SQLString str;" << "\n";
	f << "\t" << "str.append(os.str());" << "\n";
	f << "\t" << "int ret = dbsql::execute_query(_stmt, str, &_result);" << "\n";
	f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
	f << "\t" << "\t" << "std::cout << \"" << cclassname << " load success.\" << std::endl; " << "\n";
	f << "\t" << "else" << "\n";
	f << "\t" << "\t" << "std::cout << \"" << cclassname << " load fail.\" << std::endl; " << "\n";
	f << "}\n";
	f << "\n";

	std::string tablestructname = name;
	const char* ctablestructname = tablestructname.c_str();
	f << "void " << cclassname << "::save(const " << ctablestructname << "& table)\n";
	f << "{\n";
	f << "\tchar buf[128] = {};\n";
	f << "\tsnprintf(buf, sizeof(buf), \"call " << name.c_str() << "_update" << " (" << row.size() << ", ";

	unsigned index = 0;
	for (auto& a : row)
	{
		if (datatypemap.find(a._type) != datatypemap.end())
		{
			if (datatypemap[a._type] == std::string("VARCHAR"))
				f << "'";
			f << datatypesignmap[a._type];
			if (datatypemap[a._type] == std::string("VARCHAR"))
				f << "'";
		}
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{
					if (datatypemap[c._type] == std::string("VARCHAR"))
						f << "'";
					f << datatypesignmap[c._type];
					if (datatypemap[c._type] == std::string("VARCHAR"))
						f << "'";
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "format is wrong. " << std::endl;
				return;
			}
		}
		if (index < row.size() - 1)
			f << ", ";
		else
			f << "); \", ";
		++index;
	}

	index = 0;
	for (auto& a : row)
	{
		if (datatypemap.find(a._type) != datatypemap.end())
			f << "table." << a._name.c_str();	
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{
					f << "table." << a._name.c_str() << "." << c._name.c_str();
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "format is wrong. " << std::endl;
				return;
			}
		}
		if (index < row.size() - 1)
			f << ", ";
		else
			f << "); \n";
		++index;
	}

	f << "\t" << "std::ostringstream os;" << "\n";
	f << "\t" << "os << buf;" << "\n";
	f << "\t" << "sql::SQLString str;" << "\n";
	f << "\t" << "str.append(os.str());" << "\n";
	f << "\t" << "int ret = dbsql::execute(_stmt, str);" << "\n";
	f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
	f << "\t" << "\t" << "std::cout << \"" << cclassname << " save success.\" << std::endl; " << "\n";
	f << "\t" << "else" << "\n";
	f << "\t" << "\t" << "std::cout << \"" << cclassname << " save fail.\" << std::endl; " << "\n";
	f << "}\n";
	f << "\n";

	std::string key;
	std::string ktype;
	std::string query;
	std::string qtype;
	for (auto a = row.begin(); a != row.end(); ++a)
	{
		unsigned int bpos = a->_name.find_last_of('_');
		std::string tail = a->_name.substr(bpos);
		if (tail == keytail)
		{
			key = a->_name;
			ktype = a->_type;
		}
		else if (tail == querytail)
		{
			query = a->_name;
			qtype = a->_type;
		}
	}

	if (key.length() > 0)
	{
		//key 和query都需要是基本数据类型
		if (datatypesignmap.find(ktype) == datatypesignmap.end())
		{
			std::cout << "key type wrong" << std::endl;
			assert(false);
		}
		f << "void " << cclassname << "::query_by" << key << "(" << ktype << " val)\n";
		f << "{\n";
		f << "\tchar buf[64] = {};\n";
		f << "\tsnprintf(buf, sizeof(buf), \"call " << name.c_str() << "_query_by" << key << "(val); \");\n";
		f << "\tstd::ostringstream os;\n";
		f << "\tos << buf;";
		f << "\t" << "sql::SQLString str;" << "\n";
		f << "\t" << "str.append(os.str());" << "\n";
		f << "\t" << "int ret = dbsql::execute_query(_stmt, str, &_result);" << "\n";
		f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " query_by" << key << " success.\" << std::endl; " << "\n";
		f << "\t" << "else" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " query_by" << key << " fail.\" << std::endl; " << "\n";
		f << "}\n";
		f << "\n";

		f << "void " << cclassname << "::delete_by" << key << "(" << ktype << " val)\n";
		f << "{\n";
		f << "\tchar buf[64] = {};\n";
		f << "\tsnprintf(buf, sizeof(buf), \"call " << name.c_str() << "_delete_by" << key << "(val); \");\n";
		f << "\tstd::ostringstream os;\n";
		f << "\tos << buf;";
		f << "\t" << "sql::SQLString str;" << "\n";
		f << "\t" << "str.append(os.str());" << "\n";
		f << "\t" << "int ret = dbsql::execute_query(_stmt, str, &_result);" << "\n";
		f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " delete_by" << key << " success.\" << std::endl; " << "\n";
		f << "\t" << "else" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " delete_by" << key << " fail.\" << std::endl; " << "\n";
		f << "}\n";
		f << "\n";
	}

	if (query.length() > 0)
	{
		if (datatypesignmap.find(ktype) == datatypesignmap.end())
		{
			std::cout << "query type wrong" << std::endl;
			assert(false);
		}
		f << "void " << cclassname << "::query_by" << query << "(" << qtype << " val)\n";
		f << "{\n";
		f << "\tchar buf[64] = {};\n";
		f << "\tsnprintf(buf, sizeof(buf), \"call " << name.c_str() << "_query_by" << query << "(val); \");\n";
		f << "\tstd::ostringstream os;\n";
		f << "\tos << buf;";
		f << "\t" << "sql::SQLString str;" << "\n";
		f << "\t" << "str.append(os.str());" << "\n";
		f << "\t" << "int ret = dbsql::execute_query(_stmt, str, &_result);" << "\n";
		f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " query_by" << query << " success.\" << std::endl; " << "\n";
		f << "\t" << "else" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " query_by" << query << " fail.\" << std::endl; " << "\n";
		f << "}\n";
		f << "\n";

		f << "void " << cclassname << "::delete_by" << query << "(" << qtype << " val)\n";
		f << "{\n";
		f << "\tchar buf[64] = {};\n";
		f << "\tsnprintf(buf, sizeof(buf), \"call " << name.c_str() << "_delete_by" << query << "(val); \");\n";
		f << "\tstd::ostringstream os;\n";
		f << "\tos << buf;";
		f << "\t" << "sql::SQLString str;" << "\n";
		f << "\t" << "str.append(os.str());" << "\n";
		f << "\t" << "int ret = dbsql::execute_query(_stmt, str, &_result);" << "\n";
		f << "\t" << "if (ret == dbsql::execute_success)" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " delete_by" << query << " success.\" << std::endl; " << "\n";
		f << "\t" << "else" << "\n";
		f << "\t" << "\t" << "std::cout << \"" << cclassname << " delete_by" << query << " fail.\" << std::endl; " << "\n";
		f << "}\n";
		f << "\n";
	}
}

void table_oper_generator::create_table(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "\tcreate table " << name.c_str() << " (";

	unsigned int index = 0;
	for (auto& a : row)
	{
		bool bend = index < row.size() - 1 ? false : true;
		if (datatypemap.find(a._type) != datatypemap.end())
		{
			f << a._name.c_str() << " " << datatypemap[a._type].c_str();
			if (datatypemap[a._type] == std::string("VARCHAR"))
				f << "(" << a._length << ")";
			if (is_primary_key(a._name))
				f << " primary key auto_increment";
		}
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{

					f << c._name.c_str() << " " << datatypemap[c._type].c_str();
					if (datatypemap[c._type] == std::string("VARCHAR"))
						f << "(" << c._length << ")";
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "format is wrong. " << std::endl;
				return;
			}
		}

		if (!bend)
			f << ", ";
		else
		{
			f << ");\n";
		}
		++index;
	}
}

void table_oper_generator::insert_record(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "\tinsert into " << name.c_str() << " values (";
	//todo 暂时只支持到表结构里有其他结构，但不支持其他结构里又嵌套结构。
	unsigned int index = 0;
	char buf[64] = {};
	for (auto& a : row)
	{
		bool bend = index < row.size() - 1 ? false : true;
		if (datatypemap.find(a._type) != datatypemap.end())
			f << copy_part(buf, a._name.c_str(), a._name.length()) ;
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{

					f << copy_part(buf, c._name.c_str(), c._name.length());
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "format is wrong. " << std::endl;
				return;
			}
		}

		if (!bend)
			f << ", ";
		else
			f << ");\n";
		++index;
	}
}

void table_oper_generator::update_record(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	f << "\tupdate " << name.c_str() << " set ";
	unsigned index = 0;
	char buf[64] = {};
	for (auto& a : row)
	{
		bool bend = index < row.size() - 1 ? false : true;
		if (datatypemap.find(a._type) != datatypemap.end())
			f << a._name.c_str() << " = " << copy_part(buf, a._name.c_str(), a._name.length());
		else
		{
			auto structinfo = _structmap.find(a._type);
			if (structinfo != _structmap.end())
			{
				unsigned int pos = 0;
				for (auto& c : structinfo->second)
				{

					f << c._name.c_str() << " = " << copy_part(buf, c._name.c_str(), c._name.length());
					if (pos < structinfo->second.size() - 1)
						f << ", ";
					++pos;
				}
			}
			else
			{
				std::cout << "file [" << _filename << "format is wrong. " << std::endl;
				return;
			}
		}

		if (!bend)
			f << ", ";
		else
			f << ";\n";
		++index;
	}
}

void table_oper_generator::query_record(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	std::string key;
	std::string ktype;
	std::string query;
	std::string qtype;
	for (auto a = row.begin(); a != row.end(); ++a)
	{
		unsigned int bpos = a->_name.find_last_of('_');
		std::string tail = a->_name.substr(bpos);
		if (tail == keytail)
		{
			key = a->_name;
			ktype = a->_type;
		}
		else if (tail == querytail)
		{
			query = a->_name;
			qtype = a->_type;
		}
	}

	char buf[64] = {};
	if (key.length() > 0)
	{
		//key 和query都需要是基本数据类型
		if (datatypesignmap.find(ktype) == datatypesignmap.end())
		{
			std::cout << "key type wrong" << std::endl;
			assert(false);
		}

		f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_query_by" << key << "; \n";
		f << "CREATE PROCEDURE " << name.c_str() << "_query_by" << key << "(" << copy_part(buf, key.c_str(), key.length());
		f << " " <<  datatypemap[ktype] << ")\n";
		f << "begin\n";
		f << "\tSELECT * from " << name.c_str() << " where " << key << " = " << copy_part(buf, key.c_str(), key.length()) << "; \n";
		f << "end; \n";
		f << "\n";
	}

	if (query.length() > 0)
	{
		if (datatypesignmap.find(qtype) == datatypesignmap.end())
		{
			std::cout << "query type wrong" << std::endl;
			assert(false);
		}

		f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_query_by" << query << "; \n";
		f << "CREATE PROCEDURE " << name.c_str() << "_query_by" << query << "(" << copy_part(buf, query.c_str(), query.length());
		f << " " << datatypemap[qtype] << ")\n";
		f << "begin\n";
		f << "\tSELECT * from " << name.c_str() << " where " << query << " = " << copy_part(buf, query.c_str(), query.length()) << "; \n";
		f << "end; \n";
		f << "\n";
	}
}

void table_oper_generator::delete_record(std::fstream& f, const table_name_t& name, const row_info_t& row)
{
	std::string key;
	std::string ktype;
	std::string query;
	std::string qtype;
	for (auto a = row.begin(); a != row.end(); ++a)
	{
		unsigned int bpos = a->_name.find_last_of('_');
		std::string tail = a->_name.substr(bpos);
		if (tail == keytail)
		{
			key = a->_name;
			ktype = a->_type;
		}
		else if (tail == querytail)
		{
			query = a->_name;
			qtype = a->_type;
		}
	}

	char buf[64] = {};
	if (key.length() > 0)
	{
		//key 和query都需要是基本数据类型
		if (datatypesignmap.find(ktype) == datatypesignmap.end())
		{
			std::cout << "key type wrong" << std::endl;
			assert(false);
		}

		f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_delete_by" << key << "; \n";
		f << "CREATE PROCEDURE " << name.c_str() << "_delete_by" << key << "(" << copy_part(buf, key.c_str(), key.length());
		f << " " << datatypemap[ktype] << ")\n";
		f << "begin\n";
		f << "\tSELECT * from " << name.c_str() << " where " << key << " = " << copy_part(buf, key.c_str(), key.length()) << "; \n";
		f << "end; \n";
		f << "\n";
	}

	if (query.length() > 0)
	{
		if (datatypesignmap.find(qtype) == datatypesignmap.end())
		{
			std::cout << "query type wrong" << std::endl;
			assert(false);
		}

		f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_delete_by" << query << "; \n";
		f << "CREATE PROCEDURE " << name.c_str() << "_delete_by" << query << "(" << copy_part(buf, query.c_str(), query.length());
		f << " " << datatypemap[qtype] << ")\n";
		f << "begin\n";
		f << "\tSELECT * from " << name.c_str() << " where " << query << " = " << copy_part(buf, query.c_str(), query.length()) << "; \n";
		f << "end; \n";
		f << "\n";
	}
}

void table_oper_generator::load_table(std::fstream& f, const table_name_t& name)
{
	f << "DROP PROCEDURE IF EXISTS " << name.c_str() << "_load" << "; \n";
	f << "CREATE PROCEDURE " << name.c_str() << "_load" << "( ) \n";
	f << "begin\n";
	f << "\tSELECT * from " << name.c_str() << "; \n";
	f << "end; \n";
	f << "\n";
}
