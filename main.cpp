#include <iostream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <hiredis/hiredis.h>

using json = nlohmann::json;

int main() {
    // Connect to Redis server
    redisContext* c = redisConnect("localhost", 6379);
    if (c == nullptr || c->err) {
        std::cerr << "Error connecting to Redis server: " << (c == nullptr ? "nullptr" : c->errstr) << std::endl;
        return 1;
    }

    // Send a Redis command
    redisReply* reply = static_cast<redisReply*>(redisCommand(c, "PING"));
    if (reply == nullptr) {
        std::cerr << "Error executing Redis command: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }

    // Print the response
    std::cout << "Received Redis response: " << reply->str << std::endl;

#if 0
    // Perform SET operation
    const char* key = "mykey";
    const char* value = "Hello, Redis!";
    reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", key, value));
    if (reply == nullptr) {
        std::cerr << "Error executing Redis SET command: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }

    // Perform GET operation
    reply = static_cast<redisReply*>(redisCommand(c, "GET %s", key));
    if (reply == nullptr) {
        std::cerr << "Error executing Redis GET command: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }
    if (reply->type == REDIS_REPLY_STRING) {
        std::cout << "Value for key '" << key << "': " << reply->str << std::endl;
    } else {
        std::cout << "Key '" << key << "' does not exist" << std::endl;
    }
#endif

    std::string key = "mykey";

    // Define the prediction 2D array
    std::vector<std::vector<int>> prediction = {
        {1, 2, 3, 4, 5, 6},
        {7, 8, 9, 10, 11, 12},
        {13, 14, 15, 16, 17, 18}
    };

    // Create a JSON object for the prediction
    json predictionJson;
    predictionJson["prediction"] = prediction;

    // Convert the prediction JSON to a string
    std::string jsonStr = predictionJson.dump();

    reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", key.c_str(), jsonStr.c_str()));
    if (reply == nullptr) {
        std::cerr << "Error executing Redis SET command: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }

    // GET the value from Redis and parse it back to the prediction
    reply = static_cast<redisReply*>(redisCommand(c, "GET %s", key.c_str()));
    if (reply == nullptr) {
        std::cerr << "Error executing Redis GET command: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        std::string retrievedJsonStr = reply->str;

        // Parse the JSON string back to the prediction JSON object
        json retrievedPredictionJson = json::parse(retrievedJsonStr);

        // Access the prediction array within the JSON object
        std::vector<std::vector<int>> retrievedPrediction = retrievedPredictionJson["prediction"];

        // Print the retrieved prediction 2D array
        std::cout << "Retrieved prediction array:" << std::endl;
        for (const auto& row : retrievedPrediction) {
            for (const auto& element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "Key '" << key << "' does not exist" << std::endl;
    }

    // Clean up
    freeReplyObject(reply);
    redisFree(c);

    return 0;
}
