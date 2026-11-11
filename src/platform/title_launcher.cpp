#include "orbit/platform/title_launcher.hpp"

#include <cstdio>
#include <cstring>

#ifdef ORBIT_PS4
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#endif

namespace orbit::platform {
int TitleLauncher::launch(const char* title_id) noexcept {
    if (!title_id || std::strlen(title_id) < 9 || std::strlen(title_id) > 12) return -1;
    for (const char* p = title_id; *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) return -1;
#ifdef ORBIT_PS4
    int user_id = -1;
    const int user_result = sceUserServiceGetForegroundUser(&user_id);
    if (user_result != 0 || user_id < 0) { std::fprintf(stderr, "ORBIT: foreground user lookup failed (%d)\n", user_result); return user_result != 0 ? user_result : -1; }
    LncAppParam params{};
    params.size = static_cast<uint32_t>(sizeof(LncAppParam));
    params.user_id = static_cast<uint32_t>(user_id);
    params.app_opt = 0;
    params.crash_report = 0;
    params.LaunchAppCheck_flag = LaunchApp_None;
    const int result = sceLncUtilLaunchApp(title_id, nullptr, &params);
    if (result < 0) std::fprintf(stderr, "ORBIT: title launch request failed for %s (%d)\n", title_id, result);
    return result;
#else
    (void)title_id;
    return -2; // Host builds deliberately do not emulate a PS4 launch service.
#endif
}
}
