#ifndef PROGRESS_BAR
#define PROGRESS_BAR
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#undef max
#undef min
#else
#include <sys/ioctl.h>
#endif
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
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        this->width = std::max(csbi.srWindow.Right - csbi.srWindow.Left + 1, 1);
#else
        struct winsize size;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
        this->width = size.ws_col;
#endif
        this->length = std::max(width - int(hint.size()) - 8, 1);
        this->length = std::min(this->length, 100);
        this->temp = std::format("\r\033[1;36m{}: [{{:<{}}}]{{:3}}%\033[0m", hint, length);
        update(0);
    }
    void update(double now) {
        if (length > 1) {
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
    }
};
#endif