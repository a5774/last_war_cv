#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/file.h>
#include <sstream>
#include <dirent.h>
#include <string.h>
#include <linux/input.h>
#include <signal.h>
// #include <EGL/egl.h>
// #include <GLES3/gl3.h>

// p_wav(); //main 重定义 SDL_main替换了，但是SDL_main貌似没有在SDL3.so中定义
using namespace std;
namespace fs = std::filesystem;
std::string getAbsPath(std::string filename)
{
    fs::path abs_p = fs::absolute(filename);
    return abs_p.string();
}
static const std::string GO_AUDIO_RP = getAbsPath("sound/go.mp3");
static const std::string AO_AUDIO_RP = getAbsPath("sound/ao.mp3");
static const std::string VOLUME_REDUCE_CMD = "input keyevent 25";
static const std::string VOLUME_INCARE_CMD = "input keyevent 24";
static const std::string GO = getAbsPath("mplayer") + " -really-quiet " + GO_AUDIO_RP;
static const std::string AO = getAbsPath("mplayer") + " -really-quiet " + AO_AUDIO_RP;
static const std::string KEYCHECK_BIN = getAbsPath("keycheck");

std::string execCommand(const char *cmd)
{
    char buffer[128];
    std::string result;
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        return "popen failed!";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe); // 关闭管道
    return result;
}
std::string findTouchEventDevice(std::string name)
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
            // 以 nullptr 作为指针数组的结束标志
            char device[256] = {0};
            read(fd, device, sizeof(device) - 1);
            device[strcspn(device, "\n")] = 0;
            close(fd);
            if (std::string(device) == name)
            {
                closedir(d_dir);
                return _prefix + std::string(entry->d_name + 5);
            }
        }
    }
    closedir(d_dir);
    return _prefix + "6";
}
int powerkeyDownable(std::string device)
{
    int fd = open(device.c_str(), O_RDONLY);

    if (fd < 0)
    {
        std::cerr << "无法打开设备: " << device << std::endl;
        return 0;
    }
    struct input_event ev;
    while (true)
    {
        int size = sizeof(ev);
        ssize_t n = read(fd, &ev, size);
        if (n == size)
        {
            if (ev.type == EV_KEY && ev.code == KEY_POWER)
            {
                if (ev.value)
                {
                    std::cout << "powerDown" << std::endl;
                    close(fd);
                    return 1;
                }
                std::cout << "keyCode: " << ev.code << std::endl;
            }
        }
        else
        {
            std::cerr << "read dev failure" << std::endl;
            break;
        }
    }
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    std::string dev = findTouchEventDevice("pmic_pwrkey");
    pid_t pid = fork();
    if (pid == 0)
    {

        setsid();
        pid_t ppid = getppid();
        if (powerkeyDownable(dev))
        {
            kill(ppid, SIGTERM);
            exit(1);
        }
    }
    if (pid > 0)
    {

        std::string out;
        std::ostringstream oss;
        std::string package = "com.tencent.tmgp.supercell.boombeach";
        system("iptables -F");
        while (true)
        {
            oss.str("");
            int exit_ = system(KEYCHECK_BIN.c_str());
            exit_ = ((exit_ >> 8) & 0xFF) | ((exit_ & 0xFF) << 8);
            std::cout << "keyCode: " << exit_ << std::endl;
            oss << "dumpsys package " << package << " | grep userId= | awk -F'=' '{print $2}' |  tr -d '\n'";
            out = execCommand(oss.str().c_str());
            std::cout << "uid: " << out << std::endl;
            switch (exit_)
            {
            case 42:
                if (!out.empty())
                {
                    oss.str("");
                    oss << "iptables -I OUTPUT -m owner --uid-owner " << out << " -j REJECT";
                    std::cout << "rule: " << oss.str() << std::endl;
                    system(oss.str().c_str());
                    system(GO.c_str());
                };
                system(VOLUME_REDUCE_CMD.c_str());
                break;
            case 41:
                oss.str("");
                oss << "am force-stop " << package;
                system(oss.str().c_str());
                if (!out.empty())
                {
                    oss.str("");
                    oss << "iptables -D OUTPUT -m owner --uid-owner " << out << " -j REJECT";
                    std::cout << "rule: " << oss.str() << std::endl;
                    system(oss.str().c_str());
                    system(AO.c_str());
                };
                system(VOLUME_INCARE_CMD.c_str());
                break;
            }
        }
    }
}

//  mv /sdcard/beach ./ && chmod 777 beach && ./beach
// ./gold -logs -digs
// cmake . && make && adb push ./detect  /sdcard
// service call SurfaceFlinger 1
