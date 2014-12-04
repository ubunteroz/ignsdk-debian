#ifndef IGN_ROCKSDB
#define IGN_ROCKSDB

#include <QObject>
#include <string>
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/version.h"

class ignrocksdb : public QObject {
	Q_OBJECT
public:
	explicit ignrocksdb(QObject* parent = 0);
public slots:
	void init(QString wal_dir);
	void close();
	bool put(QString key, QString value);
	void batchPut(QString key, QString value);
	void batchDelete(QString key);
	bool batchWrite();
	QString get(QString key);
	void delete(QString key);
	void flush();
	QString keyMayExist(QString key);
	QString version();
};

#endif