#include "databasehelper.h"


#define SQL_SELECT_PERSON "select personId,certificateNo,deptuuid,address from tb_safety_person"
#define SQL_SELECT_DEPARTMENT "select deptuuid,deptname,parentdeptuuid from tb_safety_dept"

#define SQL_SELECT_DEPARTMENTUUID "select deptuuid from tb_safety_dept where deptname = '%1'"

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

		isopen = sqlDatabase.open();
		if (!isopen)
			SingletonLog->warn(QString("%1 %2 %3 %4 %5").arg(ip).arg(port).arg(dbName).arg(user).arg(passwd).toStdString());
	}
	return isopen;
}

QString DataBaseHelper::getError()
{
	return sqlDatabase.lastError().text();
}

bool DataBaseHelper::isopen()
{
	if(!sqlDatabase.isValid())
		return open(ip_,port_,dbName_,user_,passwd_); 

	if(!sqlDatabase.isOpen())
		return open(ip_,port_,dbName_,user_,passwd_); 
}

 
 

bool DataBaseHelper::readPersonData()
{
	bool result = false;
	QSqlQuery query;
	result = query.exec(SQL_SELECT_PERSON); 
	if(query.lastError().isValid()) 
	{
		SingletonLog->warn(query.lastError().text().toStdString());  
		SingletonLog->warn(query.lastQuery().toStdString());
		
		if (isopen()) 
			SingletonLog->debug("database reconnect success"); 
	}
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
	result = query.exec(SQL_SELECT_DEPARTMENT); 
	if(query.lastError().isValid()) 
	{
		SingletonLog->warn(query.lastError().text().toStdString());  
		SingletonLog->warn(query.lastQuery().toStdString());

		if (isopen()) 
			SingletonLog->debug("database reconnect success"); 
	}
	while(query.next())
	{ 
		stDepartmentData departmentData;
		departmentData.deptuuid = query.value(0).toString();
		departmentData.deptname = query.value(1).toString();
		departmentData.parentdeptuuid = query.value(2).toString();   
	   
		mapDepartmentData.insert(departmentData.deptuuid, departmentData); 
	}
	return result;
}

bool DataBaseHelper::readDoorFlag()
{ 
	bool result = false;
	QSqlQuery query;
	result = query.exec(SQL_READDOORFLAG); 
	if(query.lastError().isValid()) 
	{
		SingletonLog->warn(query.lastError().text().toStdString());  
		SingletonLog->warn(query.lastQuery().toStdString());
	}
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

QString DataBaseHelper::getParentUuidByChildUuid(QString Uuid)
{
	QString parentUuid = mapDepartmentData.value(Uuid).parentdeptuuid;
	return parentUuid;
}

QString DataBaseHelper::getDeptNameByUuid(QString Uuid)
{
	QString departmentName = mapDepartmentData.value(Uuid).deptname;
	return departmentName;
}

QString DataBaseHelper::getDeptUuidByPersonID(int personID)
{
	stPersonData personData =  mapPersonData.value(personID);   
	return personData.deptuuid;
}

QString DataBaseHelper::getDeptUuidByDeptName(QString DeptName)
{ 
	QSqlQuery query;
	query.exec(QString(SQL_SELECT_DEPARTMENTUUID).arg(DeptName)); 
	if(query.lastError().isValid()) 
	{
		SingletonLog->warn(query.lastError().text().toStdString());  
		SingletonLog->warn(query.lastQuery().toStdString());
	}

	QString deptUuid = "";
	while(query.next())
	{  
		deptUuid = query.value(0).toString();
	}
	return deptUuid;
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
 