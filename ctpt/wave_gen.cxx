
#include "wave-signal.h"
#include <string.h>

static struct system_wave_signal *g_signal;


void set_global_signal( struct system_wave_signal *sig )
{
    g_signal = sig;
}

extern "C" {

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

    struct fpga_reg reg;

    int open_smc(const char* dev)
    {
        memset( &reg, 0, sizeof(reg) );
        reg.dc_ac_p_set = 0;
        return 0;
    }
    int close_smc(int fd)
    {
        return 0;
    }
    int read_smc_dev_reg(unsigned int addr, unsigned int *val)
    {
        char *p = (char*)&reg;

        p += addr;
        unsigned int *px = (unsigned int*)p;
        *val = *px;
        return 0;
    }
    void fpga_reset( void )
    {
    }

    int write_smc_dev_reg(unsigned int addr, unsigned int val)
    {
        return 0;
    }

    int read_smc_dev_sreg(unsigned int addr,unsigned int *val)
    {
        return 0;
    }

    int lowlevel_init( void )
    {
        return 0;
    }

    void start_sample(void)
    {}

    void stop_sample(void)
    {}

    int read_sample_buf(unsigned char *pbuf, int count)
    {
        int16_t *buf = (int16_t*)pbuf;

        buf = (int16_t*)(pbuf + 2*16384 * 0);
        wave_signal_get_int16( &g_signal->u_a, (int16_t*)buf );
        buf = (int16_t*)(pbuf + 2*16384 * 1);
        wave_signal_get_int16( &g_signal->u_b, (int16_t*)buf );
        buf = (int16_t*)(pbuf + 2*16384 * 2);
        wave_signal_get_int16( &g_signal->u_c, (int16_t*)buf );
        buf = (int16_t*)(pbuf + 2*16384 * 3);
        wave_signal_get_int16( &g_signal->u_n, (int16_t*)buf );
        return 0;
    }
}
