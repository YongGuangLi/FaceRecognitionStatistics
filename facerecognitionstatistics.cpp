#include "facerecognitionstatistics.h"

facerecognitionstatistics::facerecognitionstatistics(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags),consumer(NULL),picUrlManager(NULL),currentTimer(NULL)
{
	ui.setupUi(this);  
	ui.pushButton->hide();
	this->showFullScreen(); 
	 
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(pushButtonClick()));
	 
	//初始化日志系统 
 	SingletonLog->initPropertyFile(QString(QApplication::applicationDirPath() + "/log4cplus.properties").toStdString(), "Log");
 	SingletonLog->debug("------------------Start-------------------------");
	  
	//请求图片 请求表格Json数据
	picUrlManager = new QNetworkAccessManager(this);    
	connect(picUrlManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit())); 
	 
	currentTimer = new QTimer(this);
	connect(currentTimer, SIGNAL(timeout()), this, SLOT(dispCurrentTime()));
	currentTimer->start(1 * 1000); 
	  
	//读取mysql数据 
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

	//初始化表格样式
	initTableStyle(); 

	//初始化表格数据
	initTableWidget(); 
	  
	//初始化activemq
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
	ui.tableWidget->horizontalHeader()->setVisible(false); //隐藏行表头 
	ui.tableWidget->verticalHeader()->setVisible(false); //隐藏行表头  
	ui.tableWidget->setStyleSheet("QTableWidget::item {border: 2px solid #55ff00; color: rgb(85, 255, 0);} QTableWidget{gridline-color: rgb(85, 255, 0);}");
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void facerecognitionstatistics::initTableWidget()
{
	ui.tableWidget->setRowCount(0);

	int rowCount = ui.tableWidget->rowCount();
	ui.tableWidget->insertRow(rowCount); 

	QTableWidgetItem *item = new QTableWidgetItem(QString::fromLocal8Bit("单位"));
	item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  
	ui.tableWidget->setItem(rowCount, 0, item);
	 
	QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromLocal8Bit("进入人次"));
	item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 1, item1);

	QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromLocal8Bit("离开人次"));
	item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 2, item2);

	QTableWidgetItem *item3 = new QTableWidgetItem(QString::fromLocal8Bit("在厂人数"));
	item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); 
	ui.tableWidget->setItem(rowCount, 3, item3);
	    
	QStringList listDispDeptName = SingletonConfig->getDispDeptName(); 
	for (int i = 0; i < listDispDeptName.size(); ++i)
	{
		int rowCount = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowCount);

		QString deptName = listDispDeptName.at(i); 
		QString deptUuid = SingletonDBHelper->getDeptUuidByDeptName(deptName); 
		if (deptName.size() >= 6)    //浙江华业电力有限公司 名字太长
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
	QNetworkRequest request; //网络请求  
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");//设置头 
	request.setUrl(QUrl(SingletonConfig->getJavaUrl()));
	QNetworkReply *reply = picUrlManager->post(request, byteArray); //推送    
	eventLoop.exec(); 
	// 获取响应信息  
	QByteArray tmpData = reply->readAll(); //读取所有数据
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
	QNetworkReply *replay = picUrlManager->get(request);  //请求是异步的，用事件循环，避免照片与名字不符
	eventLoop.exec(); 
	// 获取响应信息
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
		if (iEnter == 1)       //进入
		{
			QTableWidgetItem *enterItem = ui.tableWidget->item(item->row(),1); 
			int enterNum = enterItem->text().toInt();
			enterItem->setText(QString::number(++enterNum));

			QTableWidgetItem *sumItem = ui.tableWidget->item(item->row(),3);
			int sumNum = sumItem->text().toInt();
			sumItem->setText(QString::number(++sumNum)); 
		}else if (iEnter == 0)       //出来
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

		if (departmentName != QString::fromLocal8Bit("合计")) 
			modifyTableData(QString::fromLocal8Bit("合计"), iEnter); 
	}  
}

 //同时进入，需要保存2条记录
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
		//人脸识别设备可能会发空数据
		if(accessEventLog.event_card().empty())
			return;

		//人脸识别设备数据图片地址有可能是空
		if(accessEventLog.pic_url().empty())
			return;
		 
		//1:进入 0:离开
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

		if (!judgeActionVaild(accessEventLog.person_id(), iEnter))  //当前进入、离开重复,人脸识别设备可能发送多条数据
		{
			SingletonLog->debug(QString(QString::fromLocal8Bit("当前动作重复") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
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
				QString dispDeptName = QString::fromLocal8Bit("其他人员");
				dispDeptNameSize = dispDeptName.size();
				SingletonLog->debug(QString("personID:%1 dispDeptName:%2  Enter:%3").arg(accessEventLog.person_id()).arg(dispDeptName).arg(iEnter).toLocal8Bit().data());
				modifyTableData(QString::fromLocal8Bit("其他人员"), iEnter);  
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
		if (ui.label_picture1->pixmap() == NULL)                     //第一次获取数据，没有照片 
			ui.label_picture2->setText(ui.label_picture1->text());   
		else 
			ui.label_picture2->setPixmap(*ui.label_picture1->pixmap()); 
			 
		QString dept_name = QString::fromUtf8(accessEventLog.dept_name().c_str());
		if (dept_name.size() >= 10)   
			dept_name.insert(5, "\n     ");  
		else if (dept_name.size() > 5 && dispDeptNameSize != 0)  //如果dept_name超出了5个字符，在部门和班组中间插入换行符
			dept_name.insert(dispDeptNameSize, "\n     ");  
			 
		ui.label_department1->setText(QString::fromLocal8Bit("部门:").append(dept_name));
		ui.label_personname1->setText(QString::fromLocal8Bit("姓名:").append(QString::fromUtf8(accessEventLog.person_name().c_str())));
		ui.label_personno1->setText(QString::fromLocal8Bit("编号:").append(SingletonDBHelper->getAddressByPersonID(accessEventLog.person_id())));
		ui.label_personage1->setText(QString::fromLocal8Bit("年龄:").append(SingletonDBHelper->getAgeByPersonID(accessEventLog.person_id())));
		//通过url获取照片
		QPixmap pixmap = requestPicUrlData(QString::fromUtf8(accessEventLog.pic_url().c_str())); 
		ui.label_picture1->setPixmap(pixmap);  
	}    
}
  

void facerecognitionstatistics::dispCurrentTime()
{
	ui.label_currentTime->setText(QDateTime::currentDateTime().toString(QString::fromLocal8Bit("MM月dd日 hh时mm分ss秒")));

	if(QDateTime::currentDateTime().toString("hh:mm:ss") == "01:00:00")
	{  
		SingletonDBHelper->readPersonData();  
		SingletonDBHelper->readDepartment(); 
		//初始化表格数据
		initTableWidget(); 
	}    
}
 
 
void facerecognitionstatistics::pushButtonClick()
{
	modifyTableData(QString::fromLocal8Bit("其他人员"), 1);
} 




/*

if(QString::fromLocal8Bit("贵宾卡") == QString::fromUtf8(accessEventLog.dept_name().c_str()))
{    
ui.label_department2->setText(ui.label_department1->text());
ui.label_personname2->setText(ui.label_personname1->text());
ui.label_personno2->setText(ui.label_personno1->text());
ui.label_personage2->setText(ui.label_personage1->text()); 
if (ui.label_picture1->pixmap() != NULL) 
ui.label_picture2->setPixmap(*ui.label_picture1->pixmap()); 
else 
ui.label_picture2->setText(ui.label_picture1->text());   

ui.label_department1->setText(QString::fromLocal8Bit("部门:").append(QString::fromLocal8Bit("上级领导")));
ui.label_personname1->setText(QString::fromLocal8Bit("姓名:").append(QString::fromUtf8(accessEventLog.person_name().c_str())));
ui.label_personno1->setText("");
ui.label_personage1->setText("");  
ui.label_picture1->setText(QString::fromLocal8Bit("欢迎领导\n莅临指导"));   //set之后会把pixmap清空

SingletonLog->debug(QString(QString::fromLocal8Bit("上级领导") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
modifyTableData(QString::fromLocal8Bit("上级领导"), iEnter);  
}
else 


if(mapDispDeptInfo.values().contains(departmentName))
{
	SingletonLog->debug(QString(departmentName + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(departmentName, iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("抚州发电本部"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("其他部门") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("其他部门"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("苏华建设"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("大唐环境") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("大唐环境"), iEnter);  
	break;
}else if(departmentName == QString::fromLocal8Bit("外委单位") || departmentName == QString::fromLocal8Bit("新余电厂"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("其他外委单位") + " personID:" + QString::number(accessEventLog.person_id()) + " Enter:" + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("其他外委单位"), iEnter);  
	break;
}else if (departmentName == QString::fromLocal8Bit("临时来访"))
{ 
	SingletonLog->debug(QString(QString::fromLocal8Bit("上级领导") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	modifyTableData(QString::fromLocal8Bit("上级领导"), iEnter);  
	break;
} else if (departmentName.isEmpty())
{
	departmentNameSize = 4;
	SingletonLog->debug(QString(QString::fromLocal8Bit("既不是厂内单位，也不是外委单位") + " personID:" + QString::number(accessEventLog.person_id()) + " " + QString::number(iEnter)).toLocal8Bit().data());
	break;
} 
*/ 