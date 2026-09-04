#include <gtest/gtest.h>
#include <testpulse/testpulse.hpp>

TEST(MultiAttachFixture, AttachesTwice) {
    testpulse::Case("LOGIN-45");
    testpulse::Attach("LOGIN-45", {0x01}, "a.png", "image/png");
    testpulse::Attach("LOGIN-45", {0x02}, "b.png", "image/png");
}
