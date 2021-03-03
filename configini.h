#ifndef CONFIGINI_H
#define CONFIGINI_H

#include <QtCore/QCoreApplication>
#include <QObject>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QStringList>
#include "log4cplus.h"

#define SingletonConfig ConfigIni::GetInstance()

class ConfigIni : public QObject
{
    Q_OBJECT
public:
    static ConfigIni *GetInstance();

    QString getIpMySql() const;
    int getPortMySql() const;
    QString getDbNameMySql() const;
    QString getUserNameMySql() const;
    QString getPassWdMySql() const; 

	QString getActiveMQ() const;

	QString getJavaUrl() const;

	QStringList getDeviceId() const;

	QStringList getDispDeptName() const;
private:
    explicit ConfigIni(QObject *parent = 0);
    static ConfigIni* configIni;

    QString ipMySql_;
    int portMySql_;
    QString dbName_;
    QString userNameMqSql_;
    QString passWdMySql_;
	 
	QString activeMQ_;

	QString javaUrl_;

	QStringList listDeviceId_;

	QStringList listDispDeptName_;
signals:
    
public slots:
    
};

#endif // CONFIGINI_H
