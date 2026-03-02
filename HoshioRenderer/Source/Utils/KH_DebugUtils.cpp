#include "KH_DebugUtils.h"

#include "Editor/KH_Panel.h"


static std::mutex gLogMutex;

void KH_Log(KH_LOG_FLAG Flag, const std::string& Message, const char* File, int Line) {
    std::lock_guard<std::mutex> lock(gLogMutex);

    std::string FileName = std::filesystem::path(File).filename().string();
    std::string LogInfo = std::format("[{}:{}] {}", FileName, Line, Message);

    auto now = std::chrono::system_clock::now();
    std::string Timestamp = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    std::string Label;
    std::string Color;

    switch (Flag) {
	    case KH_LOG_FLAG::Temp:    Label = "[TEMP]"; Color = "\033[37m"; break; // 白色
	    case KH_LOG_FLAG::Debug:   Label = "[DEBUG]"; Color = "\033[36m"; break; // 青色
	    case KH_LOG_FLAG::Warning: Label = "[WARNING]"; Color = "\033[33m"; break; // 黄色
	    case KH_LOG_FLAG::Error:   Label = "[ERROR]"; Color = "\033[31m"; break; // 红色
    }

    std::string FmtMessage = std::format("{}[{}] {} {} \033[0m\n",
        Color, Timestamp, Label, LogInfo);

    //std::cout << Color                     
    //    << "[" << Timestamp << "] "   
    //    << Label         
    //    << LogInfo                    
    //    << "\033[0m"                 
    //    << std::endl;

    std::cout << FmtMessage;

    FmtMessage = std::format("[{}] {} {}",
         Timestamp, Label, LogInfo);
	
    KH_Console::LogMessages.emplace_back(Flag, FmtMessage);
}