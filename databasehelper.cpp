#include "databasehelper.h"


#define SQL_READPERSON "select personId,certificateNo,deptuuid,address from tb_safety_person"
#define SQL_READDEPARTMENT "select deptuuid,deptname,parentdeptuuid from tb_safety_dept"
#define SQL_READDOORFLAG "select id,enter from tb_door"

DataBaseHelper * DataBaseHelper::dbHelp_ = NULL;

DataBaseHelper *DataBaseHelper::GetInstance()
{
    if(dbHelp_ == NULL)
    {
        dbHelp_ = new DataBaseHelper();
    }
    return dbHelp_;
}


DataBaseHelper::DataBaseHelper(QObject *parent) :
    QObject(parent)
{
}

bool DataBaseHelper::open(QString ip, int port, QString dbName, QString user, QString passwd)
{
    bool isopen = false;
    sqlDatabase = QSqlDatabase::addDatabase("QMYSQL");
    if(sqlDatabase.isValid())
    {
        sqlDatabase.setHostName(ip);
        sqlDatabase.setPort(port);
        sqlDatabase.setDatabaseName(dbName);
        sqlDatabase.setUserName(user);
        sqlDatabase.setPassword(passwd);

        ip_ = ip;
        port_ = port;
        dbName_ = dbName;
        user_ = user;
        passwd_ = passwd;

        if(isopen = sqlDatabase.open())
        {
            qDebug()<<"Mysql Connect Success:"<<ip<<port;
        }
    }
    return isopen;
}


QString DataBaseHelper::getError()
{
    return sqlDatabase.lastError().text();
}
 

bool DataBaseHelper::readPersonData()
{
	bool result = false;
	QSqlQuery query;
	result = query.exec(SQL_READPERSON); 
	while(query.next())
	{  
		stPersonData personData;
		personData.personID = query.value(0).toInt();
		personData.certificateNo = query.value(1).toString();
		personData.deptuuid = query.value(2).toString();  
		personData.address = query.value(3).toString(); 
		mapPersonData[personData.personID] =  personData; 
	}
	return result;
}

bool DataBaseHelper::readDepartment()
{ 
	bool result = false;
	QSqlQuery query;
	result = query.exec(SQL_READDEPARTMENT); 
	while(query.next())
	{ 
		QString deptuuid = query.value(0).toString();
		QString deptname = query.value(1).toString();
		QString parentdeptuuid = query.value(2).toString();   
	 
		mapDepartmentParent.insert(deptuuid, parentdeptuuid);

		mapDepartmentName.insert(deptuuid, deptname); 
	}
	return result;
}

bool DataBaseHelper::readDoorFlag()
{ 
	bool result = false;
	QSqlQuery query;
	result = query.exec(SQL_READDOORFLAG); 

	while(query.next())
	{ 
		QString id = query.value(0).toString();
		int enter = query.value(1).toInt();  
		mapDoorFlag.insert(id, enter);
	}
	return result;
}

QString DataBaseHelper::getAgeByPersonID(int personID)
{
	stPersonData personData = mapPersonData.value(personID);
	 
	QDate currentDate = QDate::currentDate();
	QDate personBirthday = QDate::fromString(personData.certificateNo.mid(6,8), "yyyyMMdd");
	 
	int age = personBirthday.daysTo(currentDate) / 365;
	if (age <= 10 || age >= 85) 
		return QString(""); 
	else 
		return QString::number(age);  
} 

QString DataBaseHelper::getParentUuidByUuid(QString Uuid)
{
	QString parentUuid = mapDepartmentParent.value(Uuid, "");
	return parentUuid;
}

QString DataBaseHelper::getDepartmentNameByUuid(QString Uuid)
{
	QString departmentName = mapDepartmentName.value(Uuid, "");
	return departmentName;
}

QString DataBaseHelper::getDepartmentUuidByPersonID(int personID)
{
	stPersonData personData =  mapPersonData.value(personID);   
	return personData.deptuuid;
}


QString DataBaseHelper::getAddressByPersonID(int personID)
{
	stPersonData personData =  mapPersonData.value(personID);   
	return personData.address;
}

int DataBaseHelper::getDoorFlag(QString id)
{
	int result = mapDoorFlag.value(id, -1);
	return result;
}

QMap<QString, int> DataBaseHelper::getDoorFlag()
{
	return mapDoorFlag;
}

QMap<int, stPersonData> DataBaseHelper::getPersonData()
{
	return mapPersonData;
}

QMap<QString, QString> DataBaseHelper::getDepartmentParent()
{
	return mapDepartmentParent;
}

QMap<QString, QString> DataBaseHelper::getDepartmentName()
{
	return mapDepartmentName;
}
