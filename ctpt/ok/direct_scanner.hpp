
#ifndef DIRECT_SCANNER_H
#define DIRECT_SCANNER_H

#include <vector>
#include "raw_wave.hpp"

class data_source;

class direct_scanner
{
public:
    direct_scanner();
    virtual ~direct_scanner();

    void register_data_source( data_source *ds ) { this->ds = ds;}


public:
    struct direct_ui
    {
        // 预置值
        double preset_vol;
        double preset_cur;
        double preset_freq;

        // input 测量值
        raw_wave in_vol;
        raw_wave in_cur;
        raw_wave out_vol;
        raw_wave out_cur;
        double in_freq;

        // 计算结果
        double c_current_error;// 复合误差电流曲线
        double in_voltage_current_angle; // 电压电流相位差
        double out_voltage_current_angle;// 电压电流相位差
        double io_voltage_angle;         // 出电压相位差
        double io_current_angle;         // 出电流相位差
    };

    bool direct_scan_fix_voltage( double fs, int np, double fix_voltage, double freq_start, double freq_end, double freq_step );
    bool direct_scan_fix_freq( double fs, int np, double fix_voltage, double freq_start, double freq_end, double freq_step );

private:
    void calculate_direct_item( struct direct_ui  &item );
    // direct scan
    std::vector<struct direct_ui> directing;
    data_source *ds;

};



#endif /* DIRECT_SCANNER_H */