#include "facerecognitionstatistics.h"

facerecognitionstatistics::facerecognitionstatistics(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
{
	ui.setupUi(this);  
	this->showFullScreen();
	ui.pushButton->hide(); 
	 
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(pushButtonClick()));
	 
	//��ʼ����־ϵͳ 
 	SingletonLog->initPropertyFile(QString(QApplication::applicationDirPath() + "/log4cplus.properties").toStdString(), "Log");
 	SingletonLog->logDebug("------------------Start-------------------------");
	 
	//��ʼ�������ʽ
	initTableStyle();   

	//����ͼƬ ������Json����
	picUrlManager = new QNetworkAccessManager();    
	connect(picUrlManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit())); 

	//����������
	initTableWidget();        
	 
	currentTimer = new QTimer();
	connect(currentTimer, SIGNAL(timeout()), this, SLOT(dispCurrentTime()));
	currentTimer->start(1 * 1000); 

	//��ȡmysql����
	if(SingletonDBHelper->open( SingletonConfig->getIpMySql(), SingletonConfig->getPortMySql(), SingletonConfig->getDbNameMySql(), 
								 SingletonConfig->getUserNameMySql(), SingletonConfig->getPassWdMySql())) {
		SingletonLog->logDebug("MYSQL Connect Success!");  
		SingletonDBHelper->readDepartment();
		SingletonDBHelper->readPersonData();
		SingletonDBHelper->readDoorFlag();
	}else
		SingletonLog->logDebug(SingletonDBHelper->getError().toStdString());  

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
	SingletonLog->logDebug("------------------End-------------------------");
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
	mapDepartmentSeq.clear();
	  
	int rowCount = ui.tableWidget->rowCount();
	ui.tableWidget->insertRow(rowCount); 
	QTableWidgetItem *item = new QTableWidgetItem(QString::fromLocal8Bit("��λ"));
	item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  
	ui.tableWidget->setItem(rowCount,0,item);

	QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromLocal8Bit("�����˴�"));
	item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount,1,item1);

	QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromLocal8Bit("�뿪�˴�"));
	item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount,2,item2);

	QTableWidgetItem *item3 = new QTableWidgetItem(QString::fromLocal8Bit("�ڳ�����"));
	item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount,3,item3);
	    
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
		QString deptUuids =  mymap["deptUuid"].toString();
		QString deptName = QString::fromUtf8(mymap["deptName"].toByteArray());
		QString entryCount = mymap["entryCount"].toString();
		QString exitCount = mymap["exitCount"].toString();
		QString stillCount = mymap["stillCount"].toString();
		QString str = "order:" + order + " deptUuids:"+ deptUuids +  " deptName:"+ deptName + " entryCount" + entryCount + " exitCount" +  exitCount +  " stillCount" +  stillCount; 
		SingletonLog->logDebug(str.toLocal8Bit().data());
		 
		for (int i = 0; i < deptUuids.split(",").size(); ++i) 
			mapDepartmentSeq.insert(deptUuids.split(",").at(i), deptName);  

		int rowCount = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowCount);

		QTableWidgetItem *item = new QTableWidgetItem(deptName);
		item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount,0,item);

		QTableWidgetItem *enterItem = new QTableWidgetItem(entryCount); 
		enterItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount,1,enterItem);

		QTableWidgetItem *leaveItem = new QTableWidgetItem(exitCount); 
		leaveItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount,2,leaveItem);

		QTableWidgetItem *sumItem = new QTableWidgetItem(stillCount); 
		sumItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
		ui.tableWidget->setItem(rowCount,3,sumItem);
	} 
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
 

void facerecognitionstatistics::modifyTableData(QString department, int iEnter)
{
	QList<QTableWidgetItem *> listItem = ui.tableWidget->findItems(department, Qt::MatchExactly);  
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

		if (department != QString::fromLocal8Bit("�ϼ�")) 
			modifyTableData(QString::fromLocal8Bit("�ϼ�"), iEnter); 
	}  
}

 
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
	int iEnter = SingletonDBHelper->getDoorFlag(commEventLog.source_idx().c_str());
	string info = commEventLog.ext_info(); 
	AccessEventLog accessEventLog; 
	SingletonLog->logDebug(QString("door_id:%1, device_name:%2").arg(accessEventLog.door_id()).arg(accessEventLog.device_name().c_str()).toLocal8Bit().data());
	
	if(!SingletonConfig->getDoorId().contains(QString::number(accessEventLog.door_id())))
		return;

	if(accessEventLog.ParseFromString(info))
	{     
		QString str = QString("cardID:")+accessEventLog.event_card().c_str() +
			" deviceID:" + commEventLog.source_idx().c_str() + 
			" personID:" + QString::number(accessEventLog.person_id()) + 
			" personName:" + QString::fromUtf8(accessEventLog.person_name().c_str()) + 
			" deptID:" + QString::number(accessEventLog.dept_id()) + 
			" picurl:" +  QString::fromUtf8(accessEventLog.pic_url().c_str()) + 
			" dept_name:" + QString::fromUtf8(accessEventLog.dept_name().c_str()) + 
			" enter:" + QString::number(accessEventLog.in_out()) + 
			" enter:" + QString::number(iEnter);

		SingletonLog->logDebug(str.toLocal8Bit().data()); 

		if (!judgeActionVaild(accessEventLog.person_id(), iEnter))  //��ǰ���롢�뿪�ظ�,����ʶ���豸���ܷ��Ͷ�������
		{
			SingletonLog->logDebug(QString(QString::fromLocal8Bit("��ǰ�����ظ�") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
			return;
		} 

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

			SingletonLog->logDebug(QString(QString::fromLocal8Bit("�ϼ��쵼") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
			modifyTableData(QString::fromLocal8Bit("�ϼ��쵼"), iEnter);  
		}
		else if (accessEventLog.pic_url().length() != 0)
		{    
			QString departmentUuid = SingletonDBHelper->getDepartmentUuidByPersonID(accessEventLog.person_id()); 
			if (departmentUuid.isEmpty()) 
				SingletonLog->logDebug(QString("personID:" + QString::number(accessEventLog.person_id()) + QString::fromLocal8Bit("������")).toLocal8Bit().data());
		  
			int dispDeptNameSize = 0;
			while(1)
			{ 
				if(mapDepartmentSeq.contains(departmentUuid))
				{
					QString dispDeptName = mapDepartmentSeq.value(departmentUuid); 
					dispDeptNameSize = dispDeptName.size(); 
					SingletonLog->logDebug(QString(dispDeptName + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
					modifyTableData(dispDeptName, iEnter);  
					break;
				}
				else if (departmentUuid.isEmpty())
				{ 
					SingletonLog->logDebug(QString(QString::fromLocal8Bit("�Ȳ��ǳ��ڵ�λ��Ҳ������ί��λ") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
					break;
				}
				else
				{ 
					departmentUuid = SingletonDBHelper->getParentUuidByUuid(departmentUuid);
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
			
			if (dept_name.size() > 5 && dispDeptNameSize > 5) 
				dept_name.insert(4, "\n     ");    
			else if (dept_name.contains(QString::fromLocal8Bit("ʵϰ��"))) 
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
}
  

void facerecognitionstatistics::dispCurrentTime()
{
	ui.label_currentTime->setText(QDateTime::currentDateTime().toString(QString::fromLocal8Bit("MM��dd�� hhʱmm��ss��")));

	if(QDateTime::currentDateTime().toString("hh:mm:ss") == "00:30:00") 
		 initTableWidget(); 
	 
	if((QDateTime::currentDateTime().toString("mm").toInt() % 10 == 0) && (QDateTime::currentDateTime().toString("ss").toInt() == 0))
	{  
		SingletonDBHelper->readPersonData(); 
		SingletonLog->logDebug(QString("PersonDataSize:%1").arg(SingletonDBHelper->getPersonData().size()).toStdString());
	}

	if(QDateTime::currentDateTime().toString("hh:mm:ss") == "01:00:00")
	{  
		SingletonDBHelper->readDepartment();
		SingletonLog->logDebug(QString("DepartmentNameSize:%1").arg(SingletonDBHelper->getDepartmentName().size()).toStdString());
	}    
}
 
 
void facerecognitionstatistics::pushButtonClick()
{
	int iEnter = 1;
	if (!judgeActionVaild(123, iEnter))  //��ǰ���롢�뿪��Ч
	{
		SingletonLog->logDebug(QString(QString::fromLocal8Bit("��ǰ�����ظ�") + " personID:" + QString::number(123) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
		return;
	} 
	 
	ui.label_department2->setText(ui.label_department1->text()); 
	ui.label_personname2->setText(ui.label_personname1->text()); 
	ui.label_personno2->setText(ui.label_personno1->text()); 
	ui.label_personage2->setText(ui.label_personage1->text()); 
	if (ui.label_picture1->pixmap() != NULL) 
		ui.label_picture2->setPixmap(ui.label_picture1->pixmap()->scaled(ui.label_picture2->size())); 
	else 
		ui.label_picture2->setText(ui.label_picture1->text());   

	ui.label_department1->setText(QString::fromLocal8Bit("����:").append(QString::fromLocal8Bit("�ϼ��쵼")));
	ui.label_personname1->setText(QString::fromLocal8Bit("����:").append(QString::fromUtf8("1123")));
	ui.label_personno1->setText("");
	ui.label_personage1->setText("");  
	ui.label_picture1->setText(QString::fromLocal8Bit("��ӭ�쵼\nݰ��ָ��"));  

	SingletonLog->logDebug(QString(QString::fromLocal8Bit("�ϼ��쵼") + " personID:" + QString::number(1123) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data()); 
} 




/*
if(mapDepartmentSeq.values().contains(departmentName))
{
	SingletonLog->logDebug(QString(departmentName + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(departmentName, iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("���ݷ��籾��"))
{ 
	SingletonLog->logDebug(QString(QString::fromLocal8Bit("��������") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("��������"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("�ջ�����"))
{ 
	SingletonLog->logDebug(QString(QString::fromLocal8Bit("���ƻ���") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("���ƻ���"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("��ί��λ") || departmentName == QString::fromLocal8Bit("����糧"))
{ 
	SingletonLog->logDebug(QString(QString::fromLocal8Bit("������ί��λ") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("������ί��λ"), iEnter);  
	break;
}else if (departmentName == QString::fromLocal8Bit("��ʱ����"))
{ 
	SingletonLog->logDebug(QString(QString::fromLocal8Bit("�ϼ��쵼") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("�ϼ��쵼"), iEnter);  
	break;
} else if (departmentName.isEmpty())
{
	departmentNameSize = 4;
	SingletonLog->logDebug(QString(QString::fromLocal8Bit("�Ȳ��ǳ��ڵ�λ��Ҳ������ί��λ") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	break;
} 
*/ 