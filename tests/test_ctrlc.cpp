#include "commonlibs/ctrlc.hpp"
#include <gtest/gtest.h>
#include <csignal>

// Tests run sequentially within a suite.  Each test calls init() to
// re-register the handler and reset the flag, so order does not matter.

TEST(CtrlCHandler, IsSingleton)
{
    auto &a = commonlibs::CTRL_C_handler::instance();
    auto &b = commonlibs::CTRL_C_handler::instance();
    EXPECT_EQ(&a, &b);
}

TEST(CtrlCHandler, InitSetsInterruptFalse)
{
    auto &handler = commonlibs::CTRL_C_handler::instance();
    handler.init();
    EXPECT_FALSE(handler.get_interrupt());
}

TEST(CtrlCHandler, SetStatusSetsInterruptTrue)
{
    auto &handler = commonlibs::CTRL_C_handler::instance();
    handler.init();
    EXPECT_FALSE(handler.get_interrupt());

    handler.set_status();
    EXPECT_TRUE(handler.get_interrupt());
}

TEST(CtrlCHandler, InitResetsAfterSetStatus)
{
    auto &handler = commonlibs::CTRL_C_handler::instance();
    handler.set_status();
    EXPECT_TRUE(handler.get_interrupt());

    handler.init();
    EXPECT_FALSE(handler.get_interrupt());
}

TEST(CtrlCHandler, RaiseSIGINT_TriggersHandler)
{
    auto &handler = commonlibs::CTRL_C_handler::instance();
    handler.init();
    EXPECT_FALSE(handler.get_interrupt());

    raise(SIGINT);

    EXPECT_TRUE(handler.get_interrupt());
}
