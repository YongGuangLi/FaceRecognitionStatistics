#ifndef FACERECOGNITIONSTATISTICS_H
#define FACERECOGNITIONSTATISTICS_H

#include "acs_event.pb.h"
#include "comm.pb.h"
#include "event_dis.pb.h"
using namespace com::hikvision:: cms::api::eps::beds;
 
#include "log4cplus.h"
#include "databasehelper.h"
#include "configini.h" 
#include "parser.h"

#include <QtGui/QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QList> 
#include "ui_facerecognitionstatistics.h"
#include "simpleasyncconsumer.h" 
 
#include <iostream>
using namespace std;


class facerecognitionstatistics : public QWidget
{
	Q_OBJECT

public:
	facerecognitionstatistics(QWidget *parent = 0, Qt::WFlags flags = 0);
	~facerecognitionstatistics();

	void initTableStyle();                  //初始化表格样式
	void initTableWidget();                 //初始化表格

	QPixmap requestPicUrlData(QString);  //请求照片数据

	void modifyTableData(QString , int);

	bool judgeActionVaild(int , int);   //判断进、出动作是否有效，防止重复
private slots:
	void receiverMessage(CommEventLog);     //接收activemq发送的消息
	 
	void dispCurrentTime();             //显示当前时间

	void pushButtonClick();
private:
	Ui::facerecognitionstatisticsClass ui;
	SimpleAsyncConsumer *consumer; 
	log4cplus::Logger logger;
	QTimer* currentTimer;
	QNetworkAccessManager *picUrlManager; 
	QEventLoop eventLoop;
	QMap<QString, QString> mapDepartmentSeq;  
};

#endif // FACERECOGNITIONSTATISTICS_H
