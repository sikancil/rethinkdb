#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdlib.h>
#include <stdint.h>

#include <string>

#include "config/args.hpp"
#include "errors.hpp"

/* Note that repli_timestamp_t does NOT represent an actual timestamp; instead it's an arbitrary
counter. */

// for safety
struct repli_timestamp_t {
    uint32_t time;

    bool operator==(repli_timestamp_t t) const {
        return time == t.time;
    }
    bool operator<(repli_timestamp_t t) const {
        return time < t.time;
    }
    bool operator>=(repli_timestamp_t t) const {
        return time >= t.time;
    }

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.time = time + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;
};

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *beg_, const char *end_) : beg(beg_), end(end_) { }
    const_charslice() : beg(NULL), end(NULL) { }
};

typedef uint64_t microtime_t;

microtime_t current_microtime();


// Like std::max, except it's technically not associative.
repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y);


void *malloc_aligned(size_t size, size_t alignment = 64);

template <class T1, class T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - (((value + alignment - 1) % alignment) + 1);
}

template <class T1, class T2>
T1 ceil_divide(T1 dividend, T2 alignment) {
    return (dividend + alignment - 1) / alignment;
}

template <class T1, class T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

template <class T1, class T2>
T1 ceil_modulo(T1 value, T2 alignment) {
    T1 x = (value + alignment - 1) % alignment;
    return value + alignment - ((x < 0 ? x + alignment : x) + 1);
}

typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs);
ticks_t get_ticks();
long get_ticks_res();
double ticks_to_secs(ticks_t ticks);

// HEY: Maybe debugf and log_call and TRACEPOINT should be placed in
// debug.hpp (and debug.cc).
/* Debugging printing API (prints current thread in addition to message) */
#ifndef NDEBUG
void debugf(const char *msg, ...) __attribute__((format (printf, 1, 2)));
#else
#define debugf(...) ((void)0)
#endif

// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
int randint(int n);

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins with a minus
// sign. strtoul_strict() and strtoull_strict() do.
long strtol_strict(const char *string, char **end, int base);
unsigned long strtoul_strict(const char *string, char **end, int base);
unsigned long long strtoull_strict(const char *string, char **end, int base);

// This is inefficient, it calls vsnprintf twice and copies the
// arglist and output buffer excessively.
std::string strprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

/* `demangle_cpp_name()` attempts to de-mangle the given symbol name. If it
succeeds, it returns the result as a `std::string`. If it fails, it throws
`demangle_failed_exc_t`. */
struct demangle_failed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};
std::string demangle_cpp_name(const char *mangled_name);

// Precise time (time+nanoseconds) for logging, etc.

struct precise_time_t : public tm {
    uint32_t ns;    // nanoseconds since the start of the second
                    // beware:
                    //   tm::tm_year is number of years since 1970,
                    //   tm::tm_mon is number of months since January,
                    //   tm::tm_sec is from 0 to 60 (to account for leap seconds)
                    // For more information see man gmtime(3)
};

void initialize_precise_time();     // should be called during startup
timespec get_uptime();              // returns relative time since initialize_precise_time(),
                                    // can return low precision time if clock_gettime call fails
precise_time_t get_absolute_time(const timespec& relative_time); // converts relative time to absolute
precise_time_t get_time_now();      // equivalent to get_absolute_time(get_uptime())

// formatted precise time:
// yyyy-mm-dd hh:mm:ss.MMMMMM   (26 characters)
const size_t formatted_precise_time_length = 26;    // not including null

void format_precise_time(const precise_time_t& time, char* buf, size_t max_chars);
std::string format_precise_time(const precise_time_t& time);

/* Printing binary data to stdout in a nice format */

void print_hd(const void *buf, size_t offset, size_t length);

// Fast string compare

int sized_strcmp(const char *str1, int len1, const char *str2, int len2);


/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_t` can be moved to
another thread; to do this, you can use the `rethread_t` type or the
`rethread()` method. */

#define INVALID_THREAD (-1)

class home_thread_mixin_t {
public:
    int home_thread() const { return real_home_thread; }

#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif  // NDEBUG

    void rethread(int thread);

    struct rethread_t {
        rethread_t(home_thread_mixin_t *m, int thread);
        ~rethread_t();
    private:
        home_thread_mixin_t *mixin;
        int old_thread, new_thread;
    };

protected:
    home_thread_mixin_t();
    virtual ~home_thread_mixin_t() { }

    int real_home_thread;

private:
    // Things with home threads should not be copyable, since we don't
    // want to nonchalantly copy their real_home_thread variable.
    DISABLE_COPYING(home_thread_mixin_t);
};

/* `on_thread_t` switches to the given thread in its constructor, then switches
back in its destructor. For example:

    printf("Suppose we are on thread 1.\n");
    {
        on_thread_t thread_switcher(2);
        printf("Now we are on thread 2.\n");
    }
    printf("And now we are on thread 1 again.\n");

*/

struct on_thread_t : public home_thread_mixin_t {
    on_thread_t(int thread);
    ~on_thread_t();
};

#endif // __UTILS_HPP__
