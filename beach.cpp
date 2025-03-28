#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/file.h>
#include <sstream>
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
int main(int argc, char *argv[])
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

//  mv /sdcard/beach ./ && chmod 777 beach && ./beach
// ./gold -logs -digs
// cmake . && make && adb push ./detect  /sdcard
// service call SurfaceFlinger 1
