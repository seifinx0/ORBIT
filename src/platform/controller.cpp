#include "orbit/input/controller.hpp"

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include <cstdio>

namespace orbit::input {

ControllerDevice::~ControllerDevice() { shutdown(); }

bool ControllerDevice::initialize() noexcept {
    shutdown();
    const int pad_init = scePadInit();
    if (pad_init != 0) {
        std::fprintf(stderr, "ORBIT: scePadInit failed (%d)\n", pad_init);
        return false;
    }
    (void)sceUserServiceInitialize(nullptr);
    int user_id = -1;
    const int user_result = sceUserServiceGetInitialUser(&user_id);
    if (user_result < 0 || user_id < 0) {
        std::fprintf(stderr, "ORBIT: no initial user for controller input (%d)\n", user_result);
        return false;
    }
    handle_ = scePadOpen(user_id, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    if (handle_ < 0) {
        std::fprintf(stderr, "ORBIT: scePadOpen failed (%d)\n", handle_);
        return false;
    }
    return true;
}

bool ControllerDevice::read(PadSample& sample) noexcept {
    if (handle_ < 0) return false;
    OrbisPadData data{};
    if (scePadReadState(handle_, &data) < 0) return false;
    sample = {};
    if (data.buttons & ORBIS_PAD_BUTTON_UP) sample.buttons |= Up;
    if (data.buttons & ORBIS_PAD_BUTTON_DOWN) sample.buttons |= Down;
    if (data.buttons & ORBIS_PAD_BUTTON_LEFT) sample.buttons |= Left;
    if (data.buttons & ORBIS_PAD_BUTTON_RIGHT) sample.buttons |= Right;
    if (data.buttons & ORBIS_PAD_BUTTON_CROSS) sample.buttons |= Confirm;
    if (data.buttons & ORBIS_PAD_BUTTON_CIRCLE) sample.buttons |= Back;
    if (data.buttons & ORBIS_PAD_BUTTON_L1) sample.buttons |= TabPrevious;
    if (data.buttons & ORBIS_PAD_BUTTON_R1) sample.buttons |= TabNext;
    if (data.buttons & ORBIS_PAD_BUTTON_OPTIONS) sample.buttons |= Context;
    sample.stick_x = data.leftStick.x;
    sample.stick_y = data.leftStick.y;
    return true;
}

void ControllerDevice::shutdown() noexcept {
    if (handle_ >= 0) {
        scePadClose(handle_);
        handle_ = -1;
    }
}

} // namespace orbit::input
