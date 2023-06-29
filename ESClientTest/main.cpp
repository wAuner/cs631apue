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

    dispatch_main();
    
    
}


