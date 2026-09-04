#include <gtest/gtest.h>
#include <testpulse/testpulse.hpp>

TEST(CrossTestFixture, DeclaresCaseKey) { testpulse::Case("LOGIN-42"); }

TEST(CrossTestFixture, AttemptsAttachUnderOtherTestsCaseKey) {
    try {
        testpulse::Attach("LOGIN-42", {0x01}, "failure.png", "image/png");
        ADD_FAILURE() << "expected Attach to throw for a case key declared by a different test";
    } catch (const testpulse::TestPulseError&) {
        SUCCEED();
    }
}
