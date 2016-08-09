
#include "test.hpp"


TEST_F(ctptctlr, t1)
{
    std::complex<double> a(1.732,  1.0);
    std::complex<double> b(1.0,  1.0);
    // std::cout<< "angle=" << std::arg(a)/M_PI*180.0 << std::endl;
    EXPECT_EQ( (int)(std::arg(a)/M_PI*180.0), 30 );
    EXPECT_EQ( (int)(std::arg(b)/M_PI*180.0), 45 );

    c.start();
}


class listentest: public rt_listener
{
public:
    listentest() : call_count(0){}
    virtual ~listentest(){}

    virtual void scanning( double voltage, double current )
        {
            call_count++;
        }
    int call_count;
};


TEST_F(ctptctlr, pointer)
{
    struct ctpt_result result;

    c.get_ctpt_result(result);
    EXPECT_TRUE( result.exciting_current );
    EXPECT_TRUE( result.exciting_voltage );
    EXPECT_TRUE( result.error5_resistance );
    EXPECT_TRUE( result.error5_factor );
    EXPECT_TRUE( result.error10_resistance );
    EXPECT_TRUE( result.error10_factor );
}

TEST_F(ctptctlr, listen)
{
    struct ct_setting cxx;
    ctpt_ctrl c2;
    listentest lt;

    c2.init();
    c2.register_real_time_listener( &lt );
    EXPECT_EQ( lt.call_count, 0 );
    c2.config( cxx );
    c2.start();
    EXPECT_NE( lt.call_count, 1 );
}


TEST_F(ctptctlr, learncpp)
{
    std::vector<int> v;

    v.push_back( 100 );
    int &last_one = v.back();
    EXPECT_EQ( v[0], 100 );
    EXPECT_EQ( last_one, 100 );
    last_one += 100;
    EXPECT_EQ( v[0], 200 );
}


