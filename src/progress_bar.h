#ifndef PROGRESS_BAR
#define PROGRESS_BAR
#include <sys/ioctl.h>
#include <format>
#include <cstdio>
class progress_bar {
private:
    std::string temp;
    int length;
    int width;
    double progress{ -1.0 };
public:
    progress_bar() = default;
    progress_bar(const std::string& hint) {
        struct winsize size;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
        this->width = size.ws_col;
        this->length = std::max(width - ssize(hint) - 8, 1z);
        this->temp = std::format("\r\033[1;36m{}: [{{:<{}}}]{{:3}}%\033[0m", hint, length);
        update(0);
    }
    void update(double now) {
        if (now - progress > 0.005) {
            progress = now;
            auto str = std::string(int(length * progress), '#');
            auto args = std::make_format_args(str, int(progress * 100));
            std::cerr << std::vformat(temp, args);
            std::cerr.flush();
        }
        if (now > 0.9999) {
            std::cerr << "\r" << std::string(width, ' ') << "\r";
            std::cerr.flush();
        }
    }
};
#endif