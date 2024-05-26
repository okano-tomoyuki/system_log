#ifndef _SYSTEM_DAEMONIZER_HPP_
#define _SYSTEM_DAEMONIZER_HPP_

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <mutex>

#ifdef __unix__
#include <unistd.h>
#include <linux/limits.h>
#include <syslog.h>
#else
#include <windows.h>
#endif

class SystemLog
{

public:
    enum ReportLevel
    {
        INFO_LEVEL  = 0,
        WARN_LEVEL  = 1,
        ERROR_LEVEL = 2
    };

    static void report(const enum ReportLevel& report_level)
    {
        if (instance_ == nullptr)
            create();

        auto thread_id = std::this_thread::get_id();
        std::stringstream ss;
        ss << "report thread id : " << thread_id << std::endl;
        if (instance_->buffer_.count(thread_id) != 0)
        {
#ifdef __unix__
	        constexpr int REPORT_LEVEL[3] = 
            {
                LOG_INFO, LOG_WARNING, LOG_ERR
            };
            syslog(REPORT_LEVEL[report_level], "%s%s", ss.str().c_str(), instance_->buffer_.at(thread_id).str().c_str());
#else
            constexpr int REPORT_LEVEL[3] = 
            {
                EVENTLOG_INFORMATION_TYPE, EVENTLOG_WARNING_TYPE, EVENTLOG_ERROR_TYPE
            };
            std::string line;
            std::vector<std::string> lines; 
            lines.push_back(ss.str());
            while (std::getline(instance_->buffer_.at(thread_id), line))
            {
                lines.push_back(line.c_str());
            }
            LPCSTR str[lines.size()];
            int line_num = 0;
            for (auto&& l : lines)
            {
                str[line_num] = l.c_str();
                line_num++;
            }
            ReportEventA(instance_->log_handle_, REPORT_LEVEL[report_level], 0, 0, NULL, line_num, 0, str, NULL);
#endif
            instance_->buffer_.at(thread_id).str(""); 
            instance_->buffer_.at(thread_id).clear(std::stringstream::goodbit);
        }
    }

    ~SystemLog()
    {
#ifdef __unix__
        closelog();
#else
        DeregisterEventSource(instance_->log_handle_);
#endif
        instance_ = nullptr;
    }

    static void add_line() 
    {
        if (instance_ == nullptr)
            create();
        
        auto thread_id = std::this_thread::get_id();
        if (instance_->buffer_.count(thread_id) == 0)
            instance_->buffer_.insert(std::make_pair(thread_id, std::stringstream()));

        instance_->buffer_.at(thread_id) << std::endl;
    }   

    template<typename T> 
    static void add_line(const T& value) 
    {
        if (instance_ == nullptr)
            create();
        auto thread_id = std::this_thread::get_id();
        if (instance_->buffer_.count(thread_id) == 0)
            instance_->buffer_.insert(std::make_pair(thread_id, std::stringstream())); 

        instance_->buffer_.at(thread_id)  << value << std::endl;
    }

    template<typename T, typename... Remain>
    static void add_line(const T& value, const Remain&... remain) 
    { 
        if (instance_ == nullptr)
            create();
        
        auto thread_id = std::this_thread::get_id();
        if (instance_->buffer_.count(thread_id) == 0)
            instance_->buffer_.insert(std::make_pair(thread_id, std::stringstream()));    

        instance_->buffer_.at(thread_id) << value; 
        add_line(remain...);     
    }

private:
    static SystemLog* instance_;
    static std::mutex mutex_;
    std::map<std::thread::id, std::stringstream> buffer_;
#ifndef __unix__
    HANDLE log_handle_;
#endif


    explicit SystemLog()
    {}

    static void create()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr)
        {
            instance_ = new SystemLog();
#ifdef __unix__
            char app_name[PATH_MAX];
            if (readlink("/proc/self/exe", app_name, sizeof(app_name)-1) == -1)
            {
                std::stringstream ss;
                ss << "Internal error in " << __func__ << " : " << errno;
                throw std::runtime_error(ss.str());
            }
            openlog(app_name, LOG_PID, LOG_USER);
#else
            char app_name[MAX_PATH];
            if (GetModuleFileNameA(NULL, app_name, MAX_PATH) == 0)
            {
                std::stringstream ss;
                ss << "Internal error in " << __func__ << " : " << GetLastError();
                throw std::runtime_error(ss.str());            
            }
            instance_->log_handle_ = RegisterEventSourceA(NULL, std::string(app_name).c_str());
#endif
        }      
    }
};

SystemLog* SystemLog::instance_ = nullptr;
std::mutex SystemLog::mutex_;

#endif
