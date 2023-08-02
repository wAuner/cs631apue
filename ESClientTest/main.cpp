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
#include <thread>
//#include <syncstream>

#include <bsm/libbsm.h>
#include <libproc.h>

#include <fmt/chrono.h>

using namespace std::literals;

std::mutex cout_mutex;

bool operator== (const timeval& lhs, const timeval& rhs) {
    return lhs.tv_sec == rhs.tv_sec && lhs.tv_usec == rhs.tv_usec;
}

std::atomic<bool> shutdownRequested{false};

void handle_event(es_client_t* client, const es_message_t* msg)
{
    switch (msg->event_type) {
            
        case ES_EVENT_TYPE_NOTIFY_FORK:
        {
            break;
        }
            
        case ES_EVENT_TYPE_NOTIFY_EXEC:
        {
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
    dispatch_queue_t eventQueue = dispatch_queue_create("myEventQueue", DISPATCH_QUEUE_SERIAL);
    
    es_client_t* client;
    es_new_client_result_t result = es_new_client(
                                                  &client, ^(es_client_t* c, const es_message_t* msg) {
                                                      es_retain_message(msg);
                                                      dispatch_async(eventQueue, ^{
                                                          if (!shutdownRequested) {
                                                              std::this_thread::sleep_for(50ms);
                                                              handle_event(c, msg);
                                                          }
                                                          es_release_message(msg);
                                                      });
                                                  }
                                                  );
    
    if (result != ES_NEW_CLIENT_RESULT_SUCCESS)
    {
        fprintf(stderr, "Failed to create new ES client \n");
        return 1;
    }
    
    es_event_type_t events[] = {ES_EVENT_TYPE_NOTIFY_FORK, ES_EVENT_TYPE_NOTIFY_EXEC, ES_EVENT_TYPE_NOTIFY_EXIT};
    uint32_t count = sizeof(events) / sizeof(es_event_type_t);
    
    if (es_subscribe(client, events, count) != ES_RETURN_SUCCESS)
    {
        fprintf(stderr, "Failed to subscribe to events\n");
        es_delete_client(client);
        return 1;
    }
    
    std::this_thread::sleep_for(2s);
    if (es_unsubscribe_all(client) != ES_RETURN_SUCCESS) {
        std::scoped_lock lock{cout_mutex};
        std::cerr << "Hupsi\n";
    }
    else {
        fmt::println("\nUnsubscribed at {}", std::chrono::system_clock::now());
    }
    fmt::println("Going to sleep for 2s");
    std::this_thread::sleep_for(2s);
    
    es_delete_client(client);
    shutdownRequested = true;
    // wait till event queue is empty
    dispatch_sync(eventQueue, ^{});
    fmt::println("Client deleted");
}


