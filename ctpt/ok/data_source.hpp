
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stdint.h>
#include <vector>

#include "raw_wave.hpp"

struct ct_setting;
struct pt_setting;

struct auto_scan_result
{
    double preset_freq;
    double preset_vol_start;
    double preset_vol_step;
    double preset_vol_stop;
    double preset_current_limit;
    std::vector<raw_wave> wave_vol_primary; // 初级线圈电压
    std::vector<raw_wave> wave_cur_primary; // 初级线圈电
    std::vector<raw_wave> wave_vol;
    std::vector<raw_wave> wave_cur;
};


enum connect_type
{
    PT_RATIO = 0,
    CT_RATIO = 1,               // CT_RATIO 和 CT_EXCITING可以合并
    CT_EXCITING = 2,            // CT_RATIO 和 CT_EXCITING可以合并
    CT_BURDEN = 4,
};

class data_source
{
public:
    data_source();
    virtual ~data_source();

    virtual bool set_range( double need );
    virtual bool set_preset_voltage( double voltage, double freq );
    virtual bool set_preset_current( double current, double freq );
    virtual bool set_sample_parameter( double sample_rate, int np );
    virtual bool sync( void );
    virtual bool get_adc_value( double *in_vol, double *in_cur, double *out_vol, double *out_cur, double *freq, int np );

    ///  auto scan with limit current, fix freq
    virtual bool auto_scan_prepare( const struct ct_setting & );
    virtual bool auto_scan_prepare( const struct pt_setting & );
    virtual bool auto_scan_step( struct auto_scan_result &result );
    // virtual bool auto_scan_stop( void );

    void set_connect_type( enum connect_type wire_type ){ this->wire_type = wire_type;}
    enum connect_type get_connect_type( void ){ return this->wire_type;}

private:
    enum connect_type wire_type;
};


#endif /* DATA_SOURCE_H */

