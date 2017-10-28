#pragma once

enum ColumnType {
	CT_DECIMAL, CT_TINY, CT_SHORT, CT_LONG, CT_FLOAT,  CT_DOUBLE, CT_NULL, CT_TIMESTAMP,
	CT_LONGLONG,CT_INT24, CT_DATE, CT_TIME, CT_DATETIME, CT_YEAR,
	CT_NEWDATE, CT_VARCHAR, CT_BIT, CT_NEWDECIMAL=246, CT_ENUM=247, CT_SET=248,
	CT_TINY_BLOB=249, CT_MEDIUM_BLOB=250, CT_LONG_BLOB=251, CT_BLOB=252,
	CT_VAR_STRING=253, CT_STRING=254, CT_GEOMETRY=255
};

class ResultSetMetaData {
public:
	ResultSetMetaData(void *obj, bool freeObj);
	~ResultSetMetaData();
	int getColumnCount(); // 列数
	char* getColumnLabel(int column);
	char* getColumnName(int column);
	int getColumnDisplaySize(int column); // 列宽(显示数)
	int getColumnSize(int column); // 列宽(字节数)
	ColumnType getColumnType(int column);
private:
	void *mObj;
	bool mFreeObj;
};

class ResultSet {
public:
	ResultSet(void *obj);
	~ResultSet();
	ResultSetMetaData *getMetaData();
	int getRowsCount(); // 总行数
	unsigned long * getColumnsLength(); // 当前行各列的数据长度, 返回一个数组（每项是一列的长度）
	bool next();
	char *getString(int columnIndex);
	int getInt(int columnIndex);
	long long getInt64(int columnIndex);
	double getDouble(int columnIndex);
private:
	void *mObj;
	void *mRow;
};

class Statement {
public:
	Statement(void *obj);
	~Statement();

	void setIntParam(int paramIdx, int val);
	void setInt64Param(int paramIdx, long long int val);
	void setDoubleParam(int paramIdx, double val);
	void setStringParam(int paramIdx, const char* val);
	void setBlobParam(int paramIdx, void *val, int len);
	void setParam(int paramIdx, ColumnType ct, void *val, int len);
	bool sendBLOB(int paramIdx, void *data, int length); // 调用之前，所有的params必需先设好

	int getParamsCount();
	int getInsertId();
	int getColumnCount();
	int getRowsCount();
	const char* getError();
	ResultSetMetaData* getMetaData();

	bool reset(bool clearBLOB = 0);
	bool exec();
	bool fetch();

	char *getString(int columnIndex);
	int getInt(int columnIndex);
	long long getInt64(int columnIndex);
	double getDouble(int columnIndex);
	void *getRow(int columnIndex, unsigned long *len = NULL);
	
private:
	void setResult(int colIdx, ColumnType ct, int maxLen);
	void *mObj;
	void *mParams;
	void *mResults;
	class Buffer;
	Buffer *mParamBuf;
	Buffer *mResBuf;
	bool mHasBindParam;
	bool mHasBindResult;
	int mParamsCount;
	int mResultColCount;
};

class Mysql {
public:
	Mysql();
	~Mysql();
	void connect(const char *db);
	bool selectDatabase(const char *db);
	void close();
	
	int getAffectedRows();
	int getInsertId();
	const char *getError();
	
	bool exec(const char *sql);
	ResultSet *query(const char *sql);
	Statement *prepare(const char *sql);
	
	void setAutoCommit(bool autoMode);
	bool commit();
	bool rollback();

	// 为当前连接返回默认的字符集
	const char *getCharset();
	bool setCharset(const char *charsetName);

	bool createDatabase(const char *dbName);
	// mysql_use_result -> mysql_fetch_row
	
private:
	void *mObj;
};
















