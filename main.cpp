#include <map>
#include <string>
#include <thread>

#include "system_log.hpp"

int main()
{
    SystemLog::add_line("This is main thread.");
    SystemLog::add_line("thread id is ", std::this_thread::get_id());
    SystemLog::report(SystemLog::INFO_LEVEL);

    std::thread th1([](){
        SystemLog::add_line("This is thread 1.");
        SystemLog::add_line("thread id is ", std::this_thread::get_id());
        SystemLog::report(SystemLog::WARN_LEVEL);
    });

    std::thread th2([](){
        SystemLog::add_line("This is thread 2.");
        SystemLog::add_line("thread id is ", std::this_thread::get_id());
        SystemLog::report(SystemLog::ERROR_LEVEL);
    });

    th1.join();
    th2.join();
}