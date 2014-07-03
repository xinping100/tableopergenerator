DROP PROCEDURE IF EXISTS table_human_update;
create PROCEDURE table_human_update (columncount INT, guid_k BIGINT, name_q VARCHAR(28), age INT, sex INT, level INT, id INT, power INT)
BEGIN
	DECLARE colcount int DEFAULT 0;
	DECLARE colid int DEFAULT 0;

	select count(*) into colcount from information_schema.columns where table_schema='xpdb'and table_name='table_human';
	IF colcount = 0 THEN
		create table table_human (_guid_k BIGINT primary key auto_increment, _name_q VARCHAR(28), _age INT, _sex INT, _level INT, _id INT, _power INT);
		insert into table_human values (guid_k, name_q, age, sex, level, id, power);
	ELSEIF colcount <> columncount THEN
		DROP TABLE table_human; 
		create table table_human (_guid_k BIGINT primary key auto_increment, _name_q VARCHAR(28), _age INT, _sex INT, _level INT, _id INT, _power INT);
		insert into table_human values (guid_k, name_q, age, sex, level, id, power);
	ELSE
		SELECT _guid_k INTO colid from table_human WHERE _guid_k = guid_k; 
		IF colid > 0 THEN
			update table_human set _guid_k = guid_k, _name_q = name_q, _age = age, _sex = sex, _level = level, _id = id, _power = power;
		ELSE
			insert into table_human values (guid_k, name_q, age, sex, level, id, power);
		END IF;
	END IF;
END;

DROP PROCEDURE IF EXISTS table_human_query_by_guid_k; 
CREATE PROCEDURE table_human_query_by_guid_k(guid_k BIGINT)
begin
	SELECT * from table_human where _guid_k = guid_k; 
end; 

DROP PROCEDURE IF EXISTS table_human_query_by_name_q; 
CREATE PROCEDURE table_human_query_by_name_q(name_q VARCHAR)
begin
	SELECT * from table_human where _name_q = name_q; 
end; 

DROP PROCEDURE IF EXISTS table_human_delete_by_guid_k; 
CREATE PROCEDURE table_human_delete_by_guid_k(guid_k BIGINT)
begin
	SELECT * from table_human where _guid_k = guid_k; 
end; 

DROP PROCEDURE IF EXISTS table_human_delete_by_name_q; 
CREATE PROCEDURE table_human_delete_by_name_q(name_q VARCHAR)
begin
	SELECT * from table_human where _name_q = name_q; 
end; 

DROP PROCEDURE IF EXISTS table_human_load; 
CREATE PROCEDURE table_human_load( ) 
begin
	SELECT * from table_human; 
end; 

