
#include <iostream>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "ctpt_api.hpp"
#include "fpga.hpp"
#include "raw_wave.hpp"



extern "C" {
    int open_smc(const char* dev);
    int close_smc(int fd);
    int read_smc_dev_reg(unsigned int reg, unsigned int *val);
    int write_smc_dev_reg(unsigned int reg, unsigned int val);
    int read_smc_dev_sample_reg(unsigned int reg,unsigned int *val);

    int read_sample_buf(unsigned char* buf, int count);
}

FILE *g_log;

/**
 * only works for gcc, pack(1)
 *
 */
#pragma pack(1)
struct fpga_reg
{
    uint32_t     int_en;        /*  */
    uint32_t     int_status;    /*  */
    uint32_t     dc_ac_run_status;
    uint32_t     dc_ac_start;
    uint32_t     dead_time_set;
    uint32_t     dc_dc_v_set;   /* [0~10]: 10ns的计数值,周期为80KHz,12.5us, 1250个计数值 */
    float        dc_ac_v_set;   /* 最大5.0 */
    uint32_t     dc_dc_f_set;   /* 每两个点的时间间隔，以10ns计数 */
    uint32_t     dc_ac_p_set;   /* [0~9]: DC-AC模块输出AC相位,即正弦波的相位, =1000*(360/要输出的角度) */
    uint32_t     power_freq;    /* RO[0~26]:频率计数，基于100MHz; [31]: 1代表超出  */
    uint32_t     utc_high;
    uint32_t     utc_low;       // 12*4 = 48 = 0x3C

    uint32_t     reserved00[4]; //

    uint32_t     fpga_version;  // 0x40

    uint32_t     reserved01[(0x128-0x44)/sizeof(uint32_t)];

    // AD寄存器
    uint32_t     fix_sample_count; // 0x128 固定采样率计数值

    // works for gcc
    union {
        struct {
            uint32_t ch1_range_value : 8;
            uint32_t ch2_range_value : 8;
            uint32_t ch3_range_value : 8;
            uint32_t ch4_range_value : 8;
        };
        uint32_t value;
    }range_set;

    uint32_t     timeout;

    uint32_t     end_addr_count_ping;

    union {
        struct {
            uint32_t frame_length       :16;
            uint32_t sample_rate_set    :3;
            uint32_t reserved           :1;
            uint32_t fix_follow_sel     :1;
            uint32_t reserved2          :11;
        };
        uint32_t value;
    }ping_config;

    union {
        struct {
            uint32_t int_status_timeout : 1;
            uint32_t int_status_full    : 1;
            uint32_t reserved0    : 2;
            uint32_t int_enable_timeout: 1;
            uint32_t int_enable_full: 1;
            uint32_t reserved1: 2;
            uint32_t rx_ad_enable: 1;
            uint32_t reserved3: 23;
        };
        uint32_t value;
    }ping_control;

    uint32_t     end_addr_count_pong;

    union {
        struct {
            uint32_t frame_length       :16;
            uint32_t sample_rate_set    :3;
            uint32_t reserved           :1;
            uint32_t fix_follow_sel     :1;
            uint32_t reserved2          :11;
        };
        uint32_t value;
    }pong_config;               // +reserved+

    union {
        struct {
            uint32_t int_status_timeout : 1;
            uint32_t int_status_full    : 1;
            uint32_t reserved0    : 2;
            uint32_t int_enable_timeout: 1;
            uint32_t int_enable_full: 1;
            uint32_t reserved1: 2;
            uint32_t rx_ad_enable: 1;
            uint32_t reserved3: 23;
        };
        uint32_t value;
    }pong_control;
};

#define reg_offset(member) offsetof(struct fpga_reg, member )
#if 0
#define READ_THIS_REG(m) __read_reg32( (unsigned int)&this->fpga_reg->m )
#define WRITE_THIS_REG(m, value) __write_reg32( (unsigned int)&this->fpga_reg->m, value )
#define WR_THIS_REG(m) __write_reg32( (unsigned int)&this->fpga_reg->m, this->fpga_reg_backup->m.value )

#else
#define READ_THIS_REG(m) __read_reg32( reg_offset(m) )
#define WRITE_THIS_REG(m, value) __write_reg32( reg_offset(m), value )
#define WR_THIS_REG(m) __write_reg32( reg_offset(m), this->fpga_reg_backup->m.value )
#endif


static uint32_t __read_reg32( unsigned int addr )
{
    unsigned int value = 0;
    int ret;
    ret = read_smc_dev_reg( addr, &value );
    if ( ret != 0 ){
        fprintf(g_log, "read addr 0x%X error\n", addr );
    }
    return value;
}

static uint32_t __write_reg32( unsigned int addr, uint32_t value )
{
    printf("write addr 0x%x with 0x%x(%u)\n", addr, value, value );
    return write_smc_dev_reg( (unsigned int)addr, value );
}

static uint32_t __write_reg32( unsigned int addr, double value )
{
    float v = (float)value;
    uint32_t *p = (uint32_t*)&v;
    printf("write addr 0x%x with float 0x%x(%f)\n", addr, *(int*)&value, value );
    return write_smc_dev_reg( (unsigned int)addr, *p );
}

fpga_dev::fpga_dev():
    fpga_reg(0),
    fpga_reg_backup(0),
    hi_aco(0.0),
    fs(0),
    np(0)
{
    this->scales[0] = 1.0;
    this->scales[1] = 1.0;
    this->scales[2] = 1.0;
    this->scales[3] = 1.0;

    this->fix_b[0] = 0.0;
    this->fix_b[1] = 0.0;
    this->fix_b[2] = 0.0;
    this->fix_b[3] = 0.0;
    this->fd = -1;
}

fpga_dev::~fpga_dev()
{
    if (this->fpga_reg_backup){
        free(this->fpga_reg_backup);
        this->fpga_reg_backup = 0;
    }
    if (this->fd >= 0 ){
        close_smc(this->fd);
        this->fd = -1;
    }
}

void fpga_dev::init( void *fpga_reg, double hi_aco, int cycle_count )
{
    // this->fpga_reg = (struct fpga_reg*)fpga_reg;
    this->fpga_reg = (struct fpga_reg*)(struct fpga_reg*)malloc(sizeof(struct fpga_reg));;
    this->hi_aco = hi_aco;
    this->fpga_reg_backup = (struct fpga_reg*)malloc(sizeof(struct fpga_reg) );
    assert(this->fpga_reg_backup);
    memset(this->fpga_reg_backup, 0x00, sizeof(sizeof(fpga_reg)));

    g_log = stdout;
    this->cycle_count = cycle_count;
    this->set_np_per_cycle( 1024 );
    this->fd = open_smc(0);
    assert(this->fd >= 0 );
    fflush(stdout);
}

bool fpga_dev::is_current_overflow(void)
{
    uint32_t value;

    value = READ_THIS_REG(dc_ac_run_status);
    if ( value & 0x01 ) {
        return true;
    } else {
        return false;
    }
}

bool fpga_dev::is_temperature_overflow()
{
    uint32_t value;

    value = READ_THIS_REG(dc_ac_run_status);
    if ( value & 0x02 ) {
        return true;
    } else {
        return false;
    }
}

void fpga_dev::adc_start_sample()
{
    this->fpga_reg_backup->ping_control.rx_ad_enable    = 1;
    this->fpga_reg_backup->ping_control.int_enable_full = 1;
    this->fpga_reg_backup->ping_control.int_enable_timeout = 1;
    WR_THIS_REG(ping_control);
}

void fpga_dev::adc_stop_sample()
{
    this->fpga_reg_backup->ping_control.rx_ad_enable = 0;
}

bool fpga_dev::dc_dc_start()
{
    uint32_t old;
    int bit = 1;

    old = READ_THIS_REG(dc_ac_start);
    old |= bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return true;
    } else {
        return false;
    }
}


bool fpga_dev::dc_ac_enable()
{
    uint32_t old;
    int bit = 2;

    old = READ_THIS_REG(dc_ac_start);
    old |= bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return true;
    } else {
        return false;
    }
}

bool fpga_dev::dc_dc_stop()
{
    uint32_t old;
    uint32_t bit = 1;

    old = READ_THIS_REG(dc_ac_start);
    old &= ~bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return true;
    } else {
        return false;
    }
}


uint32_t fpga_dev::version()
{
    return READ_THIS_REG(fpga_version);
}

bool fpga_dev::dc_ac_start()
{
    uint32_t old;
    int bit = 4;

    old = READ_THIS_REG(dc_ac_start);
    old |= bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return true;
    } else {
        return false;
    }
}

bool fpga_dev::dc_ac_stop()
{
    uint32_t old;
    uint32_t bit = 4;

    old = READ_THIS_REG(dc_ac_start);
    old &= ~bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return true;
    } else {
        return false;
    }
}

int fpga_dev::dead_time_set( uint32_t ns10 )
{
    return WRITE_THIS_REG( dead_time_set, ns10 );
}

int fpga_dev::dc_dc_v_set_percent( double percent )
{
    uint32_t v = (int)(1250.0 * percent/100.0);
    return WRITE_THIS_REG( dc_dc_v_set, v&(0x7ff) );
}

int fpga_dev::dc_ac_v_set(double vol_v)
{
    if ( vol_v > this->hi_aco ) {
        vol_v = this->hi_aco;
    }
    return WRITE_THIS_REG( dc_ac_v_set, (float)(vol_v/this->hi_aco*5.0) );
}

int fpga_dev::dc_ac_f_set(double freq )
{
    uint32_t value;
    int ret;
    value = (uint32_t)((100000000ULL/1000ULL)/freq);
    ret = WRITE_THIS_REG(dc_dc_f_set, value); // 每两个点的时间间隔
    this->freq = freq;
    this->update_fs();
    return ret;
}

int fpga_dev::dc_ac_p_set(double freq )
{
    return true;
}

int fpga_dev::read( raw_wave *wave_ch1,
                    raw_wave *wave_ch2,
                    raw_wave *wave_ch3,
                    raw_wave *wave_ch4 )
{
    int ret = read_sample_buf( (unsigned char*)this->data, sizeof(this->data) );
    if (ret == 0){
        raw_wave *the_raw_waves[4] = { wave_ch1, wave_ch2, wave_ch3, wave_ch4 };
        int np_per_channel = this->np_per_channel;
        double fs = this->fs;
        double freq = this->freq;

        for (int ch=0; ch<4; ch++) {
            if ( !the_raw_waves[ch] ) {
                continue;
            }
            the_raw_waves[ch]->set_channel_id(ch);
            the_raw_waves[ch]->set_dynamic(np_per_channel, fs );
            the_raw_waves[ch]->set_raw_data(this->data[ch], np_per_channel, this->scales[ch] );
            the_raw_waves[ch]->set_freq( freq );
        }
        return 0;
    }
    return -1;
}

int fpga_dev::set_range( int ch1, int ch2, int ch3, int ch4 )
{
    this->fpga_reg_backup->range_set.ch1_range_value = ch1;
    this->fpga_reg_backup->range_set.ch2_range_value = ch2;
    this->fpga_reg_backup->range_set.ch3_range_value = ch3;
    this->fpga_reg_backup->range_set.ch4_range_value = ch4;
    return WR_THIS_REG(range_set);
}

void fpga_dev::update_fs( void )
{
    this->fs = this->freq * this->np_per_cycle;
}

int fpga_dev::set_np_per_cycle( int np_per_cycle )
{
    int v = 7;
    switch(np_per_cycle)
    {
    case 32   : v = 0;break;
    case 64   : v = 1;break;
    case 128  : v = 2;break;
    case 256  : v = 3;break;
    case 512  : v = 4;break;
    case 1024 : v = 5;break;
    case 2048 : v = 6;break;
    case 4096 : v = 7;break;
    default:
        assert(0 && "np per cycle setting error");
        return -1;
    }
    uint64_t reg;
    this->np_per_cycle = np_per_cycle;
    this->np_per_channel = np_per_cycle * this->cycle_count;
    update_fs();
    reg = 100000000ULL*np_per_cycle/this->fs;
    if ( 0xffffffff00000000ULL & reg ) {
        fprintf(g_log, "set sample rate overfllow\n");
        return -1;
    }
    this->fpga_reg_backup->ping_config.fix_follow_sel = 1; // 固定
    this->fpga_reg_backup->ping_config.sample_rate_set = v;
    this->fpga_reg_backup->fix_sample_count = reg;

    WRITE_THIS_REG(fix_sample_count, (uint32_t)reg);
    return WR_THIS_REG(ping_config);
}

/////////////////////////////////////////////////////////////////////////////////
bool fpga_dev::auto_scan_prepare( const struct ct_setting &setting )
{
    return true;
}

bool fpga_dev::auto_scan_prepare( const struct pt_setting &setting )
{
    return true;
}

bool fpga_dev::auto_scan_step( struct auto_scan_result &result )
{
    double start = 0.1;
    double step  = 0.1;
    double stop  = 10;
    bool ret = true;

    result.preset_freq          = 50.0;
    result.preset_current_limit = 5.0;
    result.preset_vol_start     = start;
    result.preset_vol_step      = step;
    result.preset_vol_stop      = stop;

    this->dc_ac_f_set( result.preset_freq );
    // this->dc_dc_v_set_percent( ) TODO: fix dc dc
    // set_range TODO: fix range
    this->set_np_per_cycle( 512 );
    this->dc_dc_start();
    this->dc_ac_enable();
    this->dc_ac_start();

    for (int i=0; start <= stop; start += step,i++) {
        this->dc_ac_v_set( start );
        this->dc_ac_p_set( 0 );
        // TODO: resize these vector
        if ( !this->read( &result.wave_vol_primary[i], &result.wave_cur_primary[i], &result.wave_vol[i], &result.wave_cur[i] ) ){
            ret = false;
            goto done;
        }
    }

done:
    this->dc_ac_v_set( 0.0 );
    this->read( 0, 0, 0, 0 );   // xiaoci
    this->dc_ac_stop();
    this->dc_dc_stop();
    return ret;
}
