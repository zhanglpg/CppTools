// <string> must be included before errorstatus.hpp because that header uses
// std::string without including <string> itself.
#include <string>
#include "commonlibs/errorstatus.hpp"
#include "commonlibs/common_errorcode.hpp"
#include <gtest/gtest.h>

using namespace commonlibs;

// ---- simple_saver ----------------------------------------------------------

TEST(SimpleSaver, SetAndGetErrorCode)
{
    simple_saver s;
    s.set_errstatus(42, "something went wrong");
    EXPECT_EQ(42u, s.get_errcode());
}

TEST(SimpleSaver, SetAndGetErrorMessage)
{
    simple_saver s;
    s.set_errstatus(1, "test message");
    EXPECT_EQ("test message", s.get_errmsg());
}

TEST(SimpleSaver, OverwritePreviousStatus)
{
    simple_saver s;
    s.set_errstatus(1, "first error");
    s.set_errstatus(99, "second error");
    EXPECT_EQ(99u,            s.get_errcode());
    EXPECT_EQ("second error", s.get_errmsg());
}

TEST(SimpleSaver, EmptyMessage)
{
    simple_saver s;
    s.set_errstatus(0, "");
    EXPECT_EQ(0u, s.get_errcode());
    EXPECT_EQ("", s.get_errmsg());
}

TEST(SimpleSaver, ZeroErrorCode)
{
    simple_saver s;
    s.set_errstatus(0, "no error");
    EXPECT_EQ(0u, s.get_errcode());
}

TEST(SimpleSaver, LongMessage)
{
    simple_saver s;
    std::string long_msg(1024, 'x');
    s.set_errstatus(7, long_msg);
    EXPECT_EQ(7u,       s.get_errcode());
    EXPECT_EQ(long_msg, s.get_errmsg());
}

TEST(SimpleSaver, KnownErrorCode_StatusOk)
{
    simple_saver s;
    s.set_errstatus(STATUS_OK, "ok");
    EXPECT_EQ(static_cast<unsigned int>(STATUS_OK), s.get_errcode());
}

TEST(SimpleSaver, KnownErrorCode_CommError)
{
    simple_saver s;
    s.set_errstatus(STATUS_COMM_ERROR, "comm failure");
    EXPECT_EQ(static_cast<unsigned int>(STATUS_COMM_ERROR), s.get_errcode());
}

// ---- get_errmsg returns a reference ----------------------------------------

TEST(ErrorStatusSaver, GetErrMsgReturnsReference)
{
    simple_saver s;
    s.set_errstatus(5, "ref test");
    const std::string &ref = s.get_errmsg();
    EXPECT_EQ("ref test", ref);
}

TEST(ErrorStatusSaver, MultipleObjects_Independent)
{
    simple_saver a, b;
    a.set_errstatus(1, "error A");
    b.set_errstatus(2, "error B");
    EXPECT_EQ(1u,       a.get_errcode());
    EXPECT_EQ("error A", a.get_errmsg());
    EXPECT_EQ(2u,       b.get_errcode());
    EXPECT_EQ("error B", b.get_errmsg());
}

// ---- custom CRTP implementor -----------------------------------------------
// Verify that the CRTP dispatch actually calls the derived save_msg.

class tracking_saver : public errorStatus_Saver<tracking_saver>
{
public:
    int save_call_count = 0;
    unsigned int last_code = 0;
    std::string last_msg;

    void save_msg(unsigned int u_c, const std::string &s)
    {
        ++save_call_count;
        last_code = u_c;
        last_msg  = s;
        // Members live in a dependent base; use this-> for template name lookup.
        this->s_errmsg    = s;
        this->u_errorcode = u_c;
    }
};

TEST(ErrorStatusSaver, CrtpDispatchCallsDerivedSaveMsg)
{
    tracking_saver ts;
    ts.set_errstatus(10, "dispatch test");
    EXPECT_EQ(1,               ts.save_call_count);
    EXPECT_EQ(10u,             ts.last_code);
    EXPECT_EQ("dispatch test", ts.last_msg);
}

TEST(ErrorStatusSaver, CrtpDispatchCalledOnEachSet)
{
    tracking_saver ts;
    ts.set_errstatus(1, "first");
    ts.set_errstatus(2, "second");
    ts.set_errstatus(3, "third");
    EXPECT_EQ(3,  ts.save_call_count);
    EXPECT_EQ(3u, ts.get_errcode());
}
