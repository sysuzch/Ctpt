
#include <assert.h>
#include "direct_scanner.hpp"
#include "data_source.hpp"



void direct_scanner::calculate_direct_item( struct direct_ui  &item )
{
    item.in_vol.calculate_all();
    item.in_cur.calculate_all();

    item.out_vol.calculate_all();
    item.out_cur.calculate_all();

    // angle
    item.in_voltage_current_angle  = item.in_vol.get_base_angle() - item.in_cur.get_base_angle();
    item.out_voltage_current_angle = item.out_vol.get_base_angle() - item.out_cur.get_base_angle();
    item.io_voltage_angle = item.out_vol.get_base_angle() - item.in_vol.get_base_angle();
    item.io_current_angle = item.out_cur.get_base_angle() - item.in_cur.get_base_angle();
}


bool direct_scanner::direct_scan_fix_voltage( double fs, int np, double fix_voltage, double freq_start, double freq_end, double freq_step )
{
    data_source *ds = this->ds;
    std::vector<direct_ui> &d = this->directing;

    if ( !ds->set_range( fix_voltage ) )
    {
        return false;
    }

    if ( !ds->set_sample_parameter( fs, np ) )
    {
        return false;
    }


    for (double freq = freq_start ; freq<freq_end; freq += freq_step)
    {
        struct direct_ui dui;
        ds->set_preset_voltage( fix_voltage, freq );
        d.push_back( dui );
        direct_ui &bk = d.back();
        std::vector<double> &in_vol = bk.in_vol.get_writable_buffer();
        std::vector<double> &in_cur = bk.in_cur.get_writable_buffer();
        std::vector<double> &out_vol = bk.out_vol.get_writable_buffer();
        std::vector<double> &out_cur = bk.out_cur.get_writable_buffer();

        bk.preset_vol = fix_voltage;
        bk.preset_cur = 0.0;
        bk.preset_freq = freq;

        if ( d.size() > 1 )
        {
            calculate_direct_item( d[ d.size() - 1 ] );
        }

        if ( !ds->sync() || !ds->get_adc_value( &in_vol[0], &in_cur[0], &out_vol[0], &out_cur[0], &bk.in_freq, np ))
        {
            d.resize( 0 );
            return false;
        }
    }

    if ( d.size() )
    {
        calculate_direct_item( d[ d.size() - 1 ] );
    }

    return true;
}

bool direct_scanner::direct_scan_fix_freq( double fs, int np, double fix_freq, double vol_start, double vol_end, double vol_step )
{
    data_source *ds = this->ds;
    std::vector<direct_ui> &d = this->directing;

    if ( !ds->set_sample_parameter( fs, np ) )
    {
        return false;
    }

    for (double vol = vol_start ; vol<vol_end; vol += vol_step)
    {
        struct direct_ui dui;

        if ( !ds->set_range( vol ) )
        {
            return false;
        }

        if ( !ds->set_preset_voltage( vol, fix_freq ) )
        {
            return false;
        }
        d.push_back( dui );
        direct_ui &bk = d.back();
        std::vector<double> &in_vol  = bk.in_vol.get_writable_buffer();
        std::vector<double> &in_cur  = bk.in_cur.get_writable_buffer();
        std::vector<double> &out_vol = bk.out_vol.get_writable_buffer();
        std::vector<double> &out_cur = bk.out_cur.get_writable_buffer();

        bk.preset_vol = vol;
        bk.preset_cur = 0.0;
        bk.preset_freq = fix_freq;


        if ( d.size() > 1 )
        {
            calculate_direct_item( d[ d.size() - 1 ] );
        }

        if ( !ds->sync() || !ds->get_adc_value( &in_vol[0], &in_cur[0], &out_vol[0], &out_cur[0], &bk.in_freq, np ))
        {
            d.resize( 0 );
            return false;
        }
    }

    if ( d.size() )
    {
        calculate_direct_item( d[ d.size() - 1 ] );
    }

    return true;
}

