#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <vector>
#include <sys/file.h>
#include <thread>
#include <string>
#include <random>
#include <sstream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
// #include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
// p_wav(); //main 重定义 SDL_main替换了，但是SDL_main貌似没有在SDL3.so中定义

using namespace std;
using namespace cv;
namespace fs = std::filesystem;
std::string getAbsPath(std::string filename)
{
    fs::path abs_p = fs::absolute(filename);
    return abs_p.string();
}
static const std::string WAV_RP = getAbsPath("sound/DIG.wav");
static const std::string SHARK_PT_RP = getAbsPath("image/resource.PNG");
static const std::string SHARK_SP_RP = getAbsPath("image/supplies.PNG");
static const std::string FINDED_RP = getAbsPath("find/");
static const std::string PLAYBACK_CMD = getAbsPath("mplayer") + " -loop 3 -af volume=30 -really-quiet " + WAV_RP;
static const std::string BINCAPTURE_CMD = "screencap -p ";
static const std::string SHELL = R"(#!/bin/bash
                                    pgrep shark
                                    pkill shark)";

FILE *pipe_;
std::vector<unsigned char> d_frame;
unsigned char c_buffer[512 * 1024];
size_t b_read;

std::string genRDString(const std::string &prefix = "find_", const std::string &extension = ".PNG")
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10000, 99999);
    std::ostringstream oss;
    oss << FINDED_RP << prefix << dist(gen) << extension;
    return oss.str();
}

cv::Mat getMatBySrcap(int flags)
{
    pipe_ = popen(BINCAPTURE_CMD.c_str(), "r");
    if (!pipe_)
    {
        std::cerr << "Cat not open pipe\n";
        return cv::Mat();
    }
    while ((b_read = fread(c_buffer, 1, sizeof(c_buffer), pipe_)) > 0)
    {

        d_frame.insert(d_frame.end(), c_buffer, c_buffer + b_read);
    }
    b_read = 0;
    pclose(pipe_);
    cv::Mat t_mat = cv::imdecode(d_frame, flags);
    d_frame.clear();
    return t_mat;
}
int clickByPoint(cv::Point center)
{
    std::ostringstream click;
    click << "input tap" << " " << std::to_string(center.x) << " " << std::to_string(center.y);
    system(click.str().c_str());
    return 1;
}
cv::Mat cropROI(const cv::Mat &src, cv::Rect roi)
{
    roi = roi & cv::Rect(0, 0, src.cols, src.rows);
    cv::imwrite(genRDString(), src(roi).clone());
    return cv::Mat();
}
int d_search(cv::Mat &t_mat, double min_scale = 1.0, double max_scale = 1.0, double scale_step = 0.1, double threshold = 0.8, cv::Rect roi = cv::Rect())
{
    cv::Mat c_mat = getMatBySrcap(cv::IMREAD_COLOR);
    if (c_mat.empty() || t_mat.empty())
    {
        std::cerr << "Input images are empty!" << std::endl;
        return -1;
    }
    if (roi.empty())
    {
        roi = cv::Rect(0, 0, c_mat.cols, c_mat.rows);
    }
    else
    {
        c_mat = cropROI(c_mat, roi);
    }
    double min_val, max_val, expect_max_val;
    cv::Point min_loc, max_loc, optimal_loc;
    cv::Mat result, t_mat_scaled, optimal_mat;
    expect_max_val = -1.0;
    for (double scale = min_scale; scale <= max_scale; scale += scale_step)
    {
        cv::resize(t_mat, t_mat_scaled, cv::Size(), scale, scale, cv::INTER_LINEAR);
        if (t_mat_scaled.rows > c_mat.rows || t_mat_scaled.cols > c_mat.cols)
        {
            continue;
        }
        cv::matchTemplate(c_mat, t_mat_scaled, result, cv::TM_CCOEFF_NORMED);
        cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
        // 迭代最佳值
        if (max_val > expect_max_val)
        {
            expect_max_val = max_val;
            optimal_loc = max_loc;
            optimal_mat = t_mat_scaled;
        }
        t_mat_scaled.release();
    }
    if (expect_max_val > threshold)
    {
        // std::cout << "find->" << "maxVal: " << expect_max_val << std::endl;
        // cv::rectangle(c_mat, optimal_loc, cv::Point(optimal_loc.x + optimal_mat.cols, optimal_loc.y + optimal_mat.rows), cv::Scalar(0, 255, 0), 1);
        // cv::imwrite(genRDString(), c_mat);
        cv::Point center(
            roi.x + optimal_loc.x + optimal_mat.cols / 2,
            roi.y + optimal_loc.y + optimal_mat.rows / 2);
        clickByPoint(center);
        return 1;
    }
    return 0;
};
// quote
void runCmdAsync(const std::string &command)
{
    // std::ofstream("log.txt", std::ios::binary) << "find Treasure";
    system(command.c_str());
}
static cv::Mat shark_pt, shark_sp;

int main(int argc, char *argv[])
{
    shark_pt = cv::imread(SHARK_PT_RP, cv::IMREAD_COLOR);
    shark_sp = cv::imread(SHARK_SP_RP, cv::IMREAD_COLOR);
    int ret, ret_;
    double threshold = 0.75;
    int fd = open("PKILL_SHARK", O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd != -1)
    {
        write(fd, SHELL.c_str(), SHELL.size());
        close(fd);
    }
    int fd_ = open("lock/shark.lock", O_RDWR | O_CREAT, 0666);
    if (fd_ == -1)
    {
        return -1;
    }

    if (flock(fd_, LOCK_EX | LOCK_NB) != 0)
    {
        std::cerr << "Another instance of the program is already running" << std::endl;
        close(fd_);
        return 1;
    }
    nice(-20);
    std::cout << "Current working directory: " << getAbsPath("") << std::endl;

    cropROI(cv::imread("/sdcard/a1.jpg", cv::IMREAD_COLOR), cv::Rect(736, 732, 384, 192));
    // std::cout << cv::getBuildInformation() << std::endl;

    // while (true)
    // {
    //     ret = d_search(shark_pt, 0.9, 1.1, 0.1, threshold);
    //     ret_ = d_search(shark_sp, 0.9, 1.1, 0.1, threshold);
    //     if (ret || ret_)
    //     {
    //         std::thread threadAsync(runCmdAsync, PLAYBACK_CMD);
    //         threadAsync.detach();
    //         std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //         std::for_each(std::begin(points), std::end(points), [](const cv::Point &p)
    //                       {
    //                         std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    //                         clickByPoint(p); });
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // };
    return 1;
};

// 2530 - 2530
//  1245 1490
//  280 880
// 挖掘机文本模板 (280, 15, 600,245)

// 挖掘机频道裁剪 (0, 525, 1800, 2530 - 2530)

// 挖掘机图标模板 (1175, 2385, 1415 -1175 , 2620 - 2385)
// 无人机 ()
// 基地图标模板 (1520, 2630, 1740 -1520 , 2850 - 2630)
// 组队图标模板 (1570, 1720, 1760 -1570 , 1865 - 1720)
// 加入组队模板 (743,738,170,170)

// mv /sdcard/shark ./ && chmod 777 ./shark && ./shark
