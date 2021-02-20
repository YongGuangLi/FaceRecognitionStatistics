#ifndef DATABASEHELPER_H
#define DATABASEHELPER_H


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

#define SingletonDBHelper DataBaseHelper::GetInstance()

class DataBaseHelper : public QObject
{
    Q_OBJECT
public:
    static DataBaseHelper *GetInstance();
    bool open(QString ip,int port, QString dbName, QString user, QString passwd);

    QString getError(); 

	bool readPersonData();

	bool readDepartment();

	bool readDoorFlag();

	QString getAgeByPersonID(int);   
	QString getParentUuidByUuid(QString);
	QString getDepartmentNameByUuid(QString); 
	QString getDepartmentUuidByPersonID(int); 
	QString getAddressByPersonID(int);

	int getDoorFlag(QString);

	QMap<int, stPersonData> getPersonData();
	QMap<QString, QString> getDepartmentParent();  //部门uuid   父部门uuid
	QMap<QString, QString> getDepartmentName();    //部门uuid   部门名
	QMap<QString, int> getDoorFlag();
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
	QMap<QString, QString> mapDepartmentParent;  //部门uuid   父部门uuid
	QMap<QString, QString> mapDepartmentName;    //部门uuid   部门名
	QMap<QString, int> mapDoorFlag;
signals:
    
public slots:
    
};

#endif // DATABASEHELPER_H
