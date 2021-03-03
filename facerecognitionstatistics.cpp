#include "facerecognitionstatistics.h"

facerecognitionstatistics::facerecognitionstatistics(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags),consumer(NULL),picUrlManager(NULL),currentTimer(NULL)
{
	ui.setupUi(this);  
	ui.pushButton->hide();
	this->showFullScreen(); 
	 
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(pushButtonClick()));
	 
	//��ʼ����־ϵͳ 
 	SingletonLog->initPropertyFile(QString(QApplication::applicationDirPath() + "/log4cplus.properties").toStdString(), "Log");
 	SingletonLog->debug("------------------Start-------------------------");
	  
	//����ͼƬ ������Json����
	picUrlManager = new QNetworkAccessManager(this);    
	connect(picUrlManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit())); 
	 
	currentTimer = new QTimer(this);
	connect(currentTimer, SIGNAL(timeout()), this, SLOT(dispCurrentTime()));
	currentTimer->start(1 * 1000); 
	  
	//��ȡmysql���� 
	if(SingletonDBHelper->open( SingletonConfig->getIpMySql(), SingletonConfig->getPortMySql(), SingletonConfig->getDbNameMySql(), 
								 SingletonConfig->getUserNameMySql(), SingletonConfig->getPassWdMySql())) 
	{
		SingletonLog->debug("MYSQL Connect Success!");  
		SingletonDBHelper->readDepartment();
		SingletonDBHelper->readPersonData();
		SingletonDBHelper->readDoorFlag(); 
	}
	else
	{
		SingletonLog->warn(QString("MYSQL Connect Failure:%1").arg(SingletonConfig->getIpMySql()).toStdString());
		SingletonLog->warn(SingletonDBHelper->getError().toLocal8Bit().data());
	} 

	//��ʼ�������ʽ
	initTableStyle(); 

	//��ʼ���������
	initTableWidget(); 
	  
	//��ʼ��activemq
	activemq::library::ActiveMQCPP::initializeLibrary(); 
	std::string brokerURI = SingletonConfig->getActiveMQ().toStdString();
	std::string destURI = "openapi.acs.topic"; 
	bool useTopics = true;
	bool clientAck = false;
	  
	// Create the consumer
	consumer = new SimpleAsyncConsumer( brokerURI, destURI, useTopics, clientAck ); 
	connect(consumer, SIGNAL(sendMessage(CommEventLog)), this, SLOT(receiverMessage(CommEventLog)));

	// Start it up and it will listen forever.
	consumer->runConsumer();    
}

facerecognitionstatistics::~facerecognitionstatistics()
{ 
	if (consumer != NULL)
	{
		delete consumer;
		consumer = NULL;
		activemq::library::ActiveMQCPP::shutdownLibrary();
	} 
	SingletonLog->debug("------------------End-------------------------");
}

void facerecognitionstatistics::initTableStyle()
{
	ui.label_picture1->setFixedWidth(250); 
	ui.label_picture1->setFixedHeight(ui.label_picture1->height());
	ui.label_picture1->setScaledContents(true); 
	ui.label_picture2->setFixedWidth(250);  
	ui.label_picture2->setFixedHeight(ui.label_picture2->height());
	ui.label_picture2->setScaledContents(true); 
	ui.label_department1->setFixedWidth(290);
	ui.label_department2->setFixedWidth(290);
	ui.label_personname1 ->setFixedWidth(290);
	ui.label_personname2->setFixedWidth(290);
	ui.label_personno1->setFixedWidth(290);
	ui.label_personno2->setFixedWidth(290);
	ui.label_personage1->setFixedWidth(290);
	ui.label_personage2->setFixedWidth(290); 

	ui.tableWidget->setShowGrid(true);    
	ui.tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	ui.tableWidget->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);  
	ui.tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
	ui.tableWidget->horizontalHeader()->setVisible(false); //�����б�ͷ 
	ui.tableWidget->verticalHeader()->setVisible(false); //�����б�ͷ  
	ui.tableWidget->setStyleSheet("QTableWidget::item {border: 2px solid #55ff00; color: rgb(85, 255, 0);} QTableWidget{gridline-color: rgb(85, 255, 0);}");
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void facerecognitionstatistics::initTableWidget()
{
	ui.tableWidget->setRowCount(0);

	int rowCount = ui.tableWidget->rowCount();
	ui.tableWidget->insertRow(rowCount); 

	QTableWidgetItem *item = new QTableWidgetItem(QString::fromLocal8Bit("��λ"));
	item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  
	ui.tableWidget->setItem(rowCount, 0, item);
	 
	QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromLocal8Bit("�����˴�"));
	item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 1, item1);

	QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromLocal8Bit("�뿪�˴�"));
	item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 2, item2);

	QTableWidgetItem *item3 = new QTableWidgetItem(QString::fromLocal8Bit("�ڳ�����"));
	item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 3, item3);
	    
	QStringList listDispDeptName = SingletonConfig->getDispDeptName(); 
	for (int i = 0; i < listDispDeptName.size(); ++i)
	{
		int rowCount = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowCount);

		QString deptName = listDispDeptName.at(i); 
		QString deptUuid = SingletonDBHelper->getDeptUuidByDeptName(deptName); 
		if (deptName.size() >= 6)    //�㽭��ҵ�������޹�˾ ����̫��
			deptName = deptName.left(4);

		if (!deptUuid.isEmpty())
			mapDispDeptInfo[deptUuid] = deptName;

		QTableWidgetItem *deptNameItem = new QTableWidgetItem(deptName);
		deptNameItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount, 0, deptNameItem); 

		QTableWidgetItem *enterItem = new QTableWidgetItem("0"); 
		enterItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount, 1, enterItem);

		QTableWidgetItem *leaveItem = new QTableWidgetItem("0"); 
		leaveItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount, 2, leaveItem);

		QTableWidgetItem *sumItem = new QTableWidgetItem("0"); 
		sumItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount, 3, sumItem);
	}
	

	/*
	QByteArray byteArray;
	QNetworkRequest request; //��������  
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");//����ͷ 
	request.setUrl(QUrl(SingletonConfig->getJavaUrl()));
	QNetworkReply *reply = picUrlManager->post(request, byteArray); //����    
	eventLoop.exec(); 
	// ��ȡ��Ӧ��Ϣ  
	QByteArray tmpData = reply->readAll(); //��ȡ��������
	reply->deleteLater();
	reply = NULL;
	 
	QJson::Parser parser;
	bool ok;
	QVariant result = parser.parse(QString(tmpData).toUtf8(), &ok);
	QVariantList mylist = result.toList();  
	foreach (QVariant plugin, mylist) {
		QVariantMap mymap = plugin.toMap();
		qDebug()<<"deptUuid:"<<mymap["deptUuid"].toString();
		qDebug()<<"parentdeptuuid:"<<mymap["parentdeptuuid"].toString();
		qDebug()<<"deptName:"<<mymap["deptName"].toString();
		qDebug()<<"bLeaf:"<<mymap["bLeaf"].toBool();
		qDebug()<<"entryCount:"<<mymap["entryCount"].toLongLong();
		qDebug()<<"exitCount:"<<mymap["exitCount"].toLongLong();
		qDebug()<<"stillCount:"<<mymap["stillCount"].toLongLong();
		qDebug()<<"passCount:"<<mymap["passCount"].toLongLong();
		qDebug()<<"_order:"<<mymap["_order"].toLongLong(); 

		QString order = mymap["_order"].toString();
		QStringList deptUuids =  mymap["deptUuid"].toString().split(",");
		QString deptName = QString::fromUtf8(mymap["deptName"].toByteArray());
		QString entryCount = mymap["entryCount"].toString();
		QString exitCount = mymap["exitCount"].toString();
		QString stillCount = mymap["stillCount"].toString();
		QString str = "order:" + order + " deptUuids:"+ deptUuids.join(",") +  " deptName:"+ deptName + " entryCount" + entryCount + " exitCount" +  exitCount +  " stillCount" +  stillCount; 
		SingletonLog->debug(str.toLocal8Bit().data());
		 
		for (int i = 0; i < deptUuids.size(); ++i) 
			mapDispDeptInfo.insert(deptUuids.at(i), deptName);  

	} 
	*/
}

QPixmap facerecognitionstatistics::requestPicUrlData(QString picUrl)
{ 
	QNetworkRequest request;
	request.setUrl(QUrl(picUrl));
	QNetworkReply *replay = picUrlManager->get(request);  //�������첽�ģ����¼�ѭ����������Ƭ�����ֲ���
	eventLoop.exec(); 
	// ��ȡ��Ӧ��Ϣ
	QByteArray bytes = replay->readAll(); 
	QPixmap pixmap;  
	pixmap.loadFromData(bytes);

	replay->deleteLater();
	replay = NULL;
	return pixmap;
}
 


void facerecognitionstatistics::modifyTableData(QString departmentName, int iEnter)
{
	QList<QTableWidgetItem *> listItem = ui.tableWidget->findItems(departmentName, Qt::MatchExactly);  
	if (listItem.size() != 0)
	{  
		QTableWidgetItem *item = listItem.at(0);  
		if (iEnter == 1)       //����
		{
			QTableWidgetItem *enterItem = ui.tableWidget->item(item->row(),1); 
			int enterNum = enterItem->text().toInt();
			enterItem->setText(QString::number(++enterNum));

			QTableWidgetItem *sumItem = ui.tableWidget->item(item->row(),3);
			int sumNum = sumItem->text().toInt();
			sumItem->setText(QString::number(++sumNum)); 
		}else if (iEnter == 0)       //����
		{
			QTableWidgetItem *leaveItem = ui.tableWidget->item(item->row(),2); 
			int leaveNum = leaveItem->text().toInt();
			leaveItem->setText(QString::number(++leaveNum));

			QTableWidgetItem *sumItem = ui.tableWidget->item(item->row(),3);
			int sumNum = sumItem->text().toInt();
			if(sumNum == 0) 
				return; 
			sumItem->setText(QString::number(--sumNum));
		}  

		if (departmentName != QString::fromLocal8Bit("�ϼ�")) 
			modifyTableData(QString::fromLocal8Bit("�ϼ�"), iEnter); 
	}  
}

 //ͬʱ���룬��Ҫ����2����¼
bool facerecognitionstatistics::judgeActionVaild(int personID, int enter)
{
	static int person1,person2;
	static int enter1,enter2;
	bool result = true;

	if (person1 == personID && enter1 == enter) 
		result = false; 

	if (person2 == personID && enter2 == enter) 
		result = false; 

	person2 = person1;
	enter2 = enter1;

	person1 = personID;
	enter1 = enter;

	return result;
}

void facerecognitionstatistics::receiverMessage(CommEventLog commEventLog)
{   
	string info = commEventLog.ext_info(); 
	AccessEventLog accessEventLog;  
	if(accessEventLog.ParseFromString(info))
	{     
		//����ʶ���豸���ܻᷢ������
		if(accessEventLog.event_card().empty())
			return;

		//����ʶ���豸����ͼƬ��ַ�п����ǿ�
		if(accessEventLog.pic_url().empty())
			return;
		 
		//1:���� 0:�뿪
		int iEnter = SingletonDBHelper->getDoorFlag(commEventLog.source_idx().c_str());

		QString str = QString("door_id:") + QString::number(accessEventLog.door_id()) +
			" cardID:" + accessEventLog.event_card().c_str() +
			" deviceID:" + commEventLog.source_idx().c_str() + 
			" personID:" + QString::number(accessEventLog.person_id()) + 
			" personName:" + QString::fromUtf8(accessEventLog.person_name().c_str()) + 
			" deptID:" + QString::number(accessEventLog.dept_id()) + 
			" pic_url:" +  QString::fromUtf8(accessEventLog.pic_url().c_str()) + 
			" dept_name:" + QString::fromUtf8(accessEventLog.dept_name().c_str()) + 
			" enter:" +  QString::number(iEnter);

		SingletonLog->debug(str.toLocal8Bit().data());  

		if(!SingletonConfig->getDeviceId().contains(commEventLog.source_idx().c_str()))
		{
			SingletonLog->debug(QString("DeviceId not contain").toStdString());  
			return;
		} 

		if (!judgeActionVaild(accessEventLog.person_id(), iEnter))  //��ǰ���롢�뿪�ظ�,����ʶ���豸���ܷ��Ͷ�������
		{
			SingletonLog->debug(QString(QString::fromLocal8Bit("��ǰ�����ظ�") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
			return;
		} 

		QString departmentUuid = SingletonDBHelper->getDeptUuidByPersonID(accessEventLog.person_id()); 
		int dispDeptNameSize = 0;
		while(1)
		{  
			if(mapDispDeptInfo.contains(departmentUuid))
			{
				QString dispDeptName = mapDispDeptInfo.value(departmentUuid); 
				dispDeptNameSize = dispDeptName.size(); 
				SingletonLog->debug(QString("personID:%1 dispDeptName:%2  Enter:%3").arg(accessEventLog.person_id()).arg(dispDeptName).arg(iEnter).toLocal8Bit().data());
				modifyTableData(dispDeptName, iEnter);  
				break;
			}
			else if (departmentUuid.isEmpty())
			{ 
				QString dispDeptName = QString::fromLocal8Bit("������Ա");
				dispDeptNameSize = dispDeptName.size();
				SingletonLog->debug(QString("personID:%1 dispDeptName:%2  Enter:%3").arg(accessEventLog.person_id()).arg(dispDeptName).arg(iEnter).toLocal8Bit().data());
				modifyTableData(QString::fromLocal8Bit("������Ա"), iEnter);  
				break;
			} 
			else
			{ 
				departmentUuid = SingletonDBHelper->getParentUuidByChildUuid(departmentUuid);
				continue; 
			} 
		}  

		ui.label_department2->setText(ui.label_department1->text());
		ui.label_personname2->setText(ui.label_personname1->text());
		ui.label_personno2->setText(ui.label_personno1->text());
		ui.label_personage2->setText(ui.label_personage1->text());
		if (ui.label_picture1->pixmap() == NULL)                     //��һ�λ�ȡ���ݣ�û����Ƭ 
			ui.label_picture2->setText(ui.label_picture1->text());   
		else 
			ui.label_picture2->setPixmap(*ui.label_picture1->pixmap()); 
			 
		QString dept_name = QString::fromUtf8(accessEventLog.dept_name().c_str());
		if (dept_name.size() >= 10)   
			dept_name.insert(5, "\n     ");  
		else if (dept_name.size() > 5 && dispDeptNameSize != 0)  //���dept_name������5���ַ����ڲ��źͰ����м���뻻�з�
			dept_name.insert(dispDeptNameSize, "\n     ");  
			 
		ui.label_department1->setText(QString::fromLocal8Bit("����:").append(dept_name));
		ui.label_personname1->setText(QString::fromLocal8Bit("����:").append(QString::fromUtf8(accessEventLog.person_name().c_str())));
		ui.label_personno1->setText(QString::fromLocal8Bit("���:").append(SingletonDBHelper->getAddressByPersonID(accessEventLog.person_id())));
		ui.label_personage1->setText(QString::fromLocal8Bit("����:").append(SingletonDBHelper->getAgeByPersonID(accessEventLog.person_id())));
		//ͨ��url��ȡ��Ƭ
		QPixmap pixmap = requestPicUrlData(QString::fromUtf8(accessEventLog.pic_url().c_str())); 
		ui.label_picture1->setPixmap(pixmap);  
	}    
}
  

void facerecognitionstatistics::dispCurrentTime()
{
	ui.label_currentTime->setText(QDateTime::currentDateTime().toString(QString::fromLocal8Bit("MM��dd�� hhʱmm��ss��")));

	if(QDateTime::currentDateTime().toString("hh:mm:ss") == "01:00:00")
	{  
		SingletonDBHelper->readPersonData();  
		SingletonDBHelper->readDepartment(); 
		//��ʼ���������
		initTableWidget(); 
	}    
}
 
 
void facerecognitionstatistics::pushButtonClick()
{
	modifyTableData(QString::fromLocal8Bit("������Ա"), 1);
} 




/*

if(QString::fromLocal8Bit("�����") == QString::fromUtf8(accessEventLog.dept_name().c_str()))
{    
ui.label_department2->setText(ui.label_department1->text());
ui.label_personname2->setText(ui.label_personname1->text());
ui.label_personno2->setText(ui.label_personno1->text());
ui.label_personage2->setText(ui.label_personage1->text()); 
if (ui.label_picture1->pixmap() != NULL) 
ui.label_picture2->setPixmap(*ui.label_picture1->pixmap()); 
else 
ui.label_picture2->setText(ui.label_picture1->text());   

ui.label_department1->setText(QString::fromLocal8Bit("����:").append(QString::fromLocal8Bit("�ϼ��쵼")));
ui.label_personname1->setText(QString::fromLocal8Bit("����:").append(QString::fromUtf8(accessEventLog.person_name().c_str())));
ui.label_personno1->setText("");
ui.label_personage1->setText("");  
ui.label_picture1->setText(QString::fromLocal8Bit("��ӭ�쵼\nݰ��ָ��"));   //set֮����pixmap���

SingletonLog->debug(QString(QString::fromLocal8Bit("�ϼ��쵼") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
modifyTableData(QString::fromLocal8Bit("�ϼ��쵼"), iEnter);  
}
else 


if(mapDispDeptInfo.values().contains(departmentName))
{
	SingletonLog->debug(QString(departmentName + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(departmentName, iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("���ݷ��籾��"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("��������") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("��������"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("�ջ�����"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("���ƻ���") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("���ƻ���"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("��ί��λ") || departmentName == QString::fromLocal8Bit("����糧"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("������ί��λ") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("������ί��λ"), iEnter);  
	break;
}else if (departmentName == QString::fromLocal8Bit("��ʱ����"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("�ϼ��쵼") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("�ϼ��쵼"), iEnter);  
	break;
} else if (departmentName.isEmpty())
{
	departmentNameSize = 4;
	SingletonLog->debug(QString(QString::fromLocal8Bit("�Ȳ��ǳ��ڵ�λ��Ҳ������ί��λ") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	break;
} 
*/ 