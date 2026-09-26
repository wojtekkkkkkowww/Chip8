#include "MonoScreen.h"
#include <gtest/gtest.h>

namespace {

TEST(ScreenTest, ScreenInit) {
    MonoScreen screen;

    ASSERT_EQ(screen.get(0, 0), 0);
    //
    screen.set(3, 4);
    ASSERT_EQ(screen.get(3, 4), 1);

    screen.reset(3, 4);
    ASSERT_EQ(screen.get(3, 4), 0);
}

} // namespace
