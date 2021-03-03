#ifndef DATABASEHELPER_H
#define DATABASEHELPER_H

#include "log4cplus.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QDebug>
#include <QMap>
#include <QList>
#include <QSqlQuery>
#include <QTimer>
#include <QDateTime>


typedef struct
{
	int personID;
	QString certificateNo;
	QString deptuuid;
	QString address;
}stPersonData;

typedef struct
{
	QString deptuuid;
	QString deptname;
	QString parentdeptuuid;  
}stDepartmentData;


#define SingletonDBHelper DataBaseHelper::GetInstance()

class DataBaseHelper : public QObject
{
    Q_OBJECT
public:
    static DataBaseHelper *GetInstance();
    bool open(QString ip,int port, QString dbName, QString user, QString passwd);

	bool isopen(); 

	QString getError();

	bool readPersonData();

	bool readDepartment();

	bool readDoorFlag();

	QString getAgeByPersonID(int);   
	QString getParentUuidByChildUuid(QString);

	QString getDeptNameByUuid(QString); 
	QString getDeptUuidByPersonID(int); 
	QString getDeptUuidByDeptName(QString); 

	QString getAddressByPersonID(int);

	int getDoorFlag(QString); 
private:
    explicit DataBaseHelper(QObject *parent = 0);
    static DataBaseHelper * dbHelp_;

    QSqlDatabase sqlDatabase;
    QString ip_;
    int port_;
    QString dbName_;
    QString user_;
    QString passwd_;
	QMap<int, stPersonData> mapPersonData;
	QMap<QString, stDepartmentData> mapDepartmentData;  
	QMap<QString, int> mapDoorFlag;
signals:
    
public slots:
    
};

#endif // DATABASEHELPER_H
