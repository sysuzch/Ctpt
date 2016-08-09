#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/mman.h>

#include "smc_api.h"
#include <assert.h>

static int smc_fd = -1;

void fpga_reset( void )
{
    system("echo 0:0 >/sys/devices/user_gpios/user_gpios/pb30/ctrl");
    usleep(10000);
    system("echo 0:1 >/sys/devices/user_gpios/user_gpios/pb30/ctrl");
    usleep(10000);
}

/*
 * GPIO API
 * set gpio output or input
 * %label: gpio label, valid label please refer to devicetree
 * %direction: 0:output, 1:input
 * %value: output value,ignore it when set gpio as input
 */
int gpio_set(char *label, int direction, int value)
{
    int fd;
    struct user_gpio ug;

    fd = open(GPIO_NODE, O_WRONLY);
    if (fd < 0) {
        printf("open %s failed\n", GPIO_NODE);
        return -1;
    }

    ug.label = label;
    ug.type = direction;
    ug.value = value;

    if (write(fd, &ug, sizeof(ug)) != sizeof(ug)) {
        printf("user gpio write failed\n");
    }

    printf("set %s-->%d\n", ug.label, ug.value);

    close(fd);
    return 0;
}


/*
 * GPIO API
 * get gpio value
 * %label: gpio label, valid label please refer to devicetree
 */
int gpio_get(char *label)
{
    int fd;
    struct user_gpio ug;
    int value = -1;

    fd = open(GPIO_NODE, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", GPIO_NODE);
        return -1;
    }

    ug.label = label;
    ug.type = GPIO_DIRECTION_IN;

    if (read(fd, &ug, sizeof(ug)) != sizeof(ug)) {
        printf("user gpio read failed\n");
    }

    value = atoi((char *)&ug);
    printf("get %s<--%d\n", label, value);

    close(fd);
    return value;
}

/*
 * open smc device node
 * %dev:smc char device node full name
 */
int open_smc(const char* dev)
{
    int fd;

    dev = SMC_NODE;
    fd = open(dev, O_RDWR);
    assert(fd >=0);
    if (fd < 0) {
        printf("%s: open %s failed: %s\n", __func__, dev, strerror(errno));
        return -ENODEV;
    }

    smc_fd = fd;
    return fd;
}

/*
 * close smc device
 * %fd: file desc number
 */
int close_smc(int fd)
{
    int ret;

    assert(fd >=0);
    ret = close(fd);
    if (ret < 0) {
        printf("%s: close  error: %s\n", __func__, strerror(errno));
        return ret;
    }

    smc_fd = -1;

    return ret;
}

/*
 * write timing config for corresponding smc ncs
 * %c: smc timing configure struct to write
 * user should set c.ncs before invoking
 */
int write_smc_cfg(struct smc_config c)
{
    int ret;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    ret = ioctl(smc_fd, WR_HSMC_CFG, &c);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    return 0;
}

/*
 * read timing config for corresponding smc ncs
 * %c: smc timing configure struct to be filled
 * user should set c->ncs before invoking
 */
int read_smc_cfg(struct smc_config *c)
{
    int ret;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    ret = ioctl(smc_fd, WR_HSMC_CFG, c);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    return 0;
}

/*
 * write 4 byte %val to offset address %reg of %ncs
 * %ncs: smc ncs number
 * %reg: offset address of %ncs
 * %val: value to be write
 */
int writel_ncs(unsigned int ncs, unsigned int reg, unsigned int val)
{
    int ret;
    struct device_reg r;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    r.ncs = ncs;
    r.reg = reg;
    r.val = val;

    ret = ioctl(smc_fd, WR_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    return 0;
}

/*
 * read 4 byte from offset address %reg of %ncs
 * %ncs: smc ncs number
 * %reg: offset address of %ncs
 * %val: value to be filled
 */
int readl_ncs(unsigned int ncs, unsigned int reg, unsigned int *val)
{
    int ret;
    struct device_reg r;

    r.ncs = ncs;
    r.reg = reg;
    r.val = 0;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    ret = ioctl(smc_fd, RD_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    *val = r.val;

    return 0;
}

/*
 * write 4 byte %val to offset address %reg of control ncs0
 * %reg: offset address of ncs0
 * %val: value to be write
 */
int write_smc_dev_reg(unsigned int reg, unsigned int val)
{
    int ret;
    struct device_reg r;

    if(smc_fd < 0){
        printf("%s: smc device not open\n", __func__);
        return smc_fd;
    }

    r.ncs = CTRL_NCS0;
    r.reg = reg;
    r.val = val;

    ret = ioctl(smc_fd, WR_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    return 0;
}

/*
 * read 4 byte from offset address %reg of control ncs0
 * %reg: offset address of ncs0
 * %val: value to be filled
 */
int read_smc_dev_reg(unsigned int reg, unsigned int *val)
{
    int ret;
    struct device_reg r;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    r.ncs = CTRL_NCS0;
    r.reg = reg;
    r.val = 0;

    ret = ioctl(smc_fd, RD_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    *val = r.val;

    return 0;
}

/*
 * read 4 byte from sample offset address %reg of control ncs0
 * %reg: offset address of ncs0
 * %val: value to be filled
 */
int read_smc_dev_sreg(unsigned int reg,unsigned int *val)
{
    int ret;
    unsigned val_0;
    struct device_reg r;

    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    if(reg < 0x100 || reg > 0x1fc){
        printf("sample reg address illegal!");
        return -1;
    }

    r.ncs = CTRL_NCS0;
    r.reg = reg;
    r.val = 0;

    ret = ioctl(smc_fd, RD_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    val_0 = r.val;

    ret = ioctl(smc_fd, RD_DEVICE_REG, &r);
    if (ret < 0) {
        printf("%s: ioctl error: %s\n", __func__, strerror(errno));
        return ret;
    }

    *val = ((r.val<<16)&0xffff0000)|((val_0>>16)&0x0000ffff);

    return 0;
}

/*
 * set control registers to start sample
 */
int start_sample(void)
{
	int ret;
	
	/* clear driver buffer state */
	ret = ioctl(smc_fd, CL_BUF_STATE, 0);
	if (ret < 0) {
		printf("%s: ioctl error: %s\n", __func__, strerror(errno));
		/* return ret; */
	}
	
	/* enable ping buf and clear ping interrupt status */
	write_smc_dev_reg(PING_CTRL,0x00000133);
	
	/* enable pong buf and clear pong interrupt status */
	write_smc_dev_reg(PONG_CTRL,0x00000133);
	
	/* enable global interrupt */
	write_smc_dev_reg(INT_REG,0x00000001);
	
	return 0;
}

/*
 * set control registers to stop sample
 */
int stop_sample(void)
{
    /* disable global interrupt */
    write_smc_dev_reg(INT_REG,0x00000000);

    /* disable ping buf and clear ping interrupt status */
    write_smc_dev_reg(PING_CTRL,0x00000033);

    /* disable pong buf and clear pong interrupt status */
    write_smc_dev_reg(PONG_CTRL,0x00000033);

    printf("stop_sample\n");
    return 0;
}


/*
 * read data buf from data ncs1
 * %buf: buffer to filled with sample data
 * %count: buf length
 */
int read_sample_buf(unsigned char* buf, int count)
{
    if(smc_fd < 0){
        printf("%s: smc device not open", __func__);
        return smc_fd;
    }

    return read(smc_fd,(void*)buf, count);
}

static int g_fd;

int lowlevel_init( void )
{
    struct smc_config c;

    c.ncs = 0;

    /* Setup register */
    c.ncs_read_setup = 2;
    c.nrd_setup = 2;
    c.ncs_write_setup = 2;
    c.nwe_setup = 2;

    /* Pulse register */
    c.ncs_read_pulse = 6;
    c.nrd_pulse = 4;
    c.ncs_write_pulse = 4;
    c.nwe_pulse = 4;

    /* Cycle register */
    c.read_cycle = 0xa;
    c.write_cycle = 0x8;

    /* Mode register */
    c.tdf_mode = 1;
    c.tdf_cycles = 2;
    c.mode = 3;

    g_fd = open_smc(SMC_NODE);
    if(g_fd < 0){
        printf("smc device open failed.\n");
        return g_fd;
    }

    write_smc_cfg(c);

    c.ncs = 1;
    write_smc_cfg(c);

    c.ncs = 2;
    write_smc_cfg(c);

    return 0;
}

void lowlevel_exit( void )
{
    if ( g_fd >= 0 )
    {
        close_smc(g_fd);
        g_fd = -1;
    }
}

#if 0
int main(int argc, char* argv[])
{
    int fd,ret;
    int i,mode=0;
    unsigned int ncs,reg,val,d;
    struct smc_config c;

    c.ncs = 0;

    /* Setup register */
    c.ncs_read_setup = 2;
    c.nrd_setup = 2;
    c.ncs_write_setup = 2;
    c.nwe_setup = 2;

    /* Pulse register */
    c.ncs_read_pulse = 6;
    c.nrd_pulse = 4;
    c.ncs_write_pulse = 4;
    c.nwe_pulse = 4;

    /* Cycle register */
    c.read_cycle = 0xa;
    c.write_cycle = 0x8;

    /* Mode register */
    c.tdf_mode = 1;
    c.tdf_cycles = 2;
    c.mode = 3;

    fd = open_smc(SMC_NODE);
    if(fd < 0){
        printf("smc device open failed.\n");
        return fd;
    }

    printf("\nwrite smc config to NCS0 & NCS1\n");
    write_smc_cfg(c);

    c.ncs = 1;
    write_smc_cfg(c);

    c.ncs = 2;
    write_smc_cfg(c);

    if(argv[1]) {
        mode = atoi(argv[1]);

        printf("test mode: %d\n",mode);
        if(mode == 0){
            for(i=0; i < 0x80000; i++){
                val = random();
                ncs = 0;
                reg = (val/4)&0x7fff;
                writel_ncs(ncs,reg, val);
                ret = readl_ncs(ncs,reg,&d);
                if(ret >= 0){
                    if(val != d)
                    printf("ERROR[%06d]: NCS=%d, REG=%08x, WR_VAL=%08x, RD_VAL=%08x\n", i,ncs,reg,val,d);
                }
                else
                printf("smc dev reg read failed.\n");
            }
            printf("smc ncs0 random test OK.\n");
        }
        else if(mode == 1) {
            for(i=0; i < 0x80000; i++){
                val = random();
                ncs = 0x01;
                reg = (val/4)&0x7fff;
                writel_ncs(ncs,reg, val);
                ret = readl_ncs(ncs,reg,&d);
                if(ret >= 0){
                    if(val != d)
                    printf("ERROR[%06d]: NCS=%d, REG=%08x, WR_VAL=%08x, RD_VAL=%08x\n", i,ncs,reg,val,d);
                }
                else
                printf("smc dev read reg failed.\n");
            }
            printf("smc ncs1 random test OK.\n");
        }
        else if(mode == 2) {
            for(i=0; i < 0x80000; i++){
                val = random();
                ncs = val&0x01;
                reg = (val/4)&0x7fff;
                writel_ncs(ncs,reg, val);
                ret = readl_ncs(ncs,reg,&d);
                if(ret >= 0){
                    if(val != d)
                    printf("ERROR[%06d]: NCS=%d, REG=%08x, WR_VAL=%08x, RD_VAL=%08x\n", i,ncs,reg,val,d);
                }
                else
                printf("smc ncs0/ncs1 mix test OK.\n");
            }
            printf("cs0/cs1 random test OK.\n");
        }
    }
    else {
        printf("\nfpga test register write read test:\n");
        printf("\n----------write---------\n");
        do {
            for(i=0; i < 8; i++){
                reg = 0xe0+i*4;
                val = reg|(reg<<8)|(reg<<16)|(reg<<24);
                //printf("%08x ",val|(val<<8)|(val<<16)|(val<<24));
                write_smc_dev_reg(reg, val);
                ret = read_smc_dev_reg(reg,&d);
                if(ret >= 0){
                    if(val != d)
                    printf("ERROR[%06d]: REG=%08x, WR_VAL=%08x, RD_VAL=%08x\n", i,reg,val,d);
                }
                else
                printf("reg read failed.\n");
            }
            usleep(5000);
        }while(0);
#if 1
        printf("\n----------read---------\n");
        for(i=0; i < 8; i++){
            ret = read_smc_dev_reg(0xe0+i*4, &val);
            printf("%08x ",val);
        }
        printf("\n----------end---------\n");
#endif
    }

    close_smc(fd);
    return 0;
}

#endif
