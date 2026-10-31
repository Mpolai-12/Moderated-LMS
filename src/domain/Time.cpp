#include "domain/Time.hpp"

#include <cstdio>

#include "domain/Errors.hpp"

namespace lms {
namespace {

// Howard Hinnant's civil-date algorithms: portable, no timegm()/_mkgmtime().
long daysFromCivil(int y, int m, int d) {
    y -= (m <= 2) ? 1 : 0;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yoe + era * 400 + (m <= 2 ? 1 : 0));
}

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int y, int m) {
    static const int table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : table[m - 1];
}

}  // namespace

std::string toIso(TimePoint t) {
    using namespace std::chrono;
    long secs = static_cast<long>(duration_cast<seconds>(t.time_since_epoch()).count());
    long days = secs / 86400;
    long rem = secs % 86400;
    if (rem < 0) { rem += 86400; --days; }
    int y = 0, m = 0, d = 0;
    civilFromDays(days, y, m, d);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", y, m, d,
                  static_cast<int>(rem / 3600), static_cast<int>((rem % 3600) / 60),
                  static_cast<int>(rem % 60));
    return buf;
}

TimePoint fromIso(const std::string& text) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    char tail = 0;
    if (text.size() != 20 ||
        std::sscanf(text.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%c", &y, &mo, &d, &h, &mi, &s, &tail) != 7 ||
        tail != 'Z' || mo < 1 || mo > 12 || d < 1 || d > daysInMonth(y, mo) || h > 23 || mi > 59 || s > 59) {
        throw ValidationError("invalid date/time (expected YYYY-MM-DDTHH:MM:SSZ): " + text);
    }
    const long days = daysFromCivil(y, mo, d);
    const long secs = days * 86400 + h * 3600L + mi * 60L + s;
    return TimePoint(std::chrono::seconds(secs));
}

int daysLate(TimePoint submittedAt, TimePoint effectiveDeadline) {
    if (submittedAt <= effectiveDeadline) return 0;
    using Day = std::chrono::duration<long, std::ratio<86400>>;
    return static_cast<int>(std::chrono::ceil<Day>(submittedAt - effectiveDeadline).count());
}

}  // namespace lms
