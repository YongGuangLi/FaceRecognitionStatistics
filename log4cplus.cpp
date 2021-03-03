#include "log4cplus.h"


Log4Cplus::Log4Cplus(void)
{
	//初始化日志系统
	log4cplus::initialize(); 
}


Log4Cplus::~Log4Cplus(void)
{
	log4cplus::Logger::shutdown();
}

Log4Cplus * Log4Cplus::log4Cplus_ = NULL;
Log4Cplus * Log4Cplus::GetInstance()
{
	if (log4Cplus_ == NULL)
	{
		log4Cplus_ = new Log4Cplus();
	}
	return log4Cplus_;
}

void Log4Cplus::initPropertyFile( string filename, string name)
{
 	ConfigureAndWatchThread configureThread(LOG4CPLUS_TEXT(filename), 5 * 1000);  //监控.properties更新
 	logger_ = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(name)); 
} 


void Log4Cplus::debug( string logEvent)
{
	LOG4CPLUS_DEBUG(logger_, LOG4CPLUS_TEXT(logEvent));
}

void Log4Cplus::info( string logEvent)
{
	LOG4CPLUS_INFO(logger_, LOG4CPLUS_TEXT(logEvent));
}

void Log4Cplus::warn( string logEvent)
{
	LOG4CPLUS_WARN(logger_, LOG4CPLUS_TEXT(logEvent));
}

void Log4Cplus::error( string logEvent)
{
	LOG4CPLUS_ERROR(logger_, LOG4CPLUS_TEXT(logEvent));
}