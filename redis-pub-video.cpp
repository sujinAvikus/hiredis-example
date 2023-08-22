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

    const char* channel = "frame:ir";
    const char* video_path = "/workspace/detector-app/data/video/ir_hannara.mp4";
    const char* img_path = "/workspace/hiredis-example/temp.jpg";

    cv::VideoCapture video(video_path);
    cv::Mat img = cv::imread(img_path);


    if (!video.isOpened()) {
        std::cout << "Failed to open video file: " << video_path << std::endl;
        return;
    }
    
    double frameRate = video.get(cv::CAP_PROP_FPS);
    std::cout << "frameRate: " << frameRate << "FPS" << std::endl;
    int sleepDuration = static_cast<int>(1000 / frameRate); // Calculate sleep duration in milliseconds

    std::chrono::high_resolution_clock::time_point startTime, endTime;

    while (true) {

        startTime = std::chrono::high_resolution_clock::now();

        cv::Mat frame;
        video >> frame;
        
        if (frame.empty()) {
            std::cout << "End of video reached." << std::endl;
            break;
        }

        // 16'은 'CV_8UC3'에 해당하며, 이는 3개의 채널이 있는 8비트 부호 없는 정수
        std::cout << "Frame data type: " << frame.type() << std::endl;
        std::cout << "Frame size: " << frame.size() << std::endl;
        std::cout << "Frame channels: " << frame.channels() << std::endl;

        std::vector<uchar> buffer;
        // cv::imencode(".jpg", frame, buffer);
        cv::imencode(".jpg", img, buffer);
        

        // ERROR
        // std::vector<char> buffer(frame.data, frame.data + frame.total());

        if (buffer.empty()) {
            std::cout << "Buffer is empty. Failed to decode image." << std::endl;
            return;
        }

        std::cout << "buffer size: " << buffer.size() << std::endl;

        // Publish message
        // redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH %s %b", channel, buffer.data(), buffer.size());
        // if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        //     std::cout << "Failed to publish message" << std::endl;
        // }
        redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH %s %b", channel, buffer.data(), buffer.size());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Failed to publish message" << std::endl;
        }

        freeReplyObject(reply);

        // Save the image as RGB JPG

        // cv::Mat image = cv::imdecode(buffer, cv::IMREAD_COLOR);
        // if (image.empty()) {
        //     std::cout << "Failed to decode image" << std::endl;
        //     return;
        // }

        // std::string outputFilePath = "output.jpg";
        // cv::imwrite(outputFilePath, image);
        
        endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Elapsed time: " << duration << " ms, File size: " << buffer.size() << std::endl;

        int sleepAdjustment = sleepDuration - duration;
        if (sleepAdjustment > 0) {
            std::cout << "sleepAdjustment time: " << sleepAdjustment << "ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepAdjustment));
        }
    }

    video.release();
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
