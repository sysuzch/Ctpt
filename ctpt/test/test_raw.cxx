

#include <gtest/gtest.h>

#include <iostream>
#include <complex>

#include <vector>

#include "raw_wave.hpp"
#include "wave-signal.h"

class ctptwave : public testing::Test
{
protected:
    raw_wave rw;
    struct wave_signal sig;

    virtual void SetUp()
        {
            wave_signal_init( &sig );
            wave_signal_set( &sig, 1.0, 25600.0, 5120 );
            wave_signal_add( &sig, 300,  0, 50, 5);
            wave_signal_add( &sig, 400,  0, 150, 30);
        }

    virtual void TearDown()
        {
            wave_signal_destroy( &sig );
        }
};

TEST_F(ctptwave, wave_signalrms)
{
    EXPECT_FLOAT_EQ( 500.0, wave_signal_get_rms(&sig) );
    rw.set_raw_data( sig.p, sig.npoint, sig.fs );
    rw.set_freq( 50.0 );
    EXPECT_DOUBLE_EQ( sig.fs, rw.get_fs());
    EXPECT_EQ( sig.npoint, rw.get_np());

    rw.calculate_all();
    EXPECT_FLOAT_EQ( wave_signal_get_rms(&sig), rw.get_rms() );

    // std::cout<< "base rms:" << rw.get_base_rms() << std::endl;
    // std::cout<< "@50 rms:" << rw.get_rms( 50.0 ) << std::endl;
    // std::cout<< "@150 rms:" << rw.get_rms( 150.0 ) << std::endl;
    EXPECT_FLOAT_EQ( 300.0, rw.get_base_rms() );
    EXPECT_FLOAT_EQ( 400.0, rw.get_rms(150.0) );

    EXPECT_FLOAT_EQ( 400.0/300.0, rw.get_thd() );

    wave_signal_add( &sig, 250,  23, 250, -30);
    wave_signal_add( &sig, 305,  0, 300, -100);
    wave_signal_add( &sig, 350,  0, 350, 120);
    rw.set_raw_data( sig.p, sig.npoint, sig.fs );
    rw.calculate_all();

    // std::cout<< "***********************************************" << std::endl;
    // std::cout<< "@50 rms:" << rw.get_rms( 50.0 ) << std::endl;
    // std::cout<< "@150 rms:" << rw.get_rms( 150.0 ) << std::endl;
    // std::cout<< "@250 rms:" << rw.get_rms( 250.0 ) << std::endl;
    // std::cout<< "@300 rms:" << rw.get_rms( 300.0 ) << std::endl;
    // std::cout<< "@350 rms:" << rw.get_rms( 350.0 ) << std::endl;
    EXPECT_FLOAT_EQ( 300.0, rw.get_base_rms() );
    EXPECT_FLOAT_EQ( 23.0,  rw.get_rms(0.0) );
    EXPECT_FLOAT_EQ( 400.0, rw.get_rms(150.0) );
    EXPECT_FLOAT_EQ( 250.0, rw.get_rms(250.0) );
    EXPECT_FLOAT_EQ( 305.0, rw.get_rms(300.0) );
    EXPECT_FLOAT_EQ( 350.0, rw.get_rms(350.0) );

    // std::cout<< "@50 angle:" <<  rw.get_angle( 50.0 ) /3.1415926*180.0 << std::endl;
    // std::cout<< "@150 angle:" << rw.get_angle( 150.0 )/3.1415926*180.0 << std::endl;
    // std::cout<< "@250 angle:" << rw.get_angle( 250.0 )/3.1415926*180.0 << std::endl;
    // std::cout<< "@300 angle:" << rw.get_angle( 300.0 )/3.1415926*180.0 << std::endl;
    // std::cout<< "@350 angle:" << rw.get_angle( 350.0 )/3.1415926*180.0 << std::endl;

    EXPECT_FLOAT_EQ( 5.0,    rw.get_angle(50.0)/3.1415926*180.0 );
    EXPECT_FLOAT_EQ( 30.0,   rw.get_angle(150.0)/3.1415926*180.0 );
    EXPECT_FLOAT_EQ( -30.0,  rw.get_angle(250.0)/3.1415926*180.0 );
    EXPECT_FLOAT_EQ( -100.0, rw.get_angle(300.0)/3.1415926*180.0 );
    EXPECT_FLOAT_EQ( 120.0,  rw.get_angle(350.0)/3.1415926*180.0 );
}
