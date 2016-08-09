
#ifndef DATA_SOURCE_FPGA_H
#define DATA_SOURCE_FPGA_H


#include "data_source.hpp"
#include "fpga.hpp"

#define NPOINT_SCAN 100

class data_source_fpga:public data_source
{
public:
    data_source_fpga();

    void init(void);

    virtual ~data_source_fpga();

    bool auto_scan_prepare( const struct ct_setting & );
    bool auto_scan_prepare( const struct pt_setting & );

    bool auto_scan_reset( void );
    bool auto_scan_step( struct auto_scan_result &result );
    bool auto_scan_next( void );
    void set_listener( rt_listener *listener );

private:
    int mode;
    void process(int percent);
    void scanning(double voltage, double current);
    struct ct_setting ct_setting;
    struct pt_setting pt_setting;
    rt_listener *listener;

    fpga_dev fpga;

    int seq;

    double voltage[NPOINT_SCAN];
};



#endif /* DATA_SOURCE_FPGA_H */
