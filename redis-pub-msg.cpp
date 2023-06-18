#include <iostream>
#include <thread>
#include <chrono>
#include <hiredis/hiredis.h>

using namespace std::literals;

void producerThread(redisContext* redis) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for the consumer thread to start first

    const char* channel = "channel_name";
    const char* messages[] = { "Message 1", "Message 2", "Message 3" };
    const int numMessages = sizeof(messages) / sizeof(messages[0]);

    while (true)
    {
        for (int i = 0; i < numMessages; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Send a message every second

            // Publish message
            redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH %s %s", channel, messages[i]);
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                std::cout << "Failed to publish message: " << (reply == nullptr ? "Memory allocation failed" : reply->str) << std::endl;
                return;
            }

            std::cout << "Publish message: " << messages[i] << std::endl;

            freeReplyObject(reply);
        }
    }
}

int main() {
    // Redis connection setup
    redisContext* redis = redisConnect("localhost", 6379);
    if (redis == nullptr || redis->err) {
        std::cout << "Failed to connect to Redis: " << (redis == nullptr ? "Memory allocation failed" : redis->errstr) << std::endl;
        return 1;
    }

    std::thread producer(producerThread, redis);

    producer.join();

    // Redis connection cleanup
    redisFree(redis);

    return 0;
}