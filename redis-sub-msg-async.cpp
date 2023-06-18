#include <iostream>
#include <string>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <event2/event.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

// Event base for the event loop
event_base* eventBase;

// Callback function to handle subscribed messages
void handleMessage(redisAsyncContext* context, void* reply, void* privateData) {
    redisReply* rp =  static_cast<redisReply*>(reply);
	if(!rp){
		printf("Invalid parameter.\n");
		return;
	}

	if(rp->type == REDIS_REPLY_ARRAY && rp->elements == 3){
		if(strcmp(rp->element[0]->str, "subscribe") != 0){
            // std::cout << std::unitbuf;  // Set std::cout to be unbuffered
            // std::cout << "Received message: " << rp->element[2]->str << std::endl;
			printf("Received [%s] channel %s: %s\n", (char*)privateData, rp->element[1]->str, rp->element[2]->str);
		}
	}
}

// Callback function for keyboard input event
void handleKeyboardInput(evutil_socket_t fd, short event, void* arg) {
    std::cout << "Received handleKeyboardInput" << std::endl;
    char key;
    std::cin >> key;

    if (key == 'q') {
        // Stop the event loop
        std::cout << "Stop the Subscribe event loop" << std::endl;
        event_base_loopbreak(eventBase);
    }
}

void connect_callback(const redisAsyncContext* ctx, int status)
{
	if(status != REDIS_OK){
		printf("Error: %s\n", ctx->errstr);
		return;
	}
	printf("Connected...\n");
}

void disconnect_callback(const redisAsyncContext* ctx, int status)
{
    printf("Disconnected...\n");

	if(status != REDIS_OK){
		printf("Error: %s\n", ctx->errstr);
		return;
	}
}

int main() {
    // Global Redis context
    redisAsyncContext* rc;

    // Create an event base for the event loop
    eventBase = event_base_new();

    // Create a Redis async context
    rc = redisAsyncConnect("localhost", 6379);
    if (rc->err) {
        std::cout << "Error connecting to Redis: " << rc->errstr << std::endl;
        return 1;
    }

    // Attach the Redis context to the event loop
    redisLibeventAttach(rc, eventBase);

    redisAsyncSetConnectCallback(rc, connect_callback);

	redisAsyncSetDisconnectCallback(rc, disconnect_callback);

    // Subscribe to a Redis channel
    const char* channelName = "channel_name";
    redisAsyncCommand(rc, handleMessage, NULL, "SUBSCRIBE %s", channelName);

    // // Create an event for keyboard input
    struct event* keyboardEvent = event_new(eventBase, STDIN_FILENO, EV_READ | EV_PERSIST, handleKeyboardInput, nullptr);
    event_add(keyboardEvent, nullptr);

    std::cout << "Start!\n";

    // Start the event loop
    event_base_dispatch(eventBase);

    std::cout << "Clean up!\n";
    // Clean up
    if (eventBase) 
        event_base_free(eventBase);

    if (rc)
    {
        // redisAsyncDisconnect(rc);
        redisAsyncFree(rc);
    }

    return 0;
}
