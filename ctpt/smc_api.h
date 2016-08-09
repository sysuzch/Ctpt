#ifndef GPIO_API_H
/* To assert that only one occurrence is included */
#define GPIO_API_H


#ifdef __cplusplus
extern "C" {
#endif


#define SMC_NODE "/dev/hsmc"
#define GPIO_NODE "/dev/user_gpios"

    struct user_gpio {
        char *label;
        int gpio;
        int type;
        int value;
    };

    enum {
        GPIO_DIRECTION_OUT = 0,
        GPIO_DIRECTION_IN,
    };


    extern int gpio_set(char *label, int direction, int value);
    extern int gpio_get(char * label);

#define WR_DEVICE_REG _IOW('S', 0x20, int)  
#define RD_DEVICE_REG _IOWR('S', 0x21, int)  
#define WR_HSMC_CFG _IOW('S', 0x22, int)  
#define RD_HSMC_CFG _IOWR('S', 0x23, int)  
#define CL_BUF_STATE _IOWR('S', 0x24, int) 

#define CTRL_NCS0 0
#define DATA_NCS1 1
#define DATA_NCS2 2

#define INT_REG 0x000
#define STS_REG 0X004

#define PING_END_ADDR 0x134
#define PONG_END_ADDR 0x174

#define PING_CTRL 0x13c
#define PONG_CTRL 0x17c

    struct device_reg{
        unsigned int ncs;
        unsigned int reg;
        unsigned int val;
    };

    typedef unsigned char u8;
    typedef unsigned short u16;
    struct smc_config {
        int ncs;

        /* Setup register */
        u8 ncs_read_setup;
        u8 nrd_setup;
        u8 ncs_write_setup;
        u8 nwe_setup;

        /* Pulse register */
        u8 ncs_read_pulse;
        u8 nrd_pulse;
        u8 ncs_write_pulse;
        u8 nwe_pulse;

        /* Cycle register */
        u16 read_cycle;
        u16 write_cycle;

        /* Mode register */
        u8 tdf_mode;
        u8 tdf_cycles;
        u16 mode;
    };


#define     AT91_SMC_SETUP      0x00                /* Setup Register for CS n */
#define     AT91_SMC_NWESETUP   (0x3f << 0)         /* NWE Setup Length */
#define     AT91_SMC_NWESETUP_(x)   ((x) << 0)
#define     AT91_SMC_NCS_WRSETUP    (0x3f << 8)         /* NCS Setup Length in Write Access */
#define     AT91_SMC_NCS_WRSETUP_(x)    ((x) << 8)
#define     AT91_SMC_NRDSETUP   (0x3f << 16)            /* NRD Setup Length */
#define     AT91_SMC_NRDSETUP_(x)   ((x) << 16)
#define     AT91_SMC_NCS_RDSETUP    (0x3f << 24)            /* NCS Setup Length in Read Access */
#define     AT91_SMC_NCS_RDSETUP_(x)    ((x) << 24)

#define     AT91_SMC_PULSE      0x04                /* Pulse Register for CS n */
#define     AT91_SMC_NWEPULSE   (0x7f <<  0)            /* NWE Pulse Length */
#define     AT91_SMC_NWEPULSE_(x)   ((x) << 0)
#define     AT91_SMC_NCS_WRPULSE    (0x7f <<  8)            /* NCS Pulse Length in Write Access */
#define     AT91_SMC_NCS_WRPULSE_(x)((x) << 8)
#define     AT91_SMC_NRDPULSE   (0x7f << 16)            /* NRD Pulse Length */
#define     AT91_SMC_NRDPULSE_(x)   ((x) << 16)
#define     AT91_SMC_NCS_RDPULSE    (0x7f << 24)            /* NCS Pulse Length in Read Access */
#define     AT91_SMC_NCS_RDPULSE_(x)((x) << 24)

#define     AT91_SMC_CYCLE      0x08                /* Cycle Register for CS n */
#define     AT91_SMC_NWECYCLE   (0x1ff << 0 )           /* Total Write Cycle Length */
#define     AT91_SMC_NWECYCLE_(x)   ((x) << 0)
#define     AT91_SMC_NRDCYCLE   (0x1ff << 16)           /* Total Read Cycle Length */
#define     AT91_SMC_NRDCYCLE_(x)   ((x) << 16)

#define     AT91_SMC_MODE       0x0c                /* Mode Register for CS n */
#define     AT91_SMC_HSMC_MODE  (0xffff<<0)
#define     AT91_SMC_HSMC_MODE_(x)  ((x)<<0)
#define     AT91_SMC_READMODE   (1 <<  0)           /* Read Mode */
#define     AT91_SMC_WRITEMODE  (1 <<  1)           /* Write Mode */
#define     AT91_SMC_EXNWMODE   (3 <<  4)           /* NWAIT Mode */
#define     AT91_SMC_EXNWMODE_DISABLE   (0 << 4)
#define     AT91_SMC_EXNWMODE_FROZEN    (2 << 4)
#define     AT91_SMC_EXNWMODE_READY     (3 << 4)
#define     AT91_SMC_BAT        (1 <<  8)           /* Byte Access Type */
#define     AT91_SMC_BAT_SELECT     (0 << 8)
#define     AT91_SMC_BAT_WRITE      (1 << 8)
#define     AT91_SMC_DBW        (1 << 12)           /* Data Bus Width */
#define     AT91_SMC_DBW_8          (0 << 12)
#define     AT91_SMC_DBW_16         (1 << 12)
#define     AT91_SMC_TDFCYCLES      (0xf << 16)         /* Data Float Time. */
#define     AT91_SMC_TDFCYCLES_(x)      ((x) << 16)
#define     AT91_SMC_TDFMODE    (1 << 20)           /* TDF Optimization - Enabled */
#define     AT91_SMC_TDFMODE_(x)    ((x) << 20)         /* TDF Optimization - Enabled */


#ifdef __cplusplus
}
#endif

#endif
