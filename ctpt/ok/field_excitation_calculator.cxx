/**
 * @file   field_excitation_calculator.cxx
 * @brief
 *
 * <long description>
 *
 *　NOTE:
 *　CT二次空载伏安特性测试试验接线如图所示：
 *　CT一次侧开路，二次侧一组绕组施加交流电压，以电流读数为准，读取电压值，
 *　其它绕组处于开路状态，如此对每一组二次进行试验，也可以采用其它方法，当测量电压高
 *　于二次绕组工频耐压值时，须用低频率的试验电源，但必须折算到工频电压数值。
 * 试验时注意保持电压均匀上升，不得来回摆动，否则剩磁的存在将严重影响测量结果。
 * 一旦出现此种情况，应重新升压，将电压调至比原加电压稍高时再均匀降至零，
 * 重复１~2次即可消除CT剩磁的影响。测得多饱和电流约为10%额定电流的电压
  *
 * 电力互感器综合特性测试
 *
 * 
 * @author
 * @date   2015-05-31
 */

#include <stdlib.h>
#include <string.h>
#include "field_excitation_calculator.hpp"
#include "raw_wave.hpp"
#include <assert.h>
#include <math.h>
#include <iostream>
#include "ctpt_api.hpp"

ctpt_ctrl ctr;

static void _set_inf( double *p, double *end )
{
    double inf = 0.0/0.0;
    for (; p<=end; p++)
    {
        *p++ = inf;
    }
}

static bool _check_field( double *p, double *end, int &start )
{
    double *o = p;
    for (; p<end; p++) {
        double t = *p;
        if ( !std::isfinite(t) )
        {
            std::cout << t << " @" << start+ (p-o) << std::endl;
            return false;
        }
    }
    start += p-o;
    return true;
}

static void _check_all( struct field_paramters *p )
{
    int start = 0;
    assert( _check_field( &p->inductance_unsaturated, &p->i_al_u_al, start ) );
    assert( _check_field( &p->rated_primary_current, &p->phase_displacement, start ) );
    assert( _check_field( &p->burden, &p->dimensioning_factor, start ) );
}

calculator_field_excitation::calculator_field_excitation():
    calculator("field_excitation_calculator")
{
    memset( &this->parameters, 0x00, sizeof(this->parameters) );

    _set_inf( &this->parameters.inductance_unsaturated, &this->parameters.i_al_u_al );
    _set_inf( &this->parameters.rated_primary_current, &this->parameters.phase_displacement );
    _set_inf( &this->parameters.burden, &this->parameters.dimensioning_factor );
}

int calculator_field_excitation::input( raw_wave *i_vol, raw_wave *i_cur, raw_wave *o_vol, raw_wave *o_cur )
{
    struct field_paramters *o = &this->parameters;


    double rms_in_vol;
    double rms_in_cur;
    double rms_out_vol;
    double rms_out_cur;

    double rms_in_vol2;
    double rms_in_cur2;

    rms_in_vol = i_vol->get_rms();
    rms_in_cur = i_cur->get_rms();
    rms_out_vol = o_vol->get_rms();
    rms_out_cur = o_cur->get_rms();

    rms_out_vol = o_vol->get_rms();
    rms_out_cur = o_cur->get_rms();

    rms_in_cur2 = rms_in_cur * 1.0;

    o->actual_transformation_ratio = rms_in_cur / rms_in_cur2;

    return 0;
}

void calculator_field_excitation::do_exciting( const raw_wave &vol, const raw_wave &cur )
{
    struct field_paramters *o = &this->parameters;
    // 测量励磁
    // 励磁曲线
    o->curve_voltage = new raw_wave( vol );
    o->curve_current = new raw_wave( cur );
    assert( o->curve_voltage );
    assert( o->curve_current );

    double freq = vol.get_freq();
    this->voltage_ratio = o->system_freq/freq;
    o->curve_voltage->integral( this->voltage_ratio ); // 电压积分
}


double calculator_field_excitation::resistance( const raw_wave &vol, const raw_wave &cur )
{
    double rms_vol;
    double rms_cur;
    int np;
    double active_power;
    double r;

    rms_vol = vol.get_rms();
    rms_cur = cur.get_rms();

    np = vol.get_np_ncycled();
    active_power =  vol.get_multiply( cur, np ) / np;

    // P = U^2 / R
    // P = I^2 * R
    r = active_power / ( rms_cur * rms_cur );

    this->parameters.secondary_winding_resistance = r;
    this->parameters.secondary_limiting_emf = r;
    return r;
}



void calculator_field_excitation::calculate_current_error( std::complex<double> z, std::complex<double> r, double i_ct_abs )
{
    // 已知量
    double n_turns = this->get_n_turns();
    double n_rated = this->get_n_rated();

    // 负载主抗的实部和虚部
    double rb = z.real();
    double ib = z.imag();

    // 总负载阻抗的计算
    std::complex<double> zload = z + r;
    double  zload_abs = std::abs( zload );
    double gama = std::arg(zload);

    // 给定总的二次电流: 参数传入

    // 计算二次励磁电压
    double vc = i_ct_abs * zload_abs;

    // 在励磁表格上查找对应于这个vc电压和相位v的励磁电流
    double i_ex_abs = 0;
    double v = 0;
    std::complex<double> i_ex = std::polar( i_ex_abs, 0.0 );
    if ( !get_i_ex_from_vol( vc, &i_ex_abs, &v ) )
    {
        return ;
    }

    // 计算二次负载电流和励磁电流间的相位
    double beta = v - gama;

    // 计算总二次电流
    double i_st_abs;
    double t1, t2;
    t1 = i_ct_abs + i_ex_abs * cos( beta );
    t2 = i_ex_abs * sin( beta );
    i_st_abs = sqrt( (t1*t1) + (t2*t2) );

    // 计算一次电流
    double i_p_abs = i_st_abs * n_turns;

    // 计算比差
    double err_ratio = ( i_ct_abs - i_p_abs/n_rated ) / ( i_p_abs/n_rated );

    // 计算Ist和Ict间相位差
    double theta = atan( i_ex_abs * sin(beta) / ( i_ct_abs + i_ex_abs*cos(beta) ) );
}

void calculator_field_excitation::calculate_fs( void )
{
    double alpha;
    double i_s;
    double i_p;
    double n;

    double i_psc;
    double i_pn;

    double i_c;

    double a = i_s;
    double b = i_p / n;
    i_c = sqrt( a*a + b*b - 2 * a * b * cos(alpha) );

}


double calculator_field_excitation::resistance_fix( double r, double temp_ref, double t_measured  )
{
    return r * ( 234.5 + temp_ref ) / ( 234.5 + t_measured );
}


#include <stdint.h>


struct raw_frame
{
    int16_t time_us;            // in us
    int16_t reserved;            // in us
    int16_t raw_data[4];
};


struct raw_registers
{
    int16_t reg_freq_high;
    int16_t reg_freq_low;
    int16_t reg_range[4];
    struct raw_frame reg_frames[16384/sizeof(struct raw_frame)];
};




