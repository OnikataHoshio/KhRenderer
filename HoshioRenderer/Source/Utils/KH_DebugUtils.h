#pragma once

#include "KH_Common.h"

enum class KH_LOG_FLAG
{
	Temp = 0,
	Debug = 1,
	Warning = 2,
	Error = 3
};

struct KH_LOG_MESSAGE
{
	KH_LOG_FLAG Flag;
	std::string Message;
};

void KH_Log(KH_LOG_FLAG Flag, const std::string& Message, const char* File, int Line);

#define LOG_T(msg) KH_Log(KH_LOG_FLAG::Temp, msg, __FILE__, __LINE__)
#define LOG_D(msg) KH_Log(KH_LOG_FLAG::Debug, msg, __FILE__, __LINE__)
#define LOG_W(msg) KH_Log(KH_LOG_FLAG::Warning, msg, __FILE__, __LINE__)
#define LOG_E(msg) KH_Log(KH_LOG_FLAG::Error, msg, __FILE__, __LINE__)