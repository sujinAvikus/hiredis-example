#include <iostream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <hiredis/hiredis.h>
#include <thread>
#include <condition_variable>
#include <chrono>

using json = nlohmann::json;
using namespace std::literals;

// Shared variables
std::mutex mtx;
std::condition_variable cv;
bool isDataReady = false;
std::string retrievedJsonStr;

void producer(redisContext* c, const std::string& key, const std::string& jsonStr) {
	while (true) {
		while (!isDataReady) {
			try {
				// SET the value in Redis
				redisReply* reply = static_cast<redisReply*>(redisCommand(c, "SET %s %s", key.c_str(), jsonStr.c_str()));
				if (reply == nullptr) {
					std::cerr << "Error executing Redis SET command: " << c->errstr << std::endl;
					redisFree(c);
					continue;
				}
				freeReplyObject(reply);

				// Notify the consumer that data is ready
				{
					std::lock_guard<std::mutex> lock(mtx);
					isDataReady = true;
				}
				std::cout << "[PRODUCER] notify_one" << std::endl;
				cv.notify_one();
			} catch (const std::exception& e) {
				std::cerr << "Exception occurred in producer: " << e.what() << std::endl;
			}
		}
		std::this_thread::sleep_for(1ms);
	}
}

void consumer(redisContext* c, const std::string& key) {
	while (true) {
		try {
			// Wait until data is ready
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [] { return isDataReady; });

			std::cout << "[CONSUMER] acquire lock" << std::endl;

			// GET the value from Redis
			redisReply* reply = static_cast<redisReply*>(redisCommand(c, "GET %s", key.c_str()));
			if (reply == nullptr) {
				std::cerr << "Error executing Redis GET command: " << c->errstr << std::endl;
				redisFree(c);
				continue;
			}

			if (reply->type == REDIS_REPLY_STRING) {
				retrievedJsonStr = reply->str;
			} else {
				std::cout << "Key '" << key << "' does not exist" << std::endl;
			}

			freeReplyObject(reply);

			isDataReady = false;

		} catch (const std::exception& e) {
			std::cerr << "Exception occurred in consumer: " << e.what() << std::endl;
		}
	}
}

int main() {
	// Connect to Redis server
	redisContext* c = redisConnect("localhost", 6379);
	if (c == nullptr || c->err) {
		std::cerr << "Error connecting to Redis server: " << (c == nullptr ? "nullptr" : c->errstr) << std::endl;
		return 1;
	}

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

	// Create producer and consumer threads
	std::thread producerThread(producer, c, key, jsonStr);
	std::this_thread::sleep_for(10ms);
	std::thread consumerThread(consumer, c, key);

	// Wait for both threads to finish
	producerThread.join();
	consumerThread.join();

	return 0;
}
