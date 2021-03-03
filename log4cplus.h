#pragma once

#include <log4cplus/configurator.h>
#include <log4cplus/logger.h>  
#include <log4cplus/loggingmacros.h> 
#include <log4cplus/helpers/sleep.h>  
#include <log4cplus/helpers/loglog.h>
using namespace log4cplus; 
using namespace log4cplus::helpers; 
  
#include <string>
using namespace std;

#define SingletonLog Log4Cplus::GetInstance()

class Log4Cplus
{
public:
	static Log4Cplus *GetInstance();
	void initPropertyFile(string,string);

	void debug(string); 
	void info(string); 
	void warn(string);
	void error(string);
private:
	Log4Cplus(void);
	~Log4Cplus(void);
	static Log4Cplus * log4Cplus_;
	log4cplus::Logger logger_; 
};

