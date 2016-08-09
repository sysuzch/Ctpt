
#include "ctpt_api.hpp"
#include "data_source.hpp"


data_source::data_source()
{

}

data_source::~data_source()
{

}

bool data_source::auto_scan_prepare(const ct_setting &s )
{
    return false;
}

bool data_source::auto_scan_prepare(const pt_setting &s )
{
    return false;
}

bool data_source::set_preset_voltage( double voltage, double freq )
{
    return true;
}

bool data_source::set_preset_current( double current, double freq )
{
    return true;
}

bool data_source::set_sample_parameter( double sample_rate, int np )
{
    return true;
}

bool data_source::set_range( double need )
{
    return true;
}

bool data_source::auto_scan_reset( void )
{
    return true;
}
bool data_source::auto_scan_next( void )
{
    return false;
}

bool data_source::sync( void )
{
    return true;
}

bool data_source::get_adc_value( double *in_vol, double *in_cur, double *out_vol, double *out_cur, double *freq, int np )
{
    return true;
}

bool data_source::auto_scan_step( struct auto_scan_result &result )
{
    return true;
}

