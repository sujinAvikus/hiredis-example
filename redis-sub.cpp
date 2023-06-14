#include <iostream>
#include <thread>
#include <chrono>
#include <hiredis/hiredis.h>

using namespace std::literals;

void consumerThread(redisContext* redis) {
    redisReply* reply = (redisReply*)redisCommand(redis, "SUBSCRIBE channel_name");
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Failed to subscribe: " << (reply == nullptr ? "Memory allocation failed" : reply->str) << std::endl;
        return;
    }
    freeReplyObject(reply);

    while (true) {
        // Receive message
        reply = nullptr;
        if (redisGetReply(redis, (void**)&reply) == REDIS_OK) {
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
                std::string message = reply->element[2]->str;
                std::cout << "Consumer: " << message << std::endl;
            }
        }
        freeReplyObject(reply);

        std::this_thread::sleep_for(1ms);
    }
}

int main() {
    // Redis connection setup
    redisContext* redis = redisConnect("localhost", 6379);
    if (redis == nullptr || redis->err) {
        std::cout << "Failed to connect to Redis: " << (redis == nullptr ? "Memory allocation failed" : redis->errstr) << std::endl;
        return 1;
    }

    std::thread consumer(consumerThread, redis);

    consumer.join();

    // Redis connection cleanup
    redisFree(redis);

    return 0;
}