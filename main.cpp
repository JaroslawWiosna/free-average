#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <optional>

const int times_per_sec = 2;
const int dt = 1000 / times_per_sec;

struct String_view;

// Ring buffer buf we never pop
template <typename T, int capacity>
struct Ring_buffer {
    T buf[capacity];
    int begin;
    bool is_full;

    void push(T x);
    String_view avg();
};

template <typename T, int capacity>
void Ring_buffer<T, capacity>::push(T x) {
    if (begin >= capacity) {
        is_full = true;
        begin = 0;
    }
    buf[begin++] = x;
}

struct String_view {
    size_t len;
    const char* str;

    String_view trim_begin(void) const
    {
        String_view result = *this;

        while (result.len != 0 && isspace(*result.str)) {
            result.str  += 1;
            result.len -= 1;
        }
        return result;
    }

    String_view trim_end(void) const
    {
        String_view result = *this;

        while (result.len != 0 && isspace(*(result.str + result.len - 1))) {
            result.len -= 1;
        }
        return result;
    }

    String_view trim(void) const
    {
        return trim_begin().trim_end();
    }

    void chop(size_t n)
    {
        if (n > len) {
            len = 0;
        } else {
            str += n;
            len -= n;
        }
    }

    String_view chop_by_delim(char delim)
    {
        size_t i{};
        while (i < len && str[i] != delim) {
            i++;
        }
        String_view result{i, str};
        chop(i + 1);

        return result;
    }

    String_view chop_word(void)
    {
        *this = trim_begin();

        size_t i{};
        while (i < len && !isspace(str[i])) i++;

        String_view result {i, str};

        len -= i;
        str += i;

        return result;
    }

    template <typename Int>
    std::optional<Int> chop_int() const
    {
        Int sign {1};
        Int number {};
        String_view view = *this;

        if (view.len == 0) {
            return {};
        }

        if (*view.str == '-') {
            sign = -1;
            view.chop(1);
        }

        while (view.len) {
            if (!isdigit(*view.str)) {
                return {};
            }
            number = number * 10 + (*view.str - '0');
            view.chop(1);
        }

        return {sign * number};
    }
};

String_view operator ""_sv (const char *str, size_t len) {
    String_view result;
    result.str = str;
    result.len = len;
    return result;
}

template <typename T, int capacity>
String_view Ring_buffer<T, capacity>::avg() {
    if (!is_full) {
        return ("--.--"_sv);
    }
    T acc{};
    for (int i{}; i < capacity; ++i) {
        acc += buf[i];
    }
    static char buf[10];
    sprintf(buf, "%2.2f", acc / capacity);
    String_view result{};
    result.str = buf;
    result.len = 5;
    return result;
}

std::optional<String_view> meminfo_to_sv() {
    String_view result{};

    size_t n = 0;
    FILE *f = fopen("/proc/meminfo", "rb");
    if (!f) {
        return {};
    }

    int code = fseek(f, 0, SEEK_END);
    if (code < 0) {
        return {};
    }

    result.len = (size_t) 140; // b'cuz it 3 lines fit in 140 char...

    code = fseek(f, 0, SEEK_SET);
    if (code < 0) {
        return {};
    }

    char *buffer = new char[result.len];
    if (!buffer) {
        return {};
    }

    n = fread(buffer, 1, result.len, f);
    if (n != result.len) {
        return {};
    }

    result.str = buffer;
    free(buffer);

    return result;
}

struct Memory {
    unsigned long total;
    unsigned long available;
};

std::optional<Memory> current_memory() {
    Memory result;
    auto meminfo = meminfo_to_sv();
    if (!meminfo.has_value()) {
        return {};
    }
    auto total = meminfo.value().chop_by_delim('\n');

    total.chop_word();
    total = total.trim();
    result.total = total.chop_word().chop_int<unsigned long>().value();

    meminfo.value().chop_by_delim('\n');
    auto available = meminfo.value().chop_by_delim('\n');

    available.chop_word();
    available = available.trim();
    result.available = available.chop_word().chop_int<unsigned long>().value();
    return result;
}

int main() {
    setbuf(stdout, nullptr);

    Ring_buffer<float, times_per_sec> one_second{};
    Ring_buffer<float, times_per_sec * 60> one_minute{};
    Ring_buffer<float, times_per_sec * 60 * 2> two_minutes{};
    while (1) {
        std::optional<Memory> memory = current_memory();
        if (!memory.has_value()) {
        printf("                                                    \r"); 
        printf("  :-(                                               \n"); 
            continue;
        }
        
        auto i = 100.0 * memory.value().available / memory.value().total;

        one_second.push(i);
        one_minute.push(i);
        two_minutes.push(i);

        printf("                                                    \r"); 
        printf(" free-average\t%.*s\t%.*s\t%.*s\r", 
                one_second.avg().len, one_second.avg().str, 
                one_minute.avg().len, one_minute.avg().str,
                two_minutes.avg().len, two_minutes.avg().str);
        std::this_thread::sleep_for(std::chrono::milliseconds(dt));
    }
}
