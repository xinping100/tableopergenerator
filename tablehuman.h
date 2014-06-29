#pragma once

enum
{
	max_name_length = 28,
};

struct equip
{
	int _id;
	int _power;
};

struct table_human
{
	long _guid_k; //primarykey 
	char _name_q[max_name_length]; //query
	int _age;
	int _sex;
	int _level;
	equip _equip;
};

