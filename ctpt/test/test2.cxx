

#include "test.hpp"


TEST_F(ctptctlr, wave_test)
{
    wave_signal_add( &sig.u_a, 230, 0, 50, 0   );
    wave_signal_add( &sig.u_b, 231, 0, 50, 240 );
    wave_signal_add( &sig.u_c, 232, 0, 50, 120 );

    system_wave_signal_write_csv( &sig, "abc.csv" );
}



