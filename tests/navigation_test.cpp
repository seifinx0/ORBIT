#include "orbit/input/controller.hpp"

#include <cassert>

int main() {
    using namespace orbit::input;
    NavigationInput input;
    PadSample sample{};

    sample.buttons = Down;
    assert(input.update(sample, 16) == Down);
    assert(input.update(sample, 16) == 0);
    for (int i = 0; i < 21; ++i) assert(input.update(sample, 16) == 0);
    assert(input.update(sample, 16) == Down);
    for (int i = 0; i < 5; ++i) assert(input.update(sample, 16) == 0);
    assert(input.update(sample, 16) == Down);

    sample.buttons = 0;
    assert(input.update(sample, 16) == 0);
    sample.stick_x = 220;
    assert(input.update(sample, 16) == Right);
    sample.buttons = Confirm | Context | TabNext;
    assert(input.update(sample, 16) == (Confirm | Context | TabNext));
    assert(input.update(sample, 16) == 0);

    sample.buttons = 0;
    sample.stick_x = 128;
    input.update(sample, 16);
    sample.buttons = Up | Left;
    assert(input.update(sample, 16) == Up);
    sample.buttons = Back;
    assert(input.update(sample, 16) == Back);
}
