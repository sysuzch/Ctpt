
#ifndef EXCITING_SCANNER_H
#define EXCITING_SCANNER_H

#include <vector>
#include "raw_wave.hpp"


class data_source;

class exciting_scanner
{
public:
    exciting_scanner();
    virtual ~exciting_scanner();

    void register_data_source( data_source *ds ) { this->ds = ds;}

public:
    // 扫苗励磁表格
    struct exciting_ui
    {
        // 预置值
        double preset_vol;
        double preset_cur;
        double preset_freq;

        // input 测量值
        raw_wave messured_vol;
        raw_wave messured_cur;
        double   messured_freq;

        // 计算结果
        double angle;// 磁电压与电流的相位差
    };

    bool exciting_scan( double fs, int np, double output_voltage, double freq_start, double freq_end, double freq_step );

    bool find_knee_point( int *knee_index );

private:
    data_source *ds;

    std::vector<struct exciting_ui> exciting;

    void calculate_exciting_item( struct exciting_ui &item );
    int find_near_voltage( double voltage );

    int exciting_knee_point_index;
};


#endif /* EXCITING_SCANNER_H */
