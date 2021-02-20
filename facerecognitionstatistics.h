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

	void initTableStyle();                  //��ʼ�������ʽ
	void initTableWidget();                 //��ʼ�����

	QPixmap requestPicUrlData(QString);  //������Ƭ����

	void modifyTableData(QString , int);

	bool judgeActionVaild(int , int);   //�жϽ����������Ƿ���Ч����ֹ�ظ�
private slots:
	void receiverMessage(CommEventLog);     //����activemq���͵���Ϣ
	 
	void dispCurrentTime();             //��ʾ��ǰʱ��

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
