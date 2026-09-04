#include <gtest/gtest.h>
#include <testpulse/testpulse.hpp>

TEST(TaggedFixture, LoginSucceeds) { testpulse::Case("LOGIN-42"); }

TEST(TaggedFixture, UntaggedTest) {}
