
#ifndef FPGA_H
#define FPGA_H

#include <stdint.h>

#include "data_source.hpp"

struct fpga_reg;
class raw_wave;

class fpga_dev:public data_source
{
public:
    fpga_dev();
    virtual ~fpga_dev();

    void init( void *fpga_reg, double hi_aco, int cycle_count );

    // 运行状态
    bool is_current_overflow(void);
    bool is_temperature_overflow(void);

    // ctrl
    bool dc_dc_start(void);
    bool dc_dc_stop(void);
    bool dc_ac_enable(void);
    bool dc_ac_start(void);
    bool dc_ac_stop(void);

    void adc_start_sample(void);
    void adc_stop_sample(void);

    uint32_t version(void);

    // 死区时间设置
    int dead_time_set( uint32_t ns10 );

    int dc_dc_v_set_percent( double percent );

    int dc_ac_v_set( double vol_v ); // in V
    int dc_ac_f_set( double freq ); // in Hz

    int dc_ac_p_set( double init_phase ); // in degree

    int read( raw_wave *wave_ch1,
              raw_wave *wave_ch2,
              raw_wave *wave_ch3,
              raw_wave *wave_ch4 );

    int set_range( int ch1, int ch2, int ch3, int ch4 );

    int set_np_per_cycle( int np_per_cycle );

    void set_scale( double *scale ){
        this->scales[0] = scale[0];
        this->scales[1] = scale[1];
        this->scales[2] = scale[2];
        this->scales[3] = scale[3];
    };

public:
    virtual bool auto_scan_prepare( const struct ct_setting &setting );
    virtual bool auto_scan_prepare( const struct pt_setting &setting );
    virtual bool auto_scan_step( struct auto_scan_result &result );

private:
    struct fpga_reg *fpga_reg;
    struct fpga_reg *fpga_reg_backup;
    double hi_aco;
    double fs;
    double np;
    int np_per_channel;
    int np_per_cycle;
    int cycle_count;
    int16_t data[4][16384];
    double freq;

    double scales[4];
    double fix_b[4];

    void update_fs( void );

    int fd;
};

#endif /* FPGA_H */
