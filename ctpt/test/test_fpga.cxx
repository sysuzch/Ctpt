

#include "raw_wave.hpp"
#include "test.hpp"

TEST_F(ctptctlr, fpga_read)
{
    raw_wave ch1;
    raw_wave ch2;
    raw_wave ch3;
    raw_wave ch4;
    double scale[4] = {
        20.0*67.0/65536.0,
        20.0*67.0/65536.0,
        20.0*67.0/65536.0,
        20.0*67.0/65536.0,
    };
#if 0
    wave_signal_set( scale[0], 25600, 5120 );
    fpga.set_scale( scale );

    wave_signal_add( &sig.u_a, 230, 0, 50,  0   );
    wave_signal_add( &sig.u_a, 16,  3, 150, 30   );
    wave_signal_add( &sig.u_b, 231, 0, 50, 240 );
    wave_signal_add( &sig.u_c, 232, 0, 50, 120 );
    wave_signal_add( &sig.u_n, 233, 0, 50, 0 );

    fpga.dc_dc_v_set_percent( 100 );
    fpga.dc_ac_v_set( 10 );
    fpga.dc_ac_f_set( 50.0 );
    fpga.dc_ac_p_set( 0 );
    fpga.set_range( 0, 0, 0, 0 );
    fpga.set_np_per_cycle( 512 );

    fpga.dc_dc_start();
    fpga.dc_ac_enable();
    fpga.dc_ac_start();

    int ret = fpga.read( &ch1, &ch2, &ch3, &ch4 );

    ASSERT_EQ(0, ret);
    EXPECT_EQ( 5120, ch1.get_np() );
    EXPECT_EQ( 5120, ch2.get_np() );
    EXPECT_EQ( 5120, ch3.get_np() );
    EXPECT_EQ( 5120, ch4.get_np() );

    // calculate
    ch1.calculate_all();
    ch2.calculate_all();
    ch3.calculate_all();
    ch4.calculate_all();

    EXPECT_FLOAT_EQ( 3.0, ch1.get_rms(0.0) );
    EXPECT_FLOAT_EQ( 230.0, ch1.get_base_rms() );
    EXPECT_FLOAT_EQ( 230.0, ch1.get_rms(50.0) );
    EXPECT_FLOAT_EQ( 16.0, ch1.get_rms(150.0) );
    EXPECT_FLOAT_EQ( 30.0, ch1.get_angle(150.0)/M_PI*180.0 );
#else
    std::cout << "test_fpga.cxx error: not object" << std::endl;
#endif
    std::cout << "freq: " << ch1.get_freq() << std::endl;
    std::cout << "rms0: " << ch1.get_rms() << std::endl;
    std::cout << "rms4: " << ch4.get_rms() << std::endl;
    std::cout << "rms1: " << wave_signal_get_rms( &sig.u_a ) << std::endl;
    std::cout << "peek: " << ch4.get_peek() << std::endl;
}



