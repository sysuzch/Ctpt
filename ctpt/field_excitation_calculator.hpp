/**
 * @file   field_excitation_calculator.hpp
 * @brief  励磁参数计算
 *
 * 励磁电流测量方法:
 *     一次端开路，给二次施加电压得到的电流.
 *
 * @author
 * @date   2015-06-05
 */
#ifndef FIELD_EXCITATION_CALCULATOR_H
#define FIELD_EXCITATION_CALCULATOR_H

#include <vector>
#include <complex>
#include <math.h>

#include "calculator.hpp"
#include "data_source.hpp"


struct field_paramters
{
    // 拐点 电压 和 拐点电流
    // IEC 6044-1励磁曲线上当二次端子电压上升10%导致二次电流上升50%的点
    // double exciting_voltage;  // V
    // double exciting_current;  // A

    double inductance_unsaturated; // 不饱和电感

    double kr;                  // 剩磁系数Kr

    double ts;                  // 二次时间常数, L/R
    double e_al;                 // 极限电动势
    double err_al;               // 复合误差
    double err;                  // 峰瞬误差
    // 准确限值系数
    double sx;

    /// @brief 仪表保安系数
    ///
    /// 仪表保安系数的验证可用下述间接接进行:
    /// 在一次绕组开路的情况下，对二次绕组施加额定频率的实际正弦波电压，其方均根值等于二次极限感应电势时，测量励磁电流。
    /// @brief 暂态面积系数
    double kare_static;

    // 计算系数
    double kx;

    // 额定拐点电势
    double e_k;

    // Ek 对应的IE
    double ie_ek;

    // 额定Ual
    double u_al;

    // Ual对应的Ial
    double i_al_u_al;


    // 励磁特性曲线
    raw_wave *curve_voltage;
    raw_wave *curve_current;
    double    system_freq;         // system frequency

    // GT1208-2006
    double rated_primary_current; // 额定一次电流, in
    double rated_secondary_current; // 额定二次电流, in
    double actual_transformation_ratio; // 实际电流比, out, --ok
    double rated_transformation_ratio;  // 额定电流比, in
    double current_error;               // 电流误差(比值差), out,
    double phase_displacement;          // 相位差, out
    int    accuracy_class;              // 准确级, in
    double burden;                      // 二次负荷, in
    double rated_burden;                // 额定负荷, in
    double rated_output;                // 额定输出,  in
    double highest_voltage_for_equipment;    // 设备最高电压, in
    double highest_voltage_for_a_system;     // 系统最高电压, in
    double rated_insulation_level;           // 额定绝缘水平, in
    // ....
    double rated_frequency;                  // 额定频率, in
    double rated_short_time_thermal_current; // 额定短时热电流, ??
    double rated_dynamic_current;            // 额定动稳定电流, in
    double rate_continous_thermal_current;   // 额定连续热电流, in
    double exciting_current;                 // 励磁电流, ---
    double rated_resitive_burden;            // 额定电阻负荷, in
    double secondary_winding_resistance;     // 二次绕组电阻, out  --ok
    double composite_error;                  // 复合误差, out,

    double instrument_security_factor; // 仪表保安系数, out
    double secondary_limiting_emf;     // 二次极限感应电势, out
    double rated_accuracy_limit_primary_current; // 额定准确限值一次电流, out
    double accuracy_limit_factor;                // 准确限值系数 ALF, out

    double saturation_flux;     // 饱和磁通, out
    double remanent_flux;       // 剩磁通,  out
    double remanence_factor;    // 剩磁系数, out
    double rated_secondary_loop_time_constant; // 额定二次回路时间常数 Ts = Ls / Rs, out

    // 励磁特性 excitation characteristic
    double rated_knee_point_emf; // 额定拐点电势, Ek, out
    double rated_turns_ratio;    // 额定匝数比, out
    double turns_ratio_error;    // 匝数比误差, out
    double dimensioning_factor;  // 计算系数， Kx, out
};




class calculator_field_excitation: public calculator
{
public:
    struct field_paramters parameters;


public:
    typedef struct exciting_ui exciting_t;

    double voltage_ratio;      // ratio of measure voltage to system voltage.

public:
    calculator_field_excitation();
    virtual ~calculator_field_excitation();

    virtual int input( raw_wave *a, raw_wave *b, raw_wave *c = 0, raw_wave *d = 0 );

    void calculate_current_error( std::complex<double> z, std::complex<double> r, double i_ct_abs );
    void calculate_fs( void );

    // fs 要检查的fs,
    // i_exc 基方根值等于二次极限感应电势时的励磁电流
    // i_sn 额定二次电流
    bool check_fs( double fs, double i_exc, double i_sn );

    double get_n_turns( void );
    double get_n_rated( void );
    bool get_i_ex_from_vol( double vc, double *i_ex, double *anglev );

protected:
    double resistance( const raw_wave &vol, const raw_wave &cur );
    double resistance_fix( double r, double temp_ref, double t_measured );
    void do_exciting( const raw_wave &vol, const raw_wave &cur );

private:
    int exciting_knee_point_index;

};



#endif /* FIELD_EXCITATION_CALCULATOR_H */
