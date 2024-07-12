#pragma once

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

// A simple OpenCV & SFML program to record timelapses for simulations.
// capture() will attempt to capture the current window as long as the duration > capture_rate_seconds
// save_video() will attempt to save the video as a.mp4
class Timelapse
{
private:
    sf::RenderWindow& window_;
    float capture_rate_;
    std::vector<cv::Mat> frames_;
    sf::Clock clock_;

public:
    Timelapse(sf::RenderWindow& capture_window, const float capture_rate_seconds)
	: window_(capture_window), capture_rate_(capture_rate_seconds)
    {
        std::cout << "OpenCV version : " << CV_VERSION << "\n";
    }

    void capture()
	{
        if (clock_.getElapsedTime().asSeconds() >= capture_rate_) 
        {
            sf::Texture texture;
            texture.create(window_.getSize().x, window_.getSize().y);
            texture.update(window_);
            const sf::Image screenshot = texture.copyToImage();

            const auto size = static_cast<sf::Vector2i>(screenshot.getSize());

            cv::Mat frame(size.y, size.x, CV_8UC4);
            std::memcpy(frame.data, screenshot.getPixelsPtr(), size.x * size.y * 4);
            cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGR);

            frames_.push_back(frame);
            clock_.restart();
        }
    }

    void save_video(const std::string& filename, const int fps = 60) const
    {
        if (frames_.empty())
        {
            std::cout << "No frames captured. Cannot save video." << "\n";
            return;
        }

        const cv::Size frame_size = frames_[0].size();

        // Ensure the file has a proper extension
        std::string actual_filename = filename;
        if (actual_filename.substr(actual_filename.find_last_of(".") + 1) != "mp4") 
        {
            actual_filename += ".mp4";
        }

        // Try different codecs
        const std::vector<int> codecs = {
            cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
            cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
            cv::VideoWriter::fourcc('X', 'V', 'I', 'D'),
            cv::VideoWriter::fourcc('H', '2', '6', '4')};

        cv::VideoWriter video;
        bool success = false;

        for (const auto& codec : codecs) 
        {
            video.open(actual_filename, codec, fps, frame_size);
            if (video.isOpened()) 
            {
                success = true;
                break;
            }
        }

        if (!success) {

            std::cout << "Failed to create video writer. Try installing additional codecs." << "\n";
            return;
        }

        for (const auto& frame : frames_) 
        {
            video.write(frame);
        }

        video.release();
        std::cout << "Video saved as " << actual_filename << "\n";
    }
};