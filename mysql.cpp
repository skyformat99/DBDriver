#include <windows.h>
#include <stdio.h>
#include "mysql.h"
#include "D:/Program Files/MySQL/MySQL Server 5.1/include/mysql.h"

#pragma comment(lib, "D:/Program Files/MySQL/MySQL Server 5.1/lib/opt/libmysql.lib")


ResultSetMetaData::ResultSetMetaData(void *obj, bool freeObj) {
	mObj = obj;
	mFreeObj = freeObj;
}
int ResultSetMetaData::getColumnSize( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mObj, column);
	if (field->type == CT_TINY) return 1;
	if (field->type == CT_SHORT) return 2;
	if (field->type == CT_LONG || field->type == CT_FLOAT) return 4;
	if (field->type == CT_DOUBLE || field->type == CT_LONGLONG) return 8;
	// if (field->type >= CT_TINY_BLOB && field->type <= CT_BLOB) return 0;
	return field->length;
}
int ResultSetMetaData::getColumnDisplaySize( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mObj, column);
	return field->length;
}
ColumnType ResultSetMetaData::getColumnType( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mObj, column);
	return (ColumnType)(field->type);
}
int ResultSetMetaData::getColumnCount() {
	return (int)mysql_num_fields((MYSQL_RES*)mObj);
}
char* ResultSetMetaData::getColumnLabel(int column) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mObj, column);
	return field->name;
}
char* ResultSetMetaData::getColumnName(int column) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mObj, column);
	return field->org_name;
}

ResultSetMetaData::~ResultSetMetaData() {
	if (mFreeObj && mObj)  mysql_free_result((MYSQL_RES*)mObj);
}

// -------------------------------------------------------------------
ResultSet::ResultSet(void *obj) :mObj(obj) ,mRow(0){
}

ResultSet::~ResultSet() {
	if (mObj) mysql_free_result((MYSQL_RES*)mObj);
}

int ResultSet::getRowsCount() {
	return (int)mysql_num_rows((MYSQL_RES*)mObj);
}

bool ResultSet::next() {
	mRow = (void*)mysql_fetch_row((MYSQL_RES*)mObj);
	return mRow != NULL;
}

char *ResultSet::getString(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mRow;
	return row[columnIndex];
}

int ResultSet::getInt(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mRow;
	char *dat = row[columnIndex];
	return atoi(dat);
}

long long ResultSet::getInt64(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mRow;
	char *dat = row[columnIndex];
	return (long long)strtod(dat, 0);
}

double ResultSet::getDouble(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mRow;
	char *dat = row[columnIndex];
	return strtod(dat, 0);
}

unsigned long * ResultSet::getColumnsLength() {
	return mysql_fetch_lengths((MYSQL_RES*)mObj);
}

ResultSetMetaData *ResultSet::getMetaData() {
	return new ResultSetMetaData(mObj, false);
}

//-----------------------------------------------------------
class Statement::Buffer {
public:
	Buffer(int sz) {
		mLen = 0;
		mCapacity = sz;
		mBuf = 0;
	}
	~Buffer() {
		free(mBuf);
	}
	void createBuf() {
		if (mBuf == 0) mBuf = (char*)malloc(mCapacity);
	}
	char *curBuf() {
		return mBuf + mLen;
	}
	void clear() {
		mLen = 0;
	}
	void inc(int sz) {
		mLen += sz;
	}
	void* append(int v) {
		createBuf();
		char *cur = curBuf();
		memcpy(cur, &v, sizeof(int));
		inc(sizeof(int));
		return cur;
	}
	void* append(long long v) {
		createBuf();
		char *cur = curBuf();
		memcpy(cur, &v, sizeof(long long));
		inc(sizeof(long long));
		return cur;
	}
	void* append(double v) {
		createBuf();
		char *cur = curBuf();
		memcpy(curBuf(), &v, sizeof(double));
		inc(sizeof(double));
		return cur;
	}
	void* append(const char* v) {
		createBuf();
		char *cur = curBuf();
		if (v == NULL) v = "";
		int len = strlen(v) + 1;
		strcpy(cur, v);
		inc(len);
		return cur;
	}
	void* appendLen(int len) {
		createBuf();
		char *cur = curBuf();
		inc(len);
		return cur;
	}

	char *mBuf;
	int mCapacity;
	int mLen;
};

Statement::Statement(void *obj) : mObj(obj) {
	mParamsCount = getParamsCount();
	mResultColCount = getColumnCount();
	int SZ = sizeof(MYSQL_BIND) * mParamsCount;
	mParams = malloc(SZ);
	memset(mParams, 0, SZ);
	SZ = sizeof(MYSQL_BIND) * mResultColCount;
	mResults = malloc(SZ);
	memset(mResults, 0, SZ);
	mParamBuf = new Buffer(256);
	mHasBindParam = FALSE;
	mHasBindResult = FALSE;

	mResBuf = NULL;
	ResultSetMetaData *rs = getMetaData();
	if (rs == NULL) return;
	int resBufLen = 0;
	for (int i = 0; i < mResultColCount; ++i) {
		ColumnType ct = rs->getColumnType(i);
		bool isBlob = ct >= CT_TINY_BLOB && ct <= CT_BLOB;
		resBufLen += rs->getColumnSize(i);
		resBufLen += 16;
	}
	mResBuf = new Buffer(resBufLen);

	for (int i = 0; i < mResultColCount; ++i) {
		ColumnType ct = rs->getColumnType(i);
		bool isBlob = ct >= CT_TINY_BLOB && ct <= CT_BLOB;
		setResult(i, ct, rs->getColumnSize(i));
	}
	delete rs;
}

Statement::~Statement() {
	if (mObj) {
		mysql_stmt_free_result((MYSQL_STMT*)mObj);
		mysql_stmt_close((MYSQL_STMT*)mObj);
	}
	delete mParamBuf;
	delete mResBuf;
}

void Statement::setIntParam(int paramIdx, int val) {
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_LONG;
	b->buffer = mParamBuf->append(val);
}
void Statement::setInt64Param(int paramIdx, long long int val) {
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_LONGLONG;
	b->buffer = mParamBuf->append(val);
}
void Statement::setDoubleParam(int paramIdx, double val) {
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_DOUBLE;
	b->buffer = mParamBuf->append(val);
}
void Statement::setStringParam(int paramIdx, const char* val) {
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_STRING;
	b->buffer = (void *)val;
	b->buffer_length = val ? strlen(val) : 0;
}
void Statement::setParam( int paramIdx, ColumnType ct, void *val, int len ) {
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = enum_field_types(ct);
	b->buffer = val;
	b->buffer_length = len;
}
int Statement::getParamsCount() {
	return (int)mysql_stmt_param_count((MYSQL_STMT*)mObj);
}
void Statement::setResult(int colIdx, ColumnType ct, int maxLen) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + colIdx;
	b->buffer_type = enum_field_types(ct);
	b->length = (unsigned long *)mResBuf->appendLen(sizeof(unsigned long *));
	b->buffer_length = maxLen;
	b->buffer = mResBuf->appendLen(maxLen + 2);
}

bool Statement::reset(bool clearBLOB) {
	const int SZ = sizeof(MYSQL_BIND) * mParamsCount;
	memset(mParams, 0, SZ);
	// mParamBuf->clear();
	// mResBuf->clear();
	mHasBindParam = false;
	mysql_stmt_free_result((MYSQL_STMT*)mObj);
	if (clearBLOB) {
		return mysql_stmt_reset((MYSQL_STMT*)mObj) == 0;
	}
	return TRUE;
}
bool Statement::exec() {
	bool ok = true;
	if (mParamsCount > 0 && !mHasBindParam) {
		mHasBindParam = true;
		ok = mysql_stmt_bind_param((MYSQL_STMT*)mObj, (MYSQL_BIND*)mParams) == 0;
	}
	if (ok && mResultColCount > 0) ok = mysql_stmt_bind_result((MYSQL_STMT*)mObj, (MYSQL_BIND*)mResults) == 0;
	if (ok) ok = mysql_stmt_execute((MYSQL_STMT*)mObj) == 0;
	if (ok && mResultColCount > 0) ok = mysql_stmt_store_result((MYSQL_STMT*)mObj) == 0; // mysql_stmt_result_metadata((MYSQL_STMT*)mObj) != NULL
	return ok;
}

bool Statement::fetch() {
	int cc = mysql_stmt_fetch((MYSQL_STMT*)mObj);
	return cc == 0;
}
char *Statement::getString(int columnIndex) {
	static char empty[4] = {0};
	*empty = 0;
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return empty;
	if (b->buffer_type == MYSQL_TYPE_VAR_STRING || b->buffer_type == MYSQL_TYPE_STRING) {
		char *p = (char*)b->buffer;
		unsigned long len = *(b->length);
		p[len] = p[len + 1] = 0;
		return p;
	}
	return empty;
}
int Statement::getInt(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_LONG)
		return *(int*)(b->buffer);
	if (b->buffer_type == MYSQL_TYPE_TINY)
		return *(char *)b->buffer;
	if (b->buffer_type == MYSQL_TYPE_SHORT)
		return *(short *)b->buffer;
	return 0;
}
long long Statement::getInt64(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_LONGLONG)
		return *(long long*)(b->buffer);
	return 0;
}
double Statement::getDouble(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_DOUBLE)
		return *(double*)(b->buffer);
	if (b->buffer_type == MYSQL_TYPE_FLOAT)
		return *(float*)(b->buffer);
	return 0;
}
void * Statement::getRow( int columnIndex, unsigned long *len) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (len) *len = *b->length;
	if (b->is_null_value)
		return 0;
	return b->buffer;
}
int Statement::getInsertId() {
	return (int)mysql_stmt_insert_id((MYSQL_STMT*)mObj);
}
int Statement::getColumnCount() {
	return (int)mysql_stmt_field_count((MYSQL_STMT*)mObj);
}
int Statement::getRowsCount() {
	return (int)mysql_stmt_num_rows((MYSQL_STMT*)mObj);
}
const char* Statement::getError() {
	return mysql_stmt_error((MYSQL_STMT*)mObj);
}
ResultSetMetaData* Statement::getMetaData() {
	void *d = mysql_stmt_result_metadata((MYSQL_STMT*)mObj);
	if (d == 0) return 0;
	return new ResultSetMetaData(d, true);
}

bool Statement::sendBLOB( int paramIdx, void *data, int length ) {
	bool ok = true;
	if (mParamsCount > 0 && !mHasBindParam) {
		mHasBindParam = true;
		ok = mysql_stmt_bind_param((MYSQL_STMT*)mObj, (MYSQL_BIND*)mParams) == 0;
		if (! ok) return false;
	}
	return mysql_stmt_send_long_data((MYSQL_STMT*)mObj, paramIdx, (const char *)data, length) == 0;
}

// ----------------------------------------------------------
Mysql::Mysql() {
	mObj = malloc(sizeof (MYSQL));
	mysql_init((MYSQL*)mObj);
}

Mysql::~Mysql() {
	mysql_close((MYSQL*)mObj);
	free(mObj);
}

int Mysql::getAffectedRows() {
	return (int)mysql_affected_rows((MYSQL*)mObj);
}

int Mysql::getInsertId() {
	return (int)mysql_insert_id((MYSQL*)mObj);
}

const char* Mysql::getError() {
	return mysql_error((MYSQL*)mObj);
}

bool Mysql::setCharset(const char *charsetName) {
	return mysql_set_character_set((MYSQL*)mObj, charsetName) == 0;
}

void Mysql::connect(const char *db) {
	mysql_real_connect((MYSQL*)mObj, "localhost", "root", "root", db, 3306, 0, 0);
}

bool Mysql::selectDatabase(const char *db) {
	return mysql_select_db((MYSQL*)mObj, db) == 0;
}

void Mysql::close() {
	mysql_close((MYSQL*)mObj);
}

bool Mysql::exec(const char *sql) {
	return mysql_query((MYSQL*)mObj, sql) == 0;
}

ResultSet *Mysql::query(const char *sql) {
	MYSQL_RES *res = NULL;
	int code = mysql_query((MYSQL*)mObj, sql);
	if (code != 0) return NULL;
	res = mysql_store_result((MYSQL*)mObj); // 一次读取全部数据
	if (res == NULL) return NULL;
	return new ResultSet((void*)res);

	// mysql_use_result -> mysql_fetch_row  // 每次调用mysql_fetch_row时，才读取数据; 用ysql_errno判断是否读取成功
}

Statement *Mysql::prepare(const char *sql) {
	MYSQL_STMT *stmt = mysql_stmt_init((MYSQL*)mObj);
	int code = mysql_stmt_prepare(stmt, sql, sql == 0 ? 0 : strlen(sql));
	if (code != 0) {
		printf("Mysql::prepare err: %s\n", getError());
		mysql_stmt_close(stmt); // free stmt ?
		return 0;
	}
	return new Statement(stmt);;
}

void Mysql::setAutoCommit(bool autoMode) {
	mysql_autocommit((MYSQL*)mObj, (my_bool)autoMode);
}

bool Mysql::commit() {
	return mysql_commit((MYSQL*)mObj) == 0;
}

bool Mysql::rollback() {
	return mysql_rollback((MYSQL*)mObj) == 0;
}

const char * Mysql::getCharset() {
	return mysql_character_set_name((MYSQL*)mObj);
}

bool Mysql::createDatabase( const char *dbName ) {
	char sql[48];
	sprintf(sql, "create database %s ", dbName);
	return mysql_query((MYSQL*)mObj, sql) == 0;
}












