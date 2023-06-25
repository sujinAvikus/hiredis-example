#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <hiredis/hiredis.h>
#include <opencv2/opencv.hpp>

using namespace std::literals;

void producerThread(redisContext* redis) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for the consumer thread to start first

    const char* channel = "channel_name";
    const char* image_path = "eo.jpg";

    std::ifstream file(image_path, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open file: " << image_path << std::endl;
        return;
    }

    std::chrono::high_resolution_clock::time_point startTime, endTime;

    while (true) {

        startTime = std::chrono::high_resolution_clock::now();

        // Determine the file size
        // 파일 포인터를 파일 끝으로 이동하여 파일 크기를 결정할 수 있다
        file.seekg(0, std::ios::end);
        // 파일의 끝 위치에서의 오프셋 반환
        std::streamsize file_size = file.tellg();
        // 파일 포인터를 처음으로 이동시킴
        file.seekg(0, std::ios::beg);

        // Allocate a buffer to hold the file data
        std::vector<char> buffer(file_size);

        // Read the file data into the buffer
        file.read(buffer.data(), file_size);
 
        cv::Mat frame;
        frame = cv::imread(image_path, cv::IMREAD_COLOR);
        
        std::cout << "file_size: " << file_size << std::endl;
        std::cout << "Frame size: " << frame.total() << std::endl;

        // Publish message
        redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH %s %b", channel, buffer.data(), file_size);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Failed to publish message for file: " << image_path << std::endl;
        }

        cv::Mat image = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cout << "Failed to decode image" << std::endl;
            return;
        }

        // Save the image as RGB JPG
        std::string outputFilePath = "output.jpg";
        cv::imwrite(outputFilePath, image);
        
        freeReplyObject(reply);

        endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Elapsed time: " << duration << " ms, File size: " << file_size << std::endl;

        std::this_thread::sleep_for(1s);
    }

    file.close();
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