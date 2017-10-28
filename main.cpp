#include <stdio.h>
#include <string.h>
#include "mysql.h"

int main(int argc, char **argv) {
	Mysql db;
	db.connect("test");
	//db.exec("create table _big_data(_name varchar(24), _blog blob)");
	//bool ok = db.exec("insert into _big_data values ('a', 'Here it is a big data A')");
	Statement *s = db.prepare("insert into _big_data values (?, ?)");
	s->setStringParam(0, "D");
	const char *xxx = "This is a string D";
	s->setParam(1, CT_BLOB, (void *)xxx, strlen(xxx));
	s->exec();
	delete s;

	Statement *stmt = db.prepare("select * from _big_data");
	ResultSetMetaData *meta = stmt->getMetaData();
	for (int i = 0; i < meta->getColumnCount(); ++i) {
		printf(" %10s : %2d : %3d \n", meta->getColumnName(i), meta->getColumnSize(i), meta->getColumnType(i));
	}
	delete meta;
	bool sok = stmt->exec();
	while (stmt->fetch()) {
		unsigned long len = 0;
		char *blobData = (char *)(stmt->getRow(1, &len));
		if (blobData) blobData[len] = 0;
		printf(" [%s]  [%s]  \n", stmt->getString(0), blobData);
	}
	delete stmt;

	return 0;
}