
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
#include <chrono>
#include <signal.h>
#include <linux/input.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>

#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
/* EGLDisplay display;
EGLSurface surface;
EGLContext context;
bool initEGL()
{
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE};

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);

    // 绑定到系统默认的屏幕缓冲区 (EGL_BACK_BUFFER)
    // surface = eglCreateWindowSurface(display, config, (nativeWindow)0, nullptr);

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE};

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    eglMakeCurrent(display, surface, surface, context);

    return true;
}

void captureScreen(int width, int height)
{
    std::vector<unsigned char> pixels(width * height * 4); // RGBA 格式

    // 绑定系统屏幕缓冲区
    glReadBuffer(GL_BACK); // 读取屏幕后缓冲区

    // 从屏幕帧缓冲区读取像素数据
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // 转为 OpenCV 的 cv::Mat 格式 (默认 RGBA)
    cv::Mat image(height, width, CV_8UC4, pixels.data());
    // OpenGL 是倒着存储的，需要翻转
    cv::flip(image, image, 0);
    // RGBA 转 BGR (OpenCV 默认 BGR)
    cv::Mat bgrImage;
    cv::cvtColor(image, bgrImage, cv::COLOR_RGBA2BGR);
    // 保存到 PNG 文件

    cv::imwrite("/sdcard/screenshot.png", bgrImage);
} */

void sendEvent(int fd_, __u16 type, __u16 code, __s32 value)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    write(fd_, &event, sizeof(event));
}
void highFrequencyClick(int fd, int x, int y, int count, int interval)
{
    // 物理触摸映射到像素坐标
    // x = x * (18000 / 1800);
    // y = y * (28800 / 2880);
    x = x * 10;
    y = y * 10;
    while (count >= 0)
    {
        // getevent -p /dev/input/event6
        sendEvent(fd, EV_ABS, ABS_MT_TRACKING_ID, 0xff);
        // 2. 设置触摸位置
        sendEvent(fd, EV_ABS, ABS_MT_POSITION_X, x);
        sendEvent(fd, EV_ABS, ABS_MT_POSITION_Y, y);
        // 3. 设置触摸面积（可选) max: 1 => 255,5 => 0.02
        sendEvent(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 10);
        // 4. 设置压力值 max: 1 => 1000,50 => 0.05
        sendEvent(fd, EV_ABS, ABS_MT_PRESSURE, 50);
        // 5. 按下事件
        sendEvent(fd, EV_KEY, BTN_TOUCH, 1);
        // 6. 同步事件
        sendEvent(fd, EV_SYN, SYN_REPORT, 0);
        // 7. 释放压力
        sendEvent(fd, EV_ABS, ABS_MT_PRESSURE, 0);
        // 8. 释放触摸 ID
        sendEvent(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
        // 9. 抬起事件
        sendEvent(fd, EV_KEY, BTN_TOUCH, 0);
        // 10. 同步事件
        sendEvent(fd, EV_SYN, SYN_REPORT, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        count--;
    }
}
#include <xf86drm.h>
#include <xf86drmMode.h>
int main()
{
    int fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0)
    {
        perror("无法打开 DRM 设备");
        return -1;
    }

    drmModeRes *res = drmModeGetResources(fd);
    if (!res)
    {
        perror("无法获取 DRM 资源");
        close(fd);
        return -1;
    }
    printf("发现 %d 个连接器\n", res->count_connectors);

    drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[1]);
    if (conn != nullptr)
    {
        printf("连接器 %d: %dx%d\n", 1, conn->modes[0].hdisplay, conn->modes[0].vdisplay);
        // drmModeFreeConnector(conn);
    }
    drmModeModeInfo mode = conn->modes[0];

    printf("分辨率: %dx%d\n", mode.hdisplay, mode.vdisplay);
    // 获取 Encoder & CRTC
    drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoder_id);
    drmModeCrtc *crtc = drmModeGetCrtc(fd, enc->crtc_id);

    drmModePlaneRes *planeRes = drmModeGetPlaneResources(fd);
    if (!planeRes)
    {
        perror("无法获取 Plane 资源");
        return -1;
    }

    drmModePlane *plane = NULL;
    for (uint32_t i = 0; i < planeRes->count_planes; i++)
    {
        plane = drmModeGetPlane(fd, planeRes->planes[i]);
        if (plane && (plane->crtc_id == crtc->crtc_id))
        {
            printf("找到 Primary Plane ID: %d\n", plane->plane_id);
            break;
        }
        drmModeFreePlane(plane);
        plane = NULL;
    }

    // 获取当前 Framebuffer ID
    uint32_t fb_id = plane->fb_id;
    drmModeFB *fb = drmModeGetFB(fd, fb_id);
    if (!fb)
    {
        perror("无法获取 Framebuffer");
        return -1;
    }

    uint32_t width = fb->width;
    uint32_t height = fb->height;
    uint32_t stride = fb->pitch;
    uint32_t size = stride * height;
    printf("Framebuffer ID: %d, 宽: %d, 高: %d, Stride: %d, 大小: %d 字节\n",
           fb_id, width, height, stride, size);

    // 映射 Framebuffer

    int prime_fd;
    if (drmPrimeHandleToFD(fd, fb->handle, 0, &prime_fd))
    {
        perror("drmPrimeHandleToFD 失败");
        return -1;
    };

    void *map = mmap(0, size, PROT_READ, MAP_SHARED, prime_fd, 0);
    if (map == MAP_FAILED)
    {
        perror("mmap 失败");
        return -1;
    }

    FILE *fp = fopen("screenshot.ppm", "wb");
    if (!fp)
    {
        perror("无法创建截图文件");
        return -1;
    }

    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (uint32_t y = 0; y < height; y++)
    {
        fwrite((char *)map + y * stride, 1, width * 3, fp); // 仅存 RGB
    }
    fclose(fp);

    printf("截图保存到 %s\n", "screenshot.ppm");

    munmap(map, size);
    close(prime_fd);
    drmModeFreeFB(fb);
    drmModeFreePlane(plane);
    drmModeFreePlaneResources(planeRes);
    drmModeFreeCrtc(crtc);
    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(fd);
    return 0;
    /*
     EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
     if (display == EGL_NO_DISPLAY) {
         std::cerr << "无法获取 EGL Display!" << std::endl;
         return -1;
     }

     // 2️⃣ 初始化 EGL
     if (!eglInitialize(display, NULL, NULL)) {
         std::cerr << "EGL 初始化失败!" << std::endl;
         return -1;
     }

     // 3️⃣ 选择合适的 EGL 配置
     EGLConfig config;
     EGLint numConfigs;
     EGLint configAttribs[] = {
         EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
         EGL_BLUE_SIZE, 8,
         EGL_GREEN_SIZE, 8,
         EGL_RED_SIZE, 8,
         EGL_NONE
     };

     eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);

     // 4️⃣ 创建 EGL 上下文
     EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
     if (context == EGL_NO_CONTEXT) {
         std::cerr << "EGL Context 创建失败!" << std::endl;
         return -1;
     }

     // 5️⃣ 绑定 EGL 上下文（没有 EGLSurface，这里只是测试）
     if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
         std::cerr << "EGL 绑定失败!" << std::endl;
         return -1;
     }

     // 6️⃣ 执行 OpenGL 渲染
     glClearColor(1.0f, 0.0f, 0.0f, 1.0f);  // 设置红色背景
     glClear(GL_COLOR_BUFFER_BIT);          // 清空缓冲区

     std::cout << "OpenGL 渲染完成，屏幕应该变成红色！" << std::endl;

     // 7️⃣ 清理资源
     eglDestroyContext(display, context);
     eglTerminate(display);
     return 0; */
}
// https://raw.githubusercontent.com/aosp-mirror/platform_frameworks_base/master/
