#include <cstdio>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <optional>

const int times_per_sec = 2;
const int dt = 1000 / times_per_sec;

// Ring buffer buf we never pop
template <typename T, int capacity>
struct Ring_buffer {
    T buf[capacity];
    int begin;
    bool is_full;

    void push(T x);
    std::optional<T> avg();
};

template <typename T, int capacity>
void Ring_buffer<T, capacity>::push(T x) {
    if (begin >= capacity) {
        is_full = true;
        begin = 0;
    }
    buf[begin++] = x;
}

template <typename T, int capacity>
std::optional<T> Ring_buffer<T, capacity>::avg() {
    if (!is_full) {
        return {};
    }
    T acc{};
    for (int i{}; i < capacity; ++i) {
        acc += buf[i];
    }
    return acc / capacity;
}

char buf[8192];
int fd;

int main() {
    float i{};
    setbuf(stdout, nullptr);

    Ring_buffer<float, times_per_sec> one_second{};
    Ring_buffer<float, times_per_sec * 60> one_minute{};
    Ring_buffer<float, times_per_sec * 60 * 2> two_minutes{};

    while (1) {
        fd = open("/proc/meminfo", O_RDONLY);        
        int n = read(fd, buf, sizeof (buf - 1));
        buf[n] = '\0';

        one_second.push(i);
        one_minute.push(i);
        two_minutes.push(i);

        i = (i < 15) ? i+1 : 0;
       
        printf("                                                    \r"); 
        printf(" free-average\t%3.2f\t%3.2f\t%3.2f\r", 
                one_second.avg().value_or(0.0),
                one_minute.avg().value_or(0.0),
                two_minutes.avg().value_or(0.0));
        std::this_thread::sleep_for(std::chrono::milliseconds(dt));
    }
}
