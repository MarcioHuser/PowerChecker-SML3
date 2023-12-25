#pragma once

#include "Logging/LogMacros.h"

#include <sstream>

class CommaLog
{
public:
	template <typename T>
	inline CommaLog&
	operator,(const T& value)
	{
		wos << value;

		return *this;
	}

	inline CommaLog&
	operator,(const FString& value)
	{
		wos << *value;

		return *this;
	}

	inline CommaLog&
	operator,(const FText& value)
	{
		wos << *value.ToString();

		return *this;
	}

	std::wostringstream wos;
};

DECLARE_LOG_CATEGORY_EXTERN(LogPowerChecker, Log, All)

#define PC_LOG_Verbosity(verbosity, first, ...) \
	{ \
		CommaLog l; \
		l, first, ##__VA_ARGS__; \
		UE_LOG(LogPowerChecker, verbosity, TEXT("%s"), l.wos.str().c_str()) \
	}

#define PC_LOG_Log(first, ...) PC_LOG_Verbosity(Log, first, ##__VA_ARGS__)
#define PC_LOG_Display(first, ...) PC_LOG_Verbosity(Display, first, ##__VA_ARGS__)
#define PC_LOG_Warning(first, ...) PC_LOG_Verbosity(Warning, first, ##__VA_ARGS__)
#define PC_LOG_Error(first, ...) PC_LOG_Verbosity(Error, first, ##__VA_ARGS__)

#define IS_PC_LOG_LEVEL(level) (APowerCheckerLogic::configuration.logLevel > 0 && APowerCheckerLogic::configuration.logLevel >= static_cast<uint8>(level))

#define PC_LOG_Log_Condition(first, ...) if(IS_PC_LOG_LEVEL(ELogVerbosity::Log)) PC_LOG_Log(first, ##__VA_ARGS__)
#define PC_LOG_Display_Condition(first, ...) if(IS_PC_LOG_LEVEL(ELogVerbosity::Display)) PC_LOG_Display(first, ##__VA_ARGS__)
#define PC_LOG_Warning_Condition(first, ...) if(IS_PC_LOG_LEVEL(ELogVerbosity::Warning)) PC_LOG_Warning(first, ##__VA_ARGS__)
#define PC_LOG_Error_Condition(first, ...) if(IS_PC_LOG_LEVEL(ELogVerbosity::Error)) PC_LOG_Error(first, ##__VA_ARGS__)
