#include <iostream>
#include <cstring>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <event2/event.h>
#include <unistd.h>

// Global Redis context
redisAsyncContext* rc;

// Event base for the event loop
event_base* eventBase;

// Callback function to handle subscribed messages
void handleMessage(redisAsyncContext* context, void* reply, void* privateData) {
    if (reply == nullptr) {
        std::cout << "Failed to receive message from Redis" << std::endl;
        return;
    }

    // Process the received message based on the Redis reply structure
    redisReply* rp = static_cast<redisReply*>(reply);
    if (rp->type == REDIS_REPLY_ARRAY && rp->elements == 3) {
        std::string messageType = rp->element[0]->str;
        std::string channel = rp->element[1]->str;
        std::string message = rp->element[2]->str;

        std::cout << "Received message: " << message << " on channel: " << channel << std::endl;
    }
}

// Callback function for keyboard input event
void handleKeyboardInput(evutil_socket_t fd, short event, void* arg) {
    char key;
    std::cin >> key;

    if (key == 'q') {
        // Stop the event loop
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
	if(status != REDIS_OK){
		printf("Error: %s\n", ctx->errstr);
		return;
	}
	printf("Disconnected...\n");
}

int main() {
    // Create an event base for the event loop
    eventBase = event_base_new();

    // Create a Redis async context
    rc = redisAsyncConnect("127.0.0.1", 6379);
    if (rc->err) {
        std::cout << "Error connecting to Redis: " << rc->errstr << std::endl;
        return 1;
    }

    // Attach the Redis context to the event loop
    redisLibeventAttach(rc, eventBase);

    redisAsyncSetConnectCallback(rc, connect_callback);

	redisAsyncSetDisconnectCallback(rc, disconnect_callback);

    // Subscribe to a Redis channel
    const char* channelName = "myChannel";
    redisAsyncCommand(rc, handleMessage, nullptr, "SUBSCRIBE %s", channelName);

    // Create an event for keyboard input
    struct event* keyboardEvent = event_new(eventBase, STDIN_FILENO, EV_READ | EV_PERSIST, handleKeyboardInput, nullptr);
    event_add(keyboardEvent, nullptr);

    // Start the event loop
    event_base_dispatch(eventBase);

    // Clean up
    event_base_free(eventBase);
    // redisAsyncFree(redisContext);

    return 0;
}
