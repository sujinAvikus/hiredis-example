#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <hiredis/hiredis.h>

bool producerReadyFlag = false;
bool consumerReadyFlag = false;
std::condition_variable producerCv;
std::condition_variable consumerCv;
std::mutex mtx;

#if 0

Messages sent by other clients to these channels will be pushed by Redis to all the subscribed clients.

A client subscribed to one or more channels should not issue commands, although it can subscribe and unsubscribe to and from other channels.
The replies to subscription and unsubscription operations are sent in the form of messages, so that the client can just read a coherent stream of messages where the first element indicates the type of message. 
The commands that are allowed in the context of a subscribed client are SUBSCRIBE, PSUBSCRIBE, UNSUBSCRIBE, PUNSUBSCRIBE, PING and QUIT.

#endif

void producerThread(redisContext* redis) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for the consumer thread to start first

    const char* channel = "channel_name";
    const char* messages[] = { "Message 1", "Message 2", "Message 3" };
    const int numMessages = sizeof(messages) / sizeof(messages[0]);

    for (int i = 0; i < numMessages; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Send a message every second
        std::unique_lock<std::mutex> lock(mtx);

        // (2), (6)
        producerCv.wait(lock, [] { return consumerReadyFlag; }); // Wait for the consumer to be ready

        // Publish message
        redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH %s %s", channel, messages[i]);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Failed to publish message: " << (reply == nullptr ? "Memory allocation failed" : reply->str) << std::endl;
            return;
        }
        freeReplyObject(reply);

        producerReadyFlag = true;
        consumerReadyFlag = false;
        // (4)
        consumerCv.notify_one();
    }
}

void consumerThread(redisContext* redis) {
    redisReply* reply = (redisReply*)redisCommand(redis, "SUBSCRIBE channel_name");
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Failed to subscribe: " << (reply == nullptr ? "Memory allocation failed" : reply->str) << std::endl;
        return;
    }
    freeReplyObject(reply);

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        // (1)
        consumerReadyFlag = true;
        producerCv.notify_one();
        // (3)
        consumerCv.wait(lock, [] { return producerReadyFlag; }); // Wait for the producer to be ready

        // Receive message
        reply = nullptr;
        if (redisGetReply(redis, (void**)&reply) == REDIS_OK) {
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
                std::string message = reply->element[2]->str;
                std::cout << "Consumer: " << message << std::endl;
            }
        }
        freeReplyObject(reply);

        // (5)
        consumerReadyFlag = false;
        producerReadyFlag = false;
        producerCv.notify_one();
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
    std::thread consumer(consumerThread, redis);

    producer.join();
    consumer.join();

    // Redis connection cleanup
    redisFree(redis);

    return 0;
}