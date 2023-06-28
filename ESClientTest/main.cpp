#include <CLI11.h>
#include "Types.h"
#include "EndpointSecurity/EndpointSecurity.h"
#include <dispatch/dispatch.h>

#include <bsm/libbsm.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/mach_traps.h>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <array>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <signal.h>
#include <chrono>
#include <locale>
#include <mutex>
#include <unordered_set>

#include <bsm/libbsm.h>
#include <libproc.h>

//dispatch_queue_t testQueue = dispatch_queue_create("test", DISPATCH_QUEUE_SERIAL);

std::unordered_map<pid_t, timeval> forkStartTimes {};

bool operator== (const timeval& lhs, const timeval& rhs) {
    return lhs.tv_sec == rhs.tv_sec && lhs.tv_usec == rhs.tv_usec;
}

void handle_event(es_client_t* client, const es_message_t* msg)
{
    switch (msg->event_type) {
            
        case ES_EVENT_TYPE_NOTIFY_FORK:
        {
            pid_t pid = audit_token_to_pid(msg->event.fork.child->audit_token);
            timeval esStartTime = msg->event.fork.child->start_time;
            
            forkStartTimes.insert({pid, esStartTime});
            break;
        }
            
        case ES_EVENT_TYPE_NOTIFY_EXEC:
        {
            pid_t pid = audit_token_to_pid(msg->event.exec.target->audit_token);
            timeval execStartTime = msg->event.exec.target->start_time;
            
            try {
                timeval forkTime = forkStartTimes.at(pid);
                assert (forkTime == execStartTime);
                std::cout << "pid " << pid << " exec starttime matches fork\n";
            } catch (...) {
                std::cout << "missing fork event for pid " << pid << std::endl;
            }
            
            assert(msg->event.exec.target->start_time == msg->process->start_time);
//            proc_taskallinfo info {};
//
//            auto size = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &info, PROC_PIDTASKALLINFO_SIZE);
//
//            if (size == 0) {
//                std::cerr << "proc pidinfo failed\n";
//                return;
//            }
//
//            proc_bsdinfo bsdInfo = info.pbsd;
//
//            uint64_t bsdSecs = bsdInfo.pbi_start_tvsec;
//            uint64_t bsdUSecs = bsdInfo.pbi_start_tvusec;
//
//            assert((int64_t)bsdSecs == execStartTime.tv_sec);
//            assert((int64_t)bsdUSecs == execStartTime.tv_usec);
//
//            std::cout << "Start point matches exactly for pid " << pid << std::endl;
            
//            std::chrono::seconds seconds(bsdSecs);
//            std::chrono::microseconds useconds(bsdUSecs);
            
            break;
        }
            
        case ES_EVENT_TYPE_NOTIFY_EXIT:
        {
            break;
        }
            
        default:
        {
            
            break;
        }
    }
}


//void sigHandler()
//{
//   std::scoped_lock lock{pidMutex};
//   std::cout << "Size of pidversions: " << pidversions.size() << "\n";
//}

int main(int argc, char* argv[])
{
    
    es_client_t* client;
    es_new_client_result_t result = es_new_client(
                                                  &client, ^(es_client_t* c, const es_message_t* msg) {
                                                      handle_event(c, msg);
                                                  }
                                                  );
    
    if (result != ES_NEW_CLIENT_RESULT_SUCCESS)
    {
        fprintf(stderr, "Failed to create new ES client \n");
        return 1;
    }
    
    es_event_type_t events[] = {ES_EVENT_TYPE_NOTIFY_EXEC, ES_EVENT_TYPE_NOTIFY_FORK};
    uint32_t count = sizeof(events) / sizeof(es_event_type_t);
    
    if (es_subscribe(client, events, count) != ES_RETURN_SUCCESS)
    {
        fprintf(stderr, "Failed to subscribe to events\n");
        es_delete_client(client);
        return 1;
    }
    
    int arr[] {1,2,3,4};
    
    arr[5] = 7;
    
    std::unique_ptr<int> ptr {new int(20)};
    
    ptr.get()[2] = 5;
    //   // set up signal handling
    //   dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    //   dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGINFO, 0, queue);
    //
    //   if (source)
    //   {
    //      dispatch_source_set_event_handler(source, ^{
    //         sigHandler();
    //      });
    //   }
    //   // dispatch signal handling
    //   dispatch_resume(source);
    
    // dispatch es client
    dispatch_main();
    
    
}


