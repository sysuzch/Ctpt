
#include <assert.h>
#include <stdio.h>
#include "exciting_scanner.hpp"
#include "data_source.hpp"

int exciting_scanner::find_near_voltage( double voltage )
{
    int n;

    n = this->exciting.size();
    for ( int i=0; i<n; i++ )
    {
        double vol;
        vol = this->exciting[i].messured_vol.get_rms();
        if ( vol >= voltage )
        {
            return i;
            break;
        }
    }

    return -1;
}

bool exciting_scanner::find_knee_point( int *knee_index )
{
    int i = 0;
    int n;

    assert( knee_index );
    *knee_index = -1;
    std::vector<struct exciting_ui> &b = this->exciting;
    n = b.size();
    double vol = b[i].messured_vol.get_rms();
    double cur = b[i].messured_cur.get_rms();

    for (i=1 ; i<n; i++)
    {
        double inc_10_pencent;
        int index;
        inc_10_pencent = 1.1 * vol;
        index = find_near_voltage( inc_10_pencent );
        if ( index >= 0 )
        {
            double cur_large = b[i].messured_cur.get_rms();
            double ratio = cur_large / cur;
            if ( ratio >= 1.5 )
            {
                // found.
                *knee_index = index;
                return true;
            }
        }
        vol = b[i].messured_vol.get_rms();
        cur = b[i].messured_cur.get_rms();
    }

    return false;
}

bool exciting_scanner::exciting_scan( double fs, int np, double preset_voltage, double freq_start, double freq_end, double freq_step )
{
    data_source *ds = this->ds;

    if ( !ds->set_range( preset_voltage ) )
    {
        return false;
    }

    if ( !ds->set_sample_parameter( fs, np ) )
    {
        return false;
    }

    for (double freq=freq_start; freq<=freq_end; freq+=freq_step)
    {
        struct exciting_ui eui;
        eui.preset_vol = preset_voltage;
        eui.preset_cur = 0.0;
        eui.preset_freq = freq;

        ds->set_preset_voltage( preset_voltage, freq );

        this->exciting.push_back( eui );
        struct exciting_ui &b = this->exciting.back();

        // b.in_vol.set_dynamic( np, fs,  );
        std::vector<double> &in_vol  = b.messured_vol.get_writable_buffer();
        std::vector<double> &in_cur  = b.messured_cur.get_writable_buffer();

        assert( (int)in_vol.size()  == np );
        assert( (int)in_cur.size()  == np );
        if ( this->exciting.size() > 1 )
        {
            calculate_exciting_item( this->exciting[this->exciting.size()-1] );
        }

        if ( !ds->sync() || !ds->get_adc_value( &in_vol[0], &in_cur[0], NULL, NULL, &b.messured_freq, np ) )
        {
            this->exciting.resize(0);
            return false;
        }
    }

    if ( this->exciting.size() )
    {
        calculate_exciting_item( this->exciting[this->exciting.size()-1] );
        find_knee_point( &this->exciting_knee_point_index );
    }

    return true;
}

void exciting_scanner::calculate_exciting_item( struct exciting_ui &item )
{
    item.messured_vol.calculate_all();
    item.messured_cur.calculate_all();

    // angle
    double angle = item.messured_vol.get_base_angle() - item.messured_cur.get_base_angle();
    item.angle = angle;
}


