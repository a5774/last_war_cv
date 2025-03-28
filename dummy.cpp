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
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <signal.h>
#include <linux/input.h>
#include <dirent.h>
// #include <xf86drmMode.h>
// #include <EGL/egl.h>
// #include <GLES3/gl3.h>

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
static const std::string HOME_DETECT_RP = getAbsPath("image/home.PNG");
static const std::string GOLD_JOIN_RP = getAbsPath("image/join.PNG");
static const std::string DIG_TIP_RP = getAbsPath("image/small.PNG");
static const std::string DIG_DRONES_RP = getAbsPath("image/drones.PNG");
static const std::string DIG_TREASURE_RP = getAbsPath("image/treasure.PNG");
static const std::string GOLD_TARGET_RP = getAbsPath("image/target.PNG");
static const std::string FINDED_RP = getAbsPath("find/");
static const std::string PLAYBACK_CMD = getAbsPath("mplayer") + " -loop 3 -af volume=30 -really-quiet " + WAV_RP;
static const std::string BINCAPTURE_CMD = "screencap -p ";
static const std::string KEYCODE_BACK_CMD = "input keyevent KEYCODE_BACK";
static const std::string VOLUME_REDUCE_CMD = "input keyevent 25";
static const std::string KEYCHECK_BIN = getAbsPath("keycheck");

static bool logs = false, asmb = false, digs = false, qick = false;
static int timer = 1000;
static uint intv = 50, cctm = 50;

std::string genRDString(const std::string &prefix = "find_", const std::string &extension = ".PNG")
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10000, 99999);
    std::ostringstream oss;
    oss << FINDED_RP << prefix << dist(gen) << extension;
    return oss.str();
}
FILE *pipe_;
std::vector<unsigned char> d_frame;
unsigned char c_buffer[512 * 1024];
size_t b_read;
cv::Mat getScreenFrame(int flags)
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
    if (t_mat.empty())
    {
        std::cerr << "empty mat\n";
        return cv::Mat();
    }
    return t_mat;
}
int clickByPoint(cv::Point &center, int64_t e = 0)
{
    if (e)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(e));
    }
    std::ostringstream click;
    click << "input tap" << " " << std::to_string(center.x) << " " << std::to_string(center.y);
    system(click.str().c_str());
    return 1;
}
cv::Mat cropROI(const cv::Mat &src, cv::Rect roi)
{
    roi = roi & cv::Rect(0, 0, src.cols, src.rows);
    return src(roi);
}

std::chrono::high_resolution_clock::time_point startTimer()
{
    return std::chrono::high_resolution_clock::now();
}
void getElapsedTime(std::chrono::high_resolution_clock::time_point start_)
{
    auto end_ = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(end_ - start_);
    std::cout << "spend: " << elapsed.count() << " sec" << std::endl;
    // undefined behavior (UB)
    // return
}

bool d_search_single(cv::Mat &t_mat, double threshold = 0.8, cv::Rect roi = cv::Rect(), const cv::Mat &mat = cv::Mat(), int offse_x = 0, int offse_y = 0, const bool click = true)
{
    // auto stime = startTimer();
    cv::Mat c_mat = mat.empty() ? getScreenFrame(cv::IMREAD_COLOR) : mat;
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
        // 当roi为empty时,则无需corp减少开销
        c_mat = cropROI(c_mat, roi);
    }
    cv::Mat result;
    cv::Point min_loc, max_loc;
    double min_val, max_val;
    cv::matchTemplate(c_mat, t_mat, result, cv::TM_CCOEFF_NORMED);
    cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
    if (logs)
    {
        std::cout << "find->" << "maxVal: " << max_val << std::endl;
    }
    if (max_val > threshold)
    {
        cv::Point center(
            offse_x + roi.x + max_loc.x + t_mat.cols / 2,
            offse_y + roi.y + max_loc.y + t_mat.rows / 2);
        // cv::rectangle(c_mat, max_loc, cv::Point(max_loc.x + t_mat.cols, max_loc.y + t_mat.rows), cv::Scalar(0, 255, 0), 1);
        // cv::imwrite(genRDString(), c_mat);
        // std::cout << "pointer: " << center << std::endl;
        !click || clickByPoint(center);
        // getElapsedTime(stime);
        return true;
    }
    // getElapsedTime(stime);
    return false;
}
int d_search(cv::Mat &t_mat, double min_scale = 1.0, double max_scale = 1.0, double scale_step = 0.1, double threshold = 0.8, cv::Rect roi = cv::Rect(), const cv::Mat &mat = cv::Mat())
{
    cv::Mat c_mat = mat.empty() ? getScreenFrame(cv::IMREAD_COLOR) : mat;
    // std::cout << "size->" << c_mat.size() << t_mat.size() << mat.size() << std::endl;
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
        // std::cout << "find->" << "maxVal: " << std::endl;
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
        if (logs)
        {
            std::cout << "find->" << "maxVal: " << expect_max_val << std::endl;
        }
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

void sendEvent(int fd_, __u16 type, __u16 code, __s32 value, uint interval = 0U)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    write(fd_, &event, sizeof(event));
    if (interval > 0)
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}
void highFrequencyClick(int fd, cv::Point center, uint width, uint height, uint count, uint interval)
{
    // 物理触摸映射到像素坐标
    // x = x * (18000 / 1800);
    // y = y * (28800 / 2880);
    int rand_x, rand_y;
    cv::Point base(center.x - (width / 2), center.y - (height / 2));
    // auto stime = startTimer();
    while (count > 0)
    {

        rand_x = (base.x + rand() % width) * 10;
        rand_y = (base.y + rand() % height) * 10;
        // std::cout << "coordinate: " << rand_x << "," << rand_y << std::endl;
        // getevent -p /dev/input/event6
        sendEvent(fd, EV_ABS, ABS_MT_TRACKING_ID, 0xff);
        sendEvent(fd, EV_ABS, ABS_MT_POSITION_X, rand_x);
        sendEvent(fd, EV_ABS, ABS_MT_POSITION_Y, rand_y);
        // 3. 设置触摸面积（可选) max: 1 => 255,5 => 0.02
        sendEvent(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 5);
        // 4. 设置压力值 max: 1 => 1000,50 => 0.05
        sendEvent(fd, EV_ABS, ABS_MT_PRESSURE, 50);
        sendEvent(fd, EV_KEY, BTN_TOUCH, 1);
        // 6. 同步事件(通知内核事件结束)
        sendEvent(fd, EV_SYN, SYN_REPORT, 0, 20U);
        sendEvent(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
        sendEvent(fd, EV_ABS, ABS_MT_PRESSURE, 0);
        sendEvent(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
        sendEvent(fd, EV_KEY, BTN_TOUCH, 0);
        sendEvent(fd, EV_SYN, SYN_REPORT, 0, interval);

        count--;
    }
    // getElapsedTime(stime);
}
std::string findTouchEventDevice()
{

    std::string _prefix = "/dev/input/event";
    const char *input_dev = "/sys/class/input/";
    DIR *d_dir = opendir(input_dev);
    if (!d_dir)
    {
        std::cerr << "Cant open dir" << std::endl;
    }
    struct dirent *entry;
    while ((entry = readdir(d_dir)) != nullptr)
    {
        if (strncmp(entry->d_name, "input", 5) == 0)
        {
            std::string device_name = std::string(input_dev) + entry->d_name + "/name";
            int fd = open(device_name.c_str(), O_RDONLY);
            if (fd < 0)
            {
                std::cerr << "Cant open file" << std::endl;
                continue;
            }
            char device[256] = {0};
            read(fd, device, sizeof(device) - 1);
            close(fd);
            if (strstr(device, "NVTCapacitiveTouchScreen"))
            {
                closedir(d_dir);
                return _prefix + std::string(entry->d_name + 5);
            }
        }
    }
    closedir(d_dir);
    return _prefix + "6";
}
int initEventDevices()
{
    std::string evtx = findTouchEventDevice();
    std::cout << evtx << std::endl;
    int fd_ex = open(evtx.c_str(), O_WRONLY);
    if (fd_ex < 0)
    {
        std::cerr << "Cant open intput device" << std::endl;
        return -1;
    }
    return fd_ex;
}
std::function<void(int)> signalHandler;
void bridgeFunction(int signum)
{
    signalHandler(signum);
}
int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "qladt:i:c:")) != -1)
    {
        switch (opt)
        {
        case 'l':
            logs = true;
            break; // -logs
        case 'a':
            asmb = true;
            break; // -asmb
        case 'd':
            digs = true;
            break; // -digs
        case 'q':
            qick = true;
            break; // -quick
        case 't':
            timer = std::stoi(optarg);
            break; // -dely <value>
        case 'i':
            intv = std::stoul(optarg);
            break; // -intv <value>
        case 'c':
            cctm = std::stoul(optarg);
            break; // -cctm <value>
        default:
            std::cerr << "Unkonw Params" << std::endl;
            break;
        }
    }
    std::cout << "logs: " << logs << std::endl;
    std::cout << "asmb: " << asmb << std::endl;
    std::cout << "digs: " << digs << std::endl;
    std::cout << "timer: " << timer << std::endl;
    std::cout << "intv: " << intv << std::endl;
    std::cout << "cctm: " << cctm << std::endl;
    std::cout << "qick: " << qick << std::endl;

    int fd_ = open("lock/gold.lock", O_RDWR | O_CREAT, 0666);
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
    cv::Point TownBase(900, 2740);
    cv::Point PrepDig(900, 1270);
    cv::Point Digg(900, 1725);
    cv::Point Dispatch(900, 2180);
    cv::Point Back(120, 2715);
    cv::Point Bonus(875, 1240);
    cv::Point Target(1665, 1795);

    cv::Mat m_GT = cv::imread(GOLD_TARGET_RP, cv::IMREAD_COLOR);
    cv::Mat m_GJ = cv::imread(GOLD_JOIN_RP, cv::IMREAD_COLOR);
    cv::Mat m_DP = cv::imread(DIG_TIP_RP, cv::IMREAD_COLOR);
    cv::Mat m_DT = cv::imread(DIG_DRONES_RP, cv::IMREAD_COLOR);
    cv::Mat m_DD = cv::imread(DIG_TREASURE_RP, cv::IMREAD_COLOR);
    cv::Mat m_HD = cv::imread(HOME_DETECT_RP, cv::IMREAD_COLOR);
    // 挖掘机频道裁剪 (0, 525, 1800, 2530 - 2530)
    // 挖掘机图标模板 (1175, 2385, 1415 -1175 , 2620 - 2385)
    // 基地图标模板 (1520, 2630, 1740 -1520 , 2850 - 2630)
    // 组队图标模板 (1570, 1720, 1760 -1570 , 1865 - 1720)
    cv::Rect CORP_GJ_LIMIT = cv::Rect(736, 400, 384, 2120);
    cv::Rect CORP_TD_LIMIT = cv::Rect(0, 525, 900, 2005);
    cv::Rect CORP_DP = cv::Rect(1175, 2385, 240, 235);
    cv::Rect CORP_GT = cv::Rect(1570, 1720, 190, 150);
    cv::Rect CORP_HD = cv::Rect(1520, 2630, 220, 220);
    std::cout << "power by 牛牛" << std::endl;
    bool hasGT, hasGJ, hasDP;
    pid_t pid = fork();
    if (pid == 0)
    {

        setsid();
        pid_t ppid = getppid();
        int fd_ex = initEventDevices();
        // execl("./keycheck", "keycheck", nullptr);
        while (true)
        {
            int exit_ = system(KEYCHECK_BIN.c_str());
            exit_ = ((exit_ >> 8) & 0xFF) | ((exit_ & 0xFF) << 8);
            std::cout << "keyCode: " << exit_ << std::endl;
            switch (exit_)
            {
            case 42:
                kill(ppid, SIGSTOP);
                system(VOLUME_REDUCE_CMD.c_str());
                highFrequencyClick(fd_ex, PrepDig, 10, 10, cctm, intv);
                kill(ppid, SIGCONT);
                break;
            case 41:
                std::cout << "释放flock" << std::endl;
                kill(ppid, SIGTERM);
                flock(fd_, LOCK_UN);
                close(fd_ex);
                exit(1);
                break;
            }
        }
    };
    if (pid > 0)
    {
        signalHandler = [&](int signum)
        {
            flock(fd_, LOCK_UN);
            kill(pid, SIGTERM);
            exit(signum);
        };
        signal(SIGINT, bridgeFunction);
        while (true)
        {
            // auto stime = startTimer();
            // spend lot of time
            // system("screencap -p > /dev/null");lsmod
            // getElapsedTime(stime);
            if (qick)
            {
                clickByPoint(Target);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                hasGJ = d_search_single(m_GJ, 0.85, CORP_GJ_LIMIT, cv::Mat(), -96, 0);
                if (hasGJ)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(600));
                    clickByPoint(Dispatch);
                }
                else
                {
                    clickByPoint(Back);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                continue;
            }
            cv::Mat c_mat = getScreenFrame(cv::IMREAD_COLOR);
            if (asmb)
            {

                hasGT = d_search_single(m_GT, 0.87, CORP_GT, c_mat);
                if (hasGT)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                    hasGJ = d_search_single(m_GJ, 0.85, CORP_GJ_LIMIT, cv::Mat(), -96, 0);
                    if (hasGJ)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(600));
                        clickByPoint(Dispatch);
                    }
                    else
                    {
                        clickByPoint(Back);
                    }
                }
                else
                {
                    d_search_single(m_HD, 0.9, CORP_HD, c_mat, 0, 0, false) || system(KEYCODE_BACK_CMD.c_str());
                }
            }
            if (digs)
            {
                hasDP = d_search_single(m_DP, 0.70, CORP_DP, c_mat);
                if (hasDP)
                {
                    std::thread threadAsync(runCmdAsync, PLAYBACK_CMD);
                    threadAsync.detach();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    cv::Mat d_mat = getScreenFrame(cv::IMREAD_COLOR);
                    bool find_T = d_search_single(m_DT, 0.85, CORP_TD_LIMIT, d_mat) || d_search_single(m_DD, 0.85, CORP_TD_LIMIT, d_mat);
                    if (find_T)
                    {
                        clickByPoint(PrepDig, 1000);
                        clickByPoint(Digg, 1000);
                        clickByPoint(Dispatch, 1000);
                        // clickByPoint(TownBase, 700);
                        // while (condition)ls
                        // {
                        //
                        // }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(timer));
        };
    }
    return 1;
};
//  mv /sdcard/dummy ./ && chmod 777 dummy && ./dummy -a -t 500
// ./gold -logs -digs
// cmake . && make && adb push ./dummy  /sdcard
// service call SurfaceFlinger 1
// cd /data/lost+found/game
