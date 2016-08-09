#ifndef TEST_H
#define TEST_H


#include <stdio.h>
#include <iostream>
#include <gtest/gtest.h>

#include <iostream>
#include <complex>

#include <vector>

#include "ctpt_api.hpp"
#include "wave-signal.h"
#include "fpga.hpp"

void set_global_signal( struct system_wave_signal *sig );

class ctptctlr : public testing::Test
{
protected:
    ctpt_ctrl c;
    struct ct_setting cs;
    struct system_wave_signal sig;
    fpga_dev fpga;

    virtual void SetUp()
        {
            c.init();
            c.reinit();
            memset( &cs, 0x00, sizeof(cs) );
            c.config( cs );
            c.start();
            c.stop();
            c.get_user_test_result( 0, 0, 0);

            system_wave_signal_init( &sig );
            system_wave_signal_uset( &sig, 1.0, 25600, 5120 );
            system_wave_signal_iset( &sig, 1.0, 25600, 5120 );

            // fpga init
            fpga.init( 0, 4 );

            set_global_signal( &sig );
        }

    void wave_signal_set( double scale, double sample_rate, int npoint )
        {
            system_wave_signal_uset( &sig, scale, sample_rate, npoint );
        }

    virtual void TearDown()
        {
            system_wave_signal_destroy( &sig );
        }
};



#endif /* TEST_H */

