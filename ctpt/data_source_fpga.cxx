

#include <assert.h>
#include <unistd.h>
#include "data_source_fpga.hpp"

#define MODE_CT 0
#define MODE_PT 1

data_source_fpga::data_source_fpga()
{
    this->seq = 0;
    this->mode  = MODE_CT;
    this->listener = 0;
}

void data_source_fpga::set_listener( rt_listener *listener )
{
    this->listener = listener;
}

void data_source_fpga::init( void )
{
    double freq = 50;
    fpga.init(0, 4 );


    fpga.dc_ac_f_set( freq );
    fpga.set_np_per_cycle( 512 );
    fpga.select_range_by_value(0, 10);
    fpga.select_range_by_value(1, 1);
    fpga.select_range_by_value(2, 1);
    fpga.select_range_by_value(3, 1);

    fpga.stop_output();
    fpga.adc_stop_sample();

}

bool data_source_fpga::auto_scan_reset( void )
{
    this->seq = 0;

    fpga.stop_output();
    fpga.adc_stop_sample();

    fpga.start_output();
    fpga.safe_output(voltage[this->seq]);

    process(0);                 // 0%
    fpga.adc_start_sample();

    return true;
}

void data_source_fpga::process(int percent)
{
    if (this->listener){
        this->listener->process(percent);
    }
}

void data_source_fpga::scanning(double voltage, double current)
{
    if (this->listener){
        this->listener->scanning(voltage, current);
    }
}


bool data_source_fpga::auto_scan_prepare( const struct ct_setting &ct_s )
{
    ct_setting = ct_s;
    this->mode = MODE_CT;
    fpga.adc_start_sample();
    return true;
}

bool data_source_fpga::auto_scan_prepare( const struct pt_setting &pt_s )
{
    pt_setting = pt_s;
    this->mode = MODE_PT;
    return true;
}

struct scanning_stratergy
{
    double freq;
    double vol_start;
    double vol_step;
    double cur_limit;

    double range_i;
    double range_u1;
    double range_u2;
    double range_u3;
};

static struct scanning_stratergy g_stratergy[] = {
    {50, 0.1, 0.1,  1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {40,  0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {30,  0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {20,  0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {10,  0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {5,   0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {3,   0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {2,   0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A
    {1,   0.1, 0.1, 1, 0.1, },          // 50Hz, 0.1V, 0.1V, 1A

    // {46, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {45, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {44, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {43, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {42, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {41, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {40, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {39, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {38, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {37, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {36, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {35, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {34, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {33, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {32, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {31, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {30, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
    // {31, 0.1, 0.1, 1},          // 50Hz, 0.1V, 0.1V, 1A
};

const static int g_stratergy_count = sizeof(g_stratergy) / sizeof(g_stratergy[0]);

bool data_source_fpga::auto_scan_step( struct auto_scan_result &result )
{
    double voltage;
    double current;

    if (this->seq >= g_stratergy_count){
        return false;
    }
    struct scanning_stratergy *p = &g_stratergy[this->seq];

    result.wave_vol_primary.resize(NPOINT_SCAN);
    result.wave_cur_primary.resize(NPOINT_SCAN);
    result.wave_vol.resize(NPOINT_SCAN);
    result.wave_cur.resize(NPOINT_SCAN);

    result.preset_freq = p->freq;
    result.preset_vol_start = p->vol_start;
    result.preset_vol_step  = p->vol_step;
    result.preset_vol_stop  = p->vol_step*NPOINT_SCAN + p->vol_start;

    voltage = p->vol_start;

    for (int i=0; i<NPOINT_SCAN; i++) {
        fpga.dc_ac_f_set(p->freq);
        fpga.select_range_by_value(0, p->range_i);
        fpga.select_range_by_value(1, voltage );
        fpga.select_range_by_value(2, voltage );
        fpga.select_range_by_value(3, voltage );
        fpga.safe_output(voltage);
        usleep(1000*40);
        raw_wave &ch1 = result.wave_cur_primary[i];
        raw_wave &ch2 = result.wave_vol_primary[i];
        raw_wave &ch3 = result.wave_vol[i];
        raw_wave &ch4 = result.wave_cur[i];
        int ret = fpga.read( &ch1, &ch2, &ch3, &ch4 );
        if (ret) {
            return false;
        }
        ch1.calculate_all();
        ch2.calculate_all();
        ch3.calculate_all();
        ch4.calculate_all();
    }
    return true;
}

bool data_source_fpga::auto_scan_next( void )
{
    this->seq++;
    if (this->seq>=g_stratergy_count) {
        this->seq--;
        return false;
    }
    return true;
}


data_source_fpga::~data_source_fpga()
{

}

