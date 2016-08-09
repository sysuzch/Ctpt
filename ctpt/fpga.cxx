
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
#include <unistd.h>
#include "raw_wave.hpp"


#ifndef NO_LOG
#define CT_LOG(x) printf x
#else
#define CT_LOG(x) do{}while(0)
#endif

#define FPGA_BUFFER_SIZE (16384+4)

#define MAX_OUTPUT_VOL    (200)  // 200V
#define STEABY_OUTPUT_OFFSET    0.1
#define SQRT_2   1.414213562


#define OUTPUT_CRITICAL_VAL1 50.0
#define OUTPUT_CRITICAL_VAL2 12.0


extern "C" {
    int gpio_set(char *label, int direction, int value);
    int gpio_get(char *label);
    void fpga_reset( void );
    int open_smc(const char* dev);
    int close_smc(int fd);
    int read_smc_dev_reg(unsigned int reg, unsigned int *val);
    int write_smc_dev_reg(unsigned int reg, unsigned int val);
    int read_smc_dev_sreg(unsigned int reg,unsigned int *val);

    int lowlevel_init(void);
    int read_sample_buf(unsigned char* buf, int count);
    int start_sample(void);
    int stop_sample(void);
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
    uint32_t     dc_dc_v_set;    /* [0~10]: 10ns的计数值,周期为10KHz,100us, 10000个计数值 */
    float        dc_ac_v_set;   /* 最大5.0 */
    uint32_t     dc_dc_f_set;   /* 每两个点的时间间隔，以10ns计数 */
    uint32_t     dc_ac_p_set;   /* [0~9]: DC-AC模块输出AC相位,即正弦波的相位, =1000*(360/要输出的角度) */
    uint32_t     power_freq;    /* RO[0~26]:频率计数，基于100MHz; [31]: 1代表超出  */
    uint32_t     utc_high;
    uint32_t     utc_low;       // 12*4 = 48 = 0x3C

    uint32_t     reserved00[4]; //

    uint32_t     fpga_version;  // 0x40

    uint32_t     reserved01[(0x128-0x44)/sizeof(uint32_t)];

    // AD
    // uint32_t     rx_start; // 0x128 固定采样率计数值

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
    unsigned int ad_addr = reg_offset(fix_sample_count);

    if (addr >= ad_addr){
        ret = read_smc_dev_sreg( addr, &value );
    } else {
        ret = read_smc_dev_reg( addr, &value );
    }
    if ( ret != 0 ){
        fprintf(g_log, "read addr 0x%X error\n", addr );
    }
    //CT_LOG(("read addr 0x%x: %x(%u)\n", addr, value, value));
    return value;
}

static uint32_t __write_reg32( unsigned int addr, uint32_t value )
{
    int ret = write_smc_dev_reg( (unsigned int)addr, value );
    uint32_t r = __read_reg32(addr);
    //CT_LOG(("write addr 0x%x with 0x%x(%u), read=%x %s\n", addr, value, value, r, ((value==r&&ret==0)?"ok":"ERROR") ));
    (void)r;
    return ret;
}

static uint32_t __write_reg32( unsigned int addr, double value )
{
    float v = (float)value;
    int ret;
    uint32_t *p = (uint32_t*)&v;
    //CT_LOG(("write addr 0x%x with float 0x%x(%f) ", addr, *(int*)&value, value ));
    ret = write_smc_dev_reg( (unsigned int)addr, *p );

    uint32_t r = __read_reg32(addr);
    float f;

    *(uint32_t*)&f = r;
    //CT_LOG(("read float=%f", f));

    return ret;
}


//#define _FS (0.707*0.9)
//static const struct adc_range g_tc[] = {
//    {0x00, 0,              0.01690*_FS, 1.25 / (32768.0 * 0.505*3.0*49.19*2*0.5)},
//    {0x10, 0.0169*_FS,     0.03385*_FS, 1.25 / (32768.0 * 0.505*3.0*49.19*1*0.5)},
//    {0x50, 0.03385*_FS,    0.06667*_FS, 1.25 / (32768.0 * 0.505*1.5*49.19*1*0.5)},
//    {0x20, 0.06667*_FS,    0.16670*_FS, 1.25 / (32768.0 * 0.505*3.0* 5.00*2*0.5)},
//    {0x30, 0.16670*_FS,    0.33333*_FS, 1.25 / (32768.0 * 0.505*3.0* 5.00*1*0.5)},
//    {0x70, 0.33333*_FS,    0.66667*_FS, 1.25 / (32768.0 * 0.505*1.5* 5.00*1*0.5)},
//    {0x81, 0.6667*_FS,      1.6900*_FS, 1.25 / (32768.0 * 0.005*3.0*49.19*2*0.5)},
//    {0x91, 1.6900*_FS,      3.3850*_FS, 1.25 / (32768.0 * 0.005*3.0*49.19*1*0.5)},
//    {0xd1, 3.3850*_FS,      6.6670*_FS, 1.25 / (32768.0 * 0.005*1.5*49.19*1*0.5)},
//    {0xa1, 6.6670*_FS,     16.6700*_FS, 1.25 / (32768.0 * 0.005*3.0* 5.00*2*0.5)},
//    {0xb1, 16.6700*_FS,    33.3330*_FS, 1.25 / (32768.0 * 0.005*3.0* 5.00*1*0.5)},
//    {0xF1, 33.3330*_FS,    66.6670*_FS, 1.25 / (32768.0 * 0.005*1.5* 5.00*1*0.5)},
//};

//static const struct adc_range g_tv[] = {
//    {0x05,   0,             0.902*_FS, 1.25 / (32768.0 * 0.06600* 21.0)},
//    {0x03,   0.902*_FS,     4.300*_FS, 1.25 / (32768.0 * 0.06600*  4.4)},
//    {0x01,   4.3  *_FS,     18.94*_FS, 1.25 / (32768.0 * 0.0660 *  1.0)},
//    {0x04,  18.94 *_FS,     87.50*_FS, 1.25 / (32768.0 * 0.00068* 21.0)},
//    {0x02,  87.5  *_FS,    417.80*_FS, 1.25 / (32768.0 * 0.00068*  4.4)},
//    {0x00, 417.8  *_FS,   1838.20*_FS, 1.25 / (32768.0 * 0.00068*  1.0)},
//};
#define _FS (0.707)
#define RANGE_MIN_PERECNT (0.3)
#define RANGE_MAX_PERECNT (0.8)
//必须保证每个区间有重合
static const struct adc_range g_tc[] = {
    {0x00, 0,                               0.01690*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*3.0*49.19*2*0.5)},
    {0x10, 0.03385*_FS*RANGE_MIN_PERECNT,   0.03385*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*3.0*49.19*1*0.5)},
    {0x50, 0.06667*_FS*RANGE_MIN_PERECNT,   0.06667*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*1.5*49.19*1*0.5)},
    {0x20, 0.16670*_FS*RANGE_MIN_PERECNT,   0.16670*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*3.0* 5.00*2*0.5)},
    {0x30, 0.33333*_FS*RANGE_MIN_PERECNT,   0.33333*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*3.0* 5.00*1*0.5)},
    {0x70, 0.66667*_FS*RANGE_MIN_PERECNT,   0.66667*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.505*1.5* 5.00*1*0.5)},
    {0x81, 1.6900 *_FS*RANGE_MIN_PERECNT,   1.6900 *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*3.0*49.19*2*0.5)},
    {0x91, 3.3850 *_FS*RANGE_MIN_PERECNT,   3.3850 *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*3.0*49.19*1*0.5)},
    {0xd1, 6.6670 *_FS*RANGE_MIN_PERECNT,   6.6670 *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*1.5*49.19*1*0.5)},
    {0xa1, 16.670 *_FS*RANGE_MIN_PERECNT,   16.6700*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*3.0* 5.00*2*0.5)},
    {0xb1, 33.3330*_FS*RANGE_MIN_PERECNT,   33.3330*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*3.0* 5.00*1*0.5)},
    {0xF1, 66.6670*_FS*RANGE_MIN_PERECNT,   66.6670*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.005*1.5* 5.00*1*0.5)},
};

static const struct adc_range g_tv[] = {
    {0x05,   0,                             0.902  *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.06600* 21.0)},
    {0x03,   0.902  *_FS*RANGE_MAX_PERECNT, 4.300  *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.06600*  4.4)},
    {0x01,   4.300  *_FS*RANGE_MAX_PERECNT, 18.94  *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.0660 *  1.0)},
    {0x04,   18.94  *_FS*RANGE_MAX_PERECNT, 87.50  *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.00068* 21.0)},
    {0x02,   87.50  *_FS*RANGE_MAX_PERECNT, 417.80 *_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.00068*  4.4)},
    {0x00,   417.80 *_FS*RANGE_MAX_PERECNT, 1838.20*_FS*RANGE_MAX_PERECNT, 1.25 / (32768.0 * 0.00068*  1.0)},
};


inline bool is_outoff_range(double v, const struct adc_range *range)
{
    return (v < range->range_min*0.8 || v > range->range_max);
}



fpga_dev::fpga_dev():
    fpga_reg(0),
    fpga_reg_backup(0),
    fs(0),
    np(0),
    data(0),
    data_ok(0),
    sample_started(false),
    output_started(false)
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

    printf("==fpga_dev::fpga_dev==\n");
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

    if (this->data){
        free(this->data);
    }
    if (this->data_ok){
        free(this->data_ok);
    }
}



void fpga_dev::init( void *fpga_reg, int cycle_count )
{
    printf("====fpga_dev::init=====\n");
    this->fpga_reg = (struct fpga_reg*)(struct fpga_reg*)malloc(sizeof(struct fpga_reg));
    this->fpga_reg_backup = (struct fpga_reg*)malloc(sizeof(struct fpga_reg) );
    assert(this->fpga_reg_backup);
    memset(this->fpga_reg_backup, 0x00, sizeof(sizeof(fpga_reg)));

    // reset FPGA
    fpga_reset();

    g_log = stdout;
    this->cycle_count = cycle_count;
    // this->set_np_per_cycle( 1024 );
    this->fd = open_smc(0);
    // this->fd = lowlevel_init();
    assert(this->fd >= 0 );
    CT_LOG(("version=0x%x\n", READ_THIS_REG(fpga_version) ));
    fflush(stdout);

    int size             = FPGA_BUFFER_SIZE*cycle_count;
    this->data           = (int16_t*)malloc(size);
    this->data_ok        = (int16_t*)malloc(size);
    this->data_buffer_np = size/sizeof(this->data[0]);
    assert(this->data);
    assert(this->data_ok);

    // max range select
    this->set_max_range();
    this->adc_stop_sample();
    this->stop_output();
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

bool fpga_dev::adc_start_sample()
{
    // this->fpga_reg_backup->ping_control.rx_ad_enable    = 1;
    // this->fpga_reg_backup->ping_control.int_enable_full = 1;
    // this->fpga_reg_backup->ping_control.int_enable_timeout = 1;
    // WR_THIS_REG(ping_control);
    if (!sample_started)
        sample_started = (start_sample() == 0) ? true : false;

    return sample_started;
}

bool fpga_dev::adc_stop_sample()
{
    // this->fpga_reg_backup->ping_control.rx_ad_enable = 0;
    int ret = stop_sample();
    if (ret == 0) {
        sample_started = false;
        return true;
    }
    
    return false;
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
        return false;
    } else {
        return true;
    }
}


uint32_t fpga_dev::version()
{
    return READ_THIS_REG(fpga_version);
}

bool fpga_dev::dc_ac_start()
{
    int bit = 4;

    uint32_t old = READ_THIS_REG(dc_ac_start);
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
    uint32_t bit = 4;

    uint32_t old = READ_THIS_REG(dc_ac_start);
    old &= ~bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return false;
    } else {
        return true;
    }
}


bool fpga_dev::dc_ac_disable(void)
{
    uint32_t old;
    int bit = 2;

    old = READ_THIS_REG(dc_ac_start);
    old &= ~bit;
    WRITE_THIS_REG( dc_ac_start, old );
    old = READ_THIS_REG(dc_ac_start);
    if ( old & bit ){
        return false;
    } else {
        return true;
    }
}

int fpga_dev::dead_time_set( uint32_t ns10 )
{
    return WRITE_THIS_REG( dead_time_set, ns10 );
}

/*
 *  @Func:  开始电源模块输出
 *
 *  @Return:
 *      0: success
 *     <0: failed
 *  @Note:
 *     1、电源模块的输出时能，输出电压为0V。仅需要调用一次，后续直接调用set_v配置输出电压即可。
 *     2、调用stop_output函数停止电源模块输出
 */
int fpga_dev::start_output()
{
    if (this->output_started) {
        return 0;
    }

    this->output_voltage = 0.0;
    this->dc_ac_v_set_percent(0.0);
    this->dc_dc_v_set_percent(0.0);

    this->dc_ac_enable();
    this->dc_dc_start();
    this->dc_ac_start();

    this->output_started = true;
    return 0;
}

/*
 *  @Func:  停止电源模块输出
 *
 *  @Return:
 *      0: success
 *     <0: failed
 *  @Note:
 *     1、停止电源的输出，在停止前会配置电源模块输出到0V。
 */
int fpga_dev::stop_output()
{
    if (this->output_started) {
        this->demagnetization();
    }
    this->dc_ac_stop();
    this->dc_dc_stop();
    this->dc_ac_disable();
    this->output_started = false;

    return 0;
}

/*
 *  @Func:  safe_output
 *  @Desc:
 *     电源的输出不能跳变，只能平稳过度，配置电源安全输出到指定电压
 *  @Input:
 *     v:               output voltage
 *     adapte_range:    适应AD量程
 *
 *  @Return:
 *      0: success
 *     <0: failed
 *  @Note:
 *     1、changed ADC Range of SRC_VOL_CHNL and SECOND_VOL_CHNL based on \a v
 *     2、changed ADC Range of LOAD_CUR_CHNL and PRI_VOL_CHNL to max range.
 *     3、如果未启动电源输出，返回-2
 */
int fpga_dev::safe_output(double v, bool adapte_range)
{
    if (!this->output_started) {
        return -1;
    }

    if (v == this->output_voltage) {
        return 0;
    }

    if (adapte_range) {
        if (v > this->output_voltage) {
            //适应源输出和二次测量输入AD的量程
            if (is_outoff_range(v, this->adc_range[SRC_VOL_CHNL]))
                this->select_range_by_value(SRC_VOL_CHNL, v);
            //if (is_outoff_range(v, this->adc_range[LOAD_VOL_CHNL]))
                //this->select_range_by_value(LOAD_VOL_CHNL, v;
            this->select_range_by_value(LOAD_CUR_CHNL, 6.0);
            #if EN_PRI_VOL_CHNL_ADAPTIVE
            this->select_range_by_value(PRI_VOL_CHNL, 300.0);
            #endif
        }
    }
    else {
        this->set_max_range();
    }

    this->set_v(v);

    if (adapte_range && v < this->output_voltage) {
        //适应源输出和二次测量输入AD的量程
        if (is_outoff_range(v, this->adc_range[SRC_VOL_CHNL]))
            this->select_range_by_value(SRC_VOL_CHNL, v);
        //if (is_outoff_range(v, this->adc_range[LOAD_VOL_CHNL]))
            //this->select_range_by_value(LOAD_VOL_CHNL, v;
    }

    return 0;
}

/*
 *  @Func:  wait for output steaby
 *
 *  @Return:
 *      0: success
 *     <0: failed
 *
 *  @Note:
 *     must set AD range and start AD sample.
 */
int fpga_dev::wait_output_steaby(AD_CHNL_STYLE chnl )
{
    bool ok = false;
    int ret = -1;
    int ncnt = 0;

    if(!this->is_sample_started()) {
        printf("wait_for_steaby_output: adc not started\n");
        return -1;
    }

    double fSrcOutputVol = 0.0;
    raw_wave srcRawWave;
    raw_wave *pRawWave[fpga_dev::MAX_CHNL_NUM] = {
        0,0,0,0
    };
    pRawWave[chnl] = &srcRawWave;

    do {
        ret = this->read( pRawWave[0], pRawWave[1], pRawWave[2], pRawWave[3]);
        if (ret != 0) {
            break;
        }
        for(int i =0; i < fpga_dev::MAX_CHNL_NUM; i++) {
            if (pRawWave[i]) {
                pRawWave[i]->calculate_rms();
                if ((fabs(pRawWave[i]->get_rms() - fSrcOutputVol) < STEABY_OUTPUT_OFFSET)) {
                    ok = true;
                }
                else {
                    fSrcOutputVol = pRawWave[i]->get_rms();
                }
            }
        }
        ncnt++;

    }while(!ok&&ncnt < 200);

    if (!ok || ret !=0) {
        CT_LOG(("wait_output_steaby fail.\n"));
        return -1;
    }
    CT_LOG(("++ chnl:%d rms:%f ncnt=%d\n", chnl+1, fSrcOutputVol, ncnt));
    return 0;
}

/*
 *  @Func:  自动量程切换
 *
 *  @input:
 *
 *  @Return:
 *      0: success
 *     -1: failed
 *
 *  @Note:
 *      启动测试后，必须先选择好量程
 */
int fpga_dev::auto_select_range(bool ILoad, bool UPri, bool USrc, bool ULoad)
{
    int ret = 0;
    raw_wave rawWave[fpga_dev::MAX_CHNL_NUM];
    raw_wave *pRawWave[fpga_dev::MAX_CHNL_NUM] = {
        0, 0, 0, 0
    };
    uint8_t range_backup[fpga_dev::MAX_CHNL_NUM] = {
        0xFF,0xFF,0xFF,0xFF
    };

    if (ILoad)
        pRawWave[LOAD_CUR_CHNL] = &rawWave[LOAD_CUR_CHNL];
#if EN_PRI_VOL_CHNL_ADAPTIVE
    if (UPri)
        pRawWave[PRI_VOL_CHNL] = &rawWave[PRI_VOL_CHNL];
#endif
    if (USrc)
        pRawWave[SRC_VOL_CHNL] = &rawWave[SRC_VOL_CHNL];
    if (ULoad)
        pRawWave[LOAD_VOL_CHNL] = &rawWave[LOAD_VOL_CHNL];

    this->adc_stop_sample();
    this->set_max_range();
    int flag = 0;
    for(int i = 0; i < fpga_dev::MAX_CHNL_NUM; i++) {
        range_backup[i] = this->get_range(i);
        if (pRawWave[i])
            flag |= 1<<i;
    }

    if (!this->adc_start_sample()) {
        return -1;
    }
    while(flag)
    {
        ret = this->read( pRawWave[0], pRawWave[1], pRawWave[2], pRawWave[3]);
        if (ret != 0) {
            break;
        }
        for(int i =0; i < fpga_dev::MAX_CHNL_NUM; i++) {
            if (pRawWave[i]) {
                pRawWave[i]->calculate_rms();
                this->select_range_by_value(i, pRawWave[i]->get_rms());
                uint8_t range = this->get_range(i);
                if (range_backup[i] == range) {
                    pRawWave[i] = 0;
                }
                else {
                    range_backup[i] = range;
                }
            }
            else {
                flag &= ~(1<<i);
            }
        }
    }

    if (ret != 0) {
        this->adc_stop_sample();
        printf("auto_select_range Fail:\n");
    }
    else {
        printf("auto_select_range OK:\n");
    }
    return ret;
 }

/*
 *  @Func:  退磁
 *
 *  @input:
 *      none
 *
 *  @Return:
 *      0: success
 *     -1: failed
 *
 *  @Note:
 *      测试结束后，必须对CT/PT退磁操作
 */
int fpga_dev::demagnetization()
{
    printf("fpga_dev::demagnetization\n");
    if (this->output_voltage > 50.0) {
        for(double v = this->output_voltage; v >= 30.0; v-=10.0) {
            safe_output(v, false);
            sleep(1);
        }
    }
    if (this->output_voltage > 15.0) {
        for(double v = this->output_voltage; v >= 10.0; v-=5.0) {
            safe_output(v, false);
            sleep(1);
        }
    }
    if (this->output_voltage > 1.0) {
        for(double v = this->output_voltage; v >= 0.5; v-=2.0) {
            safe_output(v, false);
            sleep(1);
        }
    }
    for(double v = this->output_voltage; v >= 0.0; v-=0.3) {
        safe_output(v, false);
        sleep(1);
    }

    return 0;
}


/*
 *  @Func:  get_active_sample_wave
 *
 *  @Desc:
 *
 *  @input:
 *      none
 *  @Return:
 *      0: success
 *     -1: failed
 *  @Note:
 *      adaptive ADC range for LOAD_CUR_CHNL and PRI_VOL_CHNL
 */
int fpga_dev::get_active_sample_wave(raw_wave *wave_i_load,
                                     raw_wave *wave_u_src,
                                     raw_wave *wave_u_pri,
                                     raw_wave *wave_u_load)
{
    int ret = 0;
    int flag = 0;

    do {
        flag = 0;

        ret = this->read(wave_i_load, wave_u_src, wave_u_pri, wave_u_load);
        if (ret == 0) {
            wave_i_load->calculate_all();
#if EN_PRI_VOL_CHNL_ADAPTIVE
            wave_u_pri->calculate_all();
#endif

            double rms = wave_i_load->get_base_rms();
            if (is_outoff_range(rms, this->adc_range[LOAD_CUR_CHNL])) {
                if (rms < 0.3 || rms > 1.0) {
                    this->select_range_by_value(LOAD_CUR_CHNL, wave_i_load->get_base_rms());
                    printf("++++ select range++++\n");
                    flag |= 1;
                }
            }
            //// pri adc chnl has error!!!!!  adc not adapte range.
#if EN_PRI_VOL_CHNL_ADAPTIVE
            if (is_outoff_range(wave_u_pri->get_base_rms(), this->adc_range[PRI_VOL_CHNL])) {
                this->select_range_by_value(PRI_VOL_CHNL, wave_u_pri->get_base_rms());
                flag |= 2;
            }
#endif
            if (flag)
            {
                adc_stop_sample();
                printf("++++ stop adc++++\n");
                usleep(500000);
                adc_start_sample();
                printf("++++ start adc++++\n");
                usleep(500000);
            }
        }
        else {
            break;
        }
    } while(flag);

    if (ret == 0) {
        ret = this->read(wave_i_load, wave_u_src, wave_u_pri, wave_u_load);
        wave_i_load->calculate_all();
        wave_u_src->calculate_all();
        wave_u_pri->calculate_all();
        wave_u_load->calculate_all();
    }
    else {
        printf("fpga_dev::get_active_sample_wave found error\n");
    }

    return ret;
}

static void calc_percent(double v, double &percent, double &aco)
{
    if (v > OUTPUT_CRITICAL_VAL1) {
        percent = v / 200.0 * 100;
        aco = 5.0;
    }
    else if (v >= OUTPUT_CRITICAL_VAL2) {
        percent = OUTPUT_CRITICAL_VAL1/200.0*100.0;
        aco = v/OUTPUT_CRITICAL_VAL1*5.0;
    }
    else {
        percent = 0.0;
        aco = v/OUTPUT_CRITICAL_VAL2*5.0;
    }
}
int fpga_dev::set_v(double v)
{
    if (v == this->output_voltage) {
        return 0;
    }
    double spwm, pwm;
    calc_percent(v, spwm, pwm);
    if (pwm > this->pwm_percent && spwm < this->spwm_percent) {
        dc_dc_v_to_percent(spwm);
        dc_ac_v_to_percent(pwm);
    }
    else {
        dc_ac_v_to_percent(pwm);
        dc_dc_v_to_percent(spwm);
    }
    this->output_voltage = v;
    return 0;
}

int fpga_dev::dc_dc_v_set_percent( double spwm_percent )
{
    uint32_t v = (int)(10000.0 * spwm_percent/100.0);
    if (v > 10000) {
        v = 10000;
        spwm_percent = 100.0;
    }
    this->spwm_percent = spwm_percent;
    return WRITE_THIS_REG( dc_dc_v_set, v );
}

int fpga_dev::dc_ac_v_set_percent(double pwm_percent)
{
    if ( pwm_percent > 5.0 ) {
        pwm_percent = 5.0;
    }
    this->pwm_percent = pwm_percent;
    return WRITE_THIS_REG( dc_ac_v_set, pwm_percent );
}


int fpga_dev::dc_dc_v_to_percent(double percent)
{
    if (this->spwm_percent == percent) {
        return 0;
    }
    double step = (percent - this->spwm_percent)/10;
    if (step > 0.5) {
        for(int i= 0; i < 10; i++) {
            this->dc_dc_v_set_percent(this->spwm_percent+step);
            usleep(10000);
        }
    }
    if (this->spwm_percent != percent) {
        this->dc_dc_v_set_percent(percent);
        usleep(10000);
    }
    return 0;
}

int fpga_dev::dc_ac_v_to_percent(double percent)
{
    if (this->pwm_percent == percent) {
        return 0;
    }
    double step = (percent - this->pwm_percent)/10;
    if (step > 0.05) {
        for(int i= 0; i < 5; i++) {
            this->dc_ac_v_set_percent(this->pwm_percent+step);
            usleep(10000);
        }
    }
    if (this->pwm_percent != percent) {
        this->dc_ac_v_set_percent(percent);
        usleep(10000);
    }
    return 0;
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

int fpga_dev::read( raw_wave *wave_i_load,
                    raw_wave *wave_u_src,
                    raw_wave *wave_u_pri,
                    raw_wave *wave_u_load )
{
    int n = this->data_buffer_np * sizeof(this->data[0]);
    memset( (unsigned char*)this->data, 0, n );
    int ret = read_sample_buf( (unsigned char*)this->data, n );
    //CT_LOG(("read_sample_buf(p, %u) return %d\n", n, ret));
    assert( ret == n );
    if (ret >= 0) {
        raw_wave *the_raw_waves[4] = { wave_i_load, wave_u_src, wave_u_pri, wave_u_load };
        int np_per_channel = this->np_per_channel;
        double fs = this->fs;
        double freq = this->freq;
        int to_np = 2048;

        int m = np_per_channel / to_np;
        //CT_LOG(("m=%d np_per_channel=%d cycle_count=%d\n", m, np_per_channel, cycle_count));

        // assert( m==0 || (m*to_np == np_per_channel) );
        // assert( m==0 || (to_np * cycle_count <= np_per_channel) );
        // if ( !(m*to_np == np_per_channel) ){
        //     CT_LOG(("ERROR: %s:%u ", __FILE__, __LINE__));
        // }
        if (m==0){
            m = 1;
        }
        for (int ch=0; ch<4; ch++) {
            int16_t *p  = this->data + 1 + ch*(to_np);

            if (the_raw_waves[ch]) {
                the_raw_waves[ch]->set_channel_id(ch);
                the_raw_waves[ch]->set_dynamic(np_per_channel, fs );
                the_raw_waves[ch]->set_freq( freq );

                int offset = 0;
                for (int i=0; i<m; i++) {
                    int np_need = (np_per_channel>to_np)?(to_np):(np_per_channel);
                    the_raw_waves[ch]->set_raw_data(p, offset, np_need, this->scales[ch] );
                    p  += FPGA_BUFFER_SIZE/sizeof(p[0]);
                    offset += np_need;
                }

            }
        }

        return 0;
    }
    return ret;
}

/*
 *  @Func:  select_range_by_value
 *
 *  @Desc:
 *      根据电压或电流值来选择AD相应的量程。
 *
 *  @input:
 *      chnl:     AD通道号 0:电流通道 1/2/3:电压通道
 *      rms:      电压电流的有效值
 *
 *  @Return:
 *      true: success
 *     false: failed
 *
 *  @Note:
 *     实际量程 = rms*sqrt(2)*2
 */
bool fpga_dev::select_range_by_value( int chnl, double rms )
{
    const char *str[] ={
        "Iload",
        "Usrc ",
        "Upri ",
        "Uload",
    };
    const struct adc_range *p_range = 0;
    double value = rms;

    if (chnl<0 || chnl >= 4){
        CT_LOG(("select range_by value ch NOT in range. ch=%d\n", chnl));
        return false;
    }

    if (chnl!=LOAD_CUR_CHNL){
        int n = sizeof(g_tv)/sizeof(g_tv[0]);
        for (int i=0; i<n; i++) {
            if ( value >= g_tv[i].range_min && value <=g_tv[i].range_max ){
                p_range = &g_tv[i];
            }
        }
        if ( value >= g_tv[n-1].range_max ){
            p_range = &g_tv[n-1];
        }
    } else {
        int n = sizeof(g_tc)/sizeof(g_tc[0]);
        for (int i=0; i<n; i++) {
            if ( value >= g_tc[i].range_min && value <= g_tc[i].range_max ){
                p_range = &g_tc[i];
            }
        }
        if ( value >= g_tc[n-1].range_max ){
            p_range = &g_tc[n-1];
        }
    }

    if (p_range) {
        CT_LOG(("select_ch_range: <%s>: found range: %02XH(%f -> %f) scale:%f by %f\n",
                str[chnl],
                p_range->fpga_reg_value,
                p_range->range_min,
                p_range->range_max,
                p_range->scale,
                value
                   ));
    } else {
        CT_LOG(("select_ch_range: <%s>: NOT found range by %f\n", str[chnl], value ));
        return false;
    }

    this->adc_range[chnl] = p_range;
    this->scales[chnl] = p_range->scale;
    switch (chnl) {
    case 0:
        this->fpga_reg_backup->range_set.ch1_range_value = p_range->fpga_reg_value;
        break;
    case 1:
        this->fpga_reg_backup->range_set.ch2_range_value = p_range->fpga_reg_value;
        break;
    case 2:
        this->fpga_reg_backup->range_set.ch3_range_value = p_range->fpga_reg_value;
        break;
    case 3:
        this->fpga_reg_backup->range_set.ch4_range_value = p_range->fpga_reg_value;
        break;
    }

    if ( WR_THIS_REG(range_set) == 0 ){
        return true;
    }
    return false;
}

//int fpga_dev::set_range( int ch1, int ch2, int ch3, int ch4 )
//{
//    this->fpga_reg_backup->range_set.ch1_range_value = ch1;
//    this->fpga_reg_backup->range_set.ch2_range_value = ch2;
//    this->fpga_reg_backup->range_set.ch3_range_value = ch3;
//    this->fpga_reg_backup->range_set.ch4_range_value = ch4;

//    return WR_THIS_REG(range_set);
//}


int fpga_dev::set_max_range()
{
    CT_LOG(("++set_max_range++\n"));
    this->select_range_by_value(LOAD_CUR_CHNL, 100.0);
    this->select_range_by_value(SRC_VOL_CHNL, 1200.0);
    this->select_range_by_value(PRI_VOL_CHNL, 1200.0);
    this->select_range_by_value(SECOND_VOL_CHNL, 1200.0);
    CT_LOG(("--set_max_range--\n"));
    return 0;
}

uint8_t fpga_dev::get_range(int chnl)
{
    if (chnl == 0) {
        return this->fpga_reg_backup->range_set.ch1_range_value;
    }
    else if (chnl == 1) {
        return this->fpga_reg_backup->range_set.ch2_range_value;
    }
    else if (chnl == 2) {
        return this->fpga_reg_backup->range_set.ch3_range_value;
    }
    else if (chnl == 3) {
        return this->fpga_reg_backup->range_set.ch4_range_value;
    }
    printf("pga_dev::get_range chnl(%d) error\n", chnl);
    return 0;
}

void fpga_dev::update_fs( void )
{
    this->fs = this->freq * this->np_per_cycle;
    CT_LOG(("fs=%f\n", this->fs));
}

int fpga_dev::set_np_per_cycle( int np_per_cycle )
{
    int v = 7;
    CT_LOG(("setting np_per_cycle=%u\n", np_per_cycle));
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
    CT_LOG(("64bitREG=%llx, fs=%f\n", reg, this->fs));
    if ( 0xffffffff00000000ULL & reg ) {
        fprintf(g_log, "set sample rate overfllow\n");
        return -1;
    }

    this->fpga_reg_backup->ping_config.fix_follow_sel  = 1; // 固定
    this->fpga_reg_backup->ping_config.sample_rate_set = v;
    this->fpga_reg_backup->fix_sample_count            = reg;
    this->fpga_reg_backup->ping_config.frame_length    = 128;

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
    this->start_output();

    for (int i=0; start <= stop; start += step,i++) {
        this->set_v(start);
        // TODO: resize these vector
        if ( !this->read( &result.wave_vol_primary[i], &result.wave_cur_primary[i], &result.wave_vol[i], &result.wave_cur[i] ) ){
            ret = false;
            goto done;
        }
    }

done:
    this->stop_output();
    return ret;
}
