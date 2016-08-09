// implement test logic
//
// 处理接线方式时分成两种：
//
// 1. 二次输入电压，测量：二次电压电流，一次电压
// 2. 负载电压.
//
// 3. 比差定义:
// 由输入电流Ip
//
#include <complex>
#include <assert.h>

#include <stdlib.h>
#include <string.h>             // for memset
#include <malloc.h>

#include <algorithm>

#include <vector>
#include "ctpt_api.hpp"
#include "data_source.hpp"

#include <sstream>
#include <iostream>

// std::stringstream g_ctpt_log;
#ifndef NO_LOG
#define g_ctpt_log std::cout
#else
std::stringstream g_ctpt_log;
#endif

class ctpt_ctrl_private
{
public:
    ctpt_ctrl_private();
    virtual ~ctpt_ctrl_private();


public:
    /*
     * 测量励磁
     */
    void run_test( void );

    bool find_knee_point( struct auto_scan_result &r );

    void calculate_resistance( void );
    void calculate_all( void );
    void copy_exciting( void );

    void register_data_source( data_source *ds ) {this->ds = ds;}

    void calculate_ct_ratio( void );
    bool calculate_inductance( void );
    bool calculate_inductance_unsaturated( void );
    bool calculate_fs( void );

    // 电压源转电流源算法,　扫描输出，查找合适的电压
    bool auto_setup_output( double &vol, double freq, double vol_min, double current_max, double current_need );

    void calculate_burden( void );

    bool get_i_ex_from_vol( std::complex<double> &cur, const std::complex<double> &vol );

    double current_error( std::complex<double> z, // 负载
                          std::complex<double> r, // 内阻
                          double i_ct_abs,        //
                          double n_turns,
                          double n_rated );

    double current_error_vector( std::complex<double> z, // 负载
                                 std::complex<double> r, // 内阻
                                 std::complex<double> i_ct,        //
                                 double n_turns,
                                 double n_rated );

public:
    rt_listener *listener;
    std::vector<struct auto_scan_result> scanning_result; // exciting
    struct auto_scan_result *exciting;
    struct auto_scan_result *ratio;
    raw_wave ratio_vol_primary;
    raw_wave ratio_vol_secondary;
    data_source *ds;
    int    knee_index;
    double vol_preknee;
    double cur_preknee;
    double vol_knee;
    double cur_knee;
    struct ctpt_result result;

    //
    std::complex<double> zburden;
    std::complex<double> zresistance;

    // config
    struct ct_setting ct_setting;
    struct pt_setting pt_setting;

    bool is_ct_mode;
};

ctpt_result::ctpt_result()
{
    int np = 100;

    this->exciting_current   = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->exciting_voltage   = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->error5_resistance  = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->error5_factor      = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->error10_resistance = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->error10_factor     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
}


ctpt_result::~ctpt_result()
{
    if ( this->exciting_current )
    {
        free( this->exciting_current );
        this->exciting_current = 0;
    }
    if ( this->exciting_voltage )
    {
        free( this->exciting_voltage );
        this->exciting_voltage = 0;
    }
    if ( this->error10_factor )
    {
        free( this->error10_factor );
        this->error10_factor = 0;
    }
    if ( this->error5_factor )
    {
        free( this->error5_factor );
        this->error5_factor = 0;
    }
    if ( this->error10_resistance )
    {
        free( this->error10_resistance );
        this->error10_resistance = 0;
    }
    if ( this->error5_resistance )
    {
        free( this->error5_resistance );
        this->error5_resistance =0;
    }

}


ctpt_ctrl_private::ctpt_ctrl_private():
    listener(0),
    exciting(0),
    ratio(0),
    ds(0),
    knee_index(-1),
    vol_preknee(0),
    cur_preknee(0),
    vol_knee(0),
    cur_knee(0),
    is_ct_mode(true)
{

    memset( &result, 0x00, sizeof(result) );
    result.exciting_current   = 0;
    result.exciting_voltage   = 0;
    result.error10_factor     = 0;
    result.error5_factor      = 0;
    result.error10_resistance = 0;
    result.error5_resistance  = 0;

    g_ctpt_log << "inited\n";
}

// 根据电压向量查表得到历次电流,　励磁电流的角度已经被修正
bool ctpt_ctrl_private::get_i_ex_from_vol( std::complex<double> &cur, const std::complex<double> &vol )
{
    bool ok = true;
    int n;

    if ( !this->exciting )
    {
        return false;
    }
    n = this->exciting->wave_vol.size();
    double cur_prev = this->exciting->wave_cur[0].get_rms();
    std::complex<double> cur_prev_vector = this->exciting->wave_cur[0].get_w();
    double vol_ct = std::abs( vol );
    double vol_angle = std::arg( vol );
    for (int i=0; i<n; i++)
    {
        double vol_next;
        double cur_next;
        std::complex<double> cur_vector = this->exciting->wave_cur[i].get_w();
        vol_next = this->exciting->wave_vol[i].get_rms();
        cur_next = this->exciting->wave_cur[i].get_rms();

        if ( vol_next > vol_ct  )
        {
            double cur_abs = (cur_next + cur_prev)/2;
            double angle = ( std::arg(cur_vector) + std::arg(cur_prev_vector) )/2;
            cur = std::polar( cur_abs, angle+vol_angle );
            return true;
        }
        cur_prev = cur_next;
        cur_prev_vector = cur_vector;
    }

    return ok;
}


ctpt_ctrl_private::~ctpt_ctrl_private()
{
    result.exciting_current   = 0;
    result.exciting_voltage   = 0;
    result.error10_factor     = 0;
    result.error5_factor      = 0;
    result.error10_resistance = 0;
    result.error5_resistance  = 0;
}


ctpt_ctrl::ctpt_ctrl()
{
    this->_private = new ctpt_ctrl_private;
}


ctpt_ctrl::~ctpt_ctrl()
{
    if ( this->_private )
    {
        delete this->_private;
        this->_private = NULL;
    }
}


bool ctpt_ctrl_private::find_knee_point( struct auto_scan_result &r )
{
    int i;
    int n_vol = 0;
    int n_cur = 0;
    int n = 0;

    knee_index = -1;
    std::vector<raw_wave>::iterator it;
    n_vol = r.wave_vol.size();
    n_cur = r.wave_cur.size();

    assert( n_vol == n_cur );
    n = std::min( n_vol, n_cur );

    if ( n < 1 )
    {
        assert( 0 && "never be n<1 ");
        return false;
    }

    double pre_vol;
    double pre_cur;
    pre_vol = r.wave_vol[0].get_rms();
    pre_cur = r.wave_cur[0].get_rms();

    for (i = 1; i < n; i++)
    {
        double k1, k2;
        double vol, cur;
        vol = r.wave_vol[i].get_rms();
        cur = r.wave_cur[i].get_rms();
        k1 = pre_cur / pre_vol;
        k2 = cur / vol;
        double incr = k2 / k1;
        if ( incr >= 1.5/1.1 )
        {
            // ok, found
            this->knee_index = i;
            this->vol_preknee = pre_vol;
            this->cur_preknee = pre_cur;
            this->vol_knee = vol;
            this->cur_knee = cur;
            return true;
        }
        pre_vol = vol;
        pre_cur = cur;
    }

    return false;
}


void ctpt_ctrl_private::run_test( void )
{
    int prepare_ok;
    // select range.
    scanning_result.clear();
    if (this->is_ct_mode){
        prepare_ok = ds->auto_scan_prepare(this->ct_setting);
    }else{
        prepare_ok = ds->auto_scan_prepare(this->pt_setting);
    }
    if ( !prepare_ok ) {
        g_ctpt_log << "prepare failed.\n";
        return ;
    }

    while ( 1 )
    {
        bool ok;

        struct auto_scan_result asr;
        scanning_result.push_back( asr );
        struct auto_scan_result &r = scanning_result.back();
        ok = ds->auto_scan_step( r );
        if ( !ok )
        {
            break;
        }
        if ( this->listener )
        {
            double voltage;
            double current;
            int s;

            s = r.wave_vol.size();
            if ( s )
            {
                voltage = r.wave_vol[s-1].get_rms();
            }

            s = r.wave_cur.size();
            if ( s )
            {
                current = r.wave_cur[s-1].get_rms();
            }
            this->listener->scanning( voltage, current );
        }

        if ( find_knee_point( r ) )
        {
            this->exciting = &r;
            /*
             * TODO: 多测一些
             */
            break;
        }

        /*
         * 调整电压和频率重新扫描
         */
    }

    // step by step
    calculate_resistance();
}

static double resistance( const raw_wave &vol, const raw_wave &cur )
{
    // double rms_vol;
    double rms_cur;
    int np;
    double active_power;
    double r;

    // rms_vol = vol.get_rms();
    rms_cur = cur.get_rms();

    np = vol.get_np_ncycled();
    active_power =  vol.get_multiply( cur, np ) / np;

    // P = U^2 / R
    // P = I^2 * R
    r = active_power / ( rms_cur * rms_cur );

    return r;
}

void ctpt_ctrl_private::calculate_resistance( void )
{
    double sum = 0;
    int i;
    int n;

    n = this->knee_index;
    for (i = 0; i<n; i++)
    {
        double r;
        raw_wave &vol = exciting->wave_vol[i];
        raw_wave &cur = exciting->wave_cur[i];
        r = resistance( vol, cur );
        sum += r;
    }
    double res;
    res = sum / n;
    result.secondary_winding_resistance = res;
    result.rated_resitive_burden = ct_setting.rated_burden;
}


double ctpt_ctrl_private::current_error( std::complex<double> z, // 负载
                                         std::complex<double> r, // 内阻
                                         double i_ct_abs,        //
                                         double n_turns,
                                         double n_rated )
{
    // 负载主抗的实部和虚部
    // double rb = z.real();
    // double ib = z.imag();

    // 总负载阻抗的计算
    std::complex<double> zload = z + r;
    // double  zload_abs = std::abs( zload );
    double gama = std::arg(zload);

    // 给定总的二次电流: 参数传入

    // 计算二次励磁电压
    // double vc = i_ct_abs * zload_abs;

    // 在励磁表格上查找对应于这个vc电压和相位v的励磁电流
    double i_ex_abs = 0;
    double v = 0;
    // std::complex<double> i_ex = std::polar( i_ex_abs, 0.0 );

    // if ( !get_i_ex_from_vol( vc, &i_ex_abs, &v ) )
    // {
    //     return ;
    // }

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
    // double theta = atan( i_ex_abs * sin(beta) / ( i_ct_abs + i_ex_abs*cos(beta) ) );
    return err_ratio;
}


double ctpt_ctrl_private::current_error_vector( std::complex<double> z, // 负载
                                                std::complex<double> r, // 内阻
                                                std::complex<double> i_ct,        //
                                                double n_turns,
                                                double n_rated )
{
    // 负载主抗的实部和虚部
    // double rb = z.real();
    // double ib = z.imag();
    double i_ct_abs = std::abs(i_ct);

    // 总负载阻抗的计算
    std::complex<double> zload = z + r;
    // double  zload_abs = std::abs( zload );
    // double gama = std::arg(zload);

    // 给定总的二次电流: 参数传入

    // 计算二次励磁电压
    std::complex<double> vc = i_ct * zload;

    // 在励磁表格上查找对应于这个vc电压和相位的励磁电流 i_ex
    std::complex<double> i_ex;

    if ( !get_i_ex_from_vol( i_ex, vc ) )
    {
        g_ctpt_log << "ERROR: exciting not ready.\n";
        return 0;
    }

    // 计算二次负载电流和励磁电流间的相位
    // double beta = v - gama;

    // 计算总二次电流
    std::complex<double> i_st;

    i_st = i_ex + i_ct;

    double i_st_abs = std::abs(i_st);

    // 计算一次电流
    double i_p_abs = i_st_abs * n_turns;

    // 计算比差
    double err_ratio = ( i_ct_abs - i_p_abs/n_rated ) / ( i_p_abs/n_rated );

    // 计算Ist和Ict间相位差
    // double theta = atan( i_ex_abs * sin(beta) / ( i_ct_abs + i_ex_abs*cos(beta) ) );
    return err_ratio;
}

bool ctpt_ctrl_private::calculate_inductance( void )
{
    double inductance = 0;
    double phy[3];
    double cur[3];

    int np_vol = this->exciting->wave_vol.size();
    int np_cur = this->exciting->wave_vol.size();


    if ( np_vol != np_cur )
    {
        g_ctpt_log << "np_vol != np_cur:" << np_vol << " vs " << np_cur << "\n";
        return false;
    }
    int len = np_vol - this->knee_index;
    if ( len <= 0 )
    {
        g_ctpt_log << "len <= 0:this->knee_index=" << this->knee_index << "\n";
        return false;
    }
    int step = len / 3;
    int np = np_vol;
    if ( step == 0 )
    {
        g_ctpt_log << "step == 0:this->knee_index=" << this->knee_index << "\n";
        return false;
    }
    phy[0] = this->exciting->wave_vol[np-1].get_integral_rms();
    phy[1] = this->exciting->wave_vol[np-1-step].get_integral_rms();
    phy[2] = this->exciting->wave_vol[np-1-step*2].get_integral_rms();

    cur[0] = this->exciting->wave_cur[np-1].get_rms();
    cur[1] = this->exciting->wave_cur[np-1-step].get_rms();
    cur[2] = this->exciting->wave_cur[np-1-step*2].get_rms();

    double L12 = ( phy[0] - phy[1] )/ (cur[0] - cur[1]);
    double L23 = ( phy[1] - phy[2] )/ (cur[1] - cur[2]);
    double L13 = ( phy[0] - phy[2] )/ (cur[0] - cur[2]);

    bool ok = true;
    if ( cur[0] > 5*cur_knee
         || (0.6 < L12/L23 && L12/L23<1.4)
         || L13 < 30.0e-3
        )
    {
        ok = false;
    }
    result.inductance = inductance; // 饱和电感
    return ok;
}

bool ctpt_ctrl_private::calculate_inductance_unsaturated( void )
{
    bool ok = true;
    int start_index = this->knee_index * 0.2;
    int stop_index = this->knee_index * 0.9;
    double sum = 0.0;
    int count = 0;

    for (int i=start_index; i<=stop_index; i++, count++)
    {
        double phy = this->exciting->wave_vol[start_index].get_integral_rms();
        double cur = this->exciting->wave_cur[start_index].get_rms();
        double Lm = phy / cur;
        sum += Lm;
    }
    if ( count == 0 )
    {
        this->result.inductance_unsaturated = 0;
        return false;
    }
    this->result.inductance_unsaturated = sum / count;
    return ok;
}

void ctpt_ctrl_private::calculate_all( void )
{
    result.knee_point_voltage = this->vol_knee; // 拐点电压
    result.knee_point_current = this->cur_knee; // 拐点电流

    // 二次极限感应电势, 依赖: 负载阻抗、内阻、保安系数和二次额定电流
    double fs = result.instrument_security_factor;            // FIXME: set fs
    result.secondary_limiting_emf = fs * ( std::abs( this->zburden + this->zresistance ) * this->ct_setting.rated_secondary_current );

    // result.inductance = -1; // 饱和电感
    // result.inductance_unsaturated = -1; // 不饱和电感
    result.rated_secondary_loop_time_constant = -1; // 额定二次回路时间常数 Ts = Ls / Rs, out
    result.composite_error = -1;        // 复合误差
    result.remanence_factor = -1;    // 剩磁系数,
    result.accuracy_limit_factor = -1; // P,PR 准确限值系数
    result.rated_knee_point_emf = -1; // PX 额定拐点电势, Ek
    result.rated_knee_point_current = -1; // PX 额定拐点电势, ie_ek
    result.kare_static = -1;         // TPY，TPX,TPZ 暂态面积系数
    result.kssc = -1; // TPY，TPS,TPX,TPZ 所得励磁电流I_exc与额定二次电流I_sn和仪表保安系数FS乘积之比用百分数表示时，其值应等于或者大于额定复合误差限值，即1-1%
    result.peek_error = -1;          // TPY，TPX,TPZ 峰瞬误差
    result.instrument_security_factor = -1; // 计量类/measured 仪表保安系数
    result.dimensioning_factor = -1;  // 计算系数， PX
    result.u_al = -1;    // TPS 额定Ual
    result.i_al_u_al = -1;// TPS Ual对应的Ial

    // 变比
    result.ratio = -1;               // 变比
    result.ratio_error = -1;         // 比值差
    result.turns_ratio = -1;    // 额定匝数比,
    result.turns_ratio_error = -1;    // 匝数比误差
    result.phase_error = -1;          // 相位差
    result.phase_polar = -1;          // 极性

    // 励磁曲线
    copy_exciting();

    // result.exciting_current = -1;   // x coordinate
    // result.exciting_voltage = -1;   // y coordinate
    // 5%误差曲线
    result.npoint_error5 = -1;
    // result.error5_resistance = -1;  // 负载电阻
    // result.error5_factor = -1;      // 5%误差限值
    // 1-1%误差曲线
    result.npoint_error10 = -1;
    // result.error10_resistance = -1;  // 负载电阻
    // result.error10_factor = -1;      // 1-1%误差限值
}

void ctpt_ctrl_private::copy_exciting( void )
{
    if ( this->exciting )
    {
        int np1 = this->exciting->wave_vol.size();
        int np2 = this->exciting->wave_cur.size();
        int size;
        int np;
        np = np1>np2 ? np2:np1;
        result.npoint_exciting = np;
        size = np * sizeof( result.exciting_voltage[0] );
        result.exciting_voltage = (double*)realloc( result.exciting_voltage, size );
        result.exciting_current = (double*)realloc( result.exciting_current, size );
        if ( result.exciting_voltage && result.exciting_current )
        {
            for (int i=0; i<np; i++)
            {
                result.exciting_voltage[i] = this->exciting->wave_vol[i].get_rms();
                result.exciting_current[i] = this->exciting->wave_cur[i].get_rms();
            }
        } else {
            result.npoint_exciting = 0;
        }
    } else {
        result.npoint_exciting = 0;
    }
}

void ctpt_ctrl_private::calculate_ct_ratio( void )
{
    if ( this->knee_index >0 )
    {
        int index = this->knee_index/2;
        this->ratio_vol_primary = this->ratio->wave_vol_primary[index];
        this->ratio_vol_secondary = this->ratio->wave_vol[index];
        double vol_1 = this->ratio_vol_primary.get_rms();
        double vol_2 = this->ratio_vol_secondary.get_rms();
        this->result.ratio       = vol_2 / vol_1;
        this->result.turns_ratio = vol_2 / vol_1;
        int np = 0;
        double p = this->ratio_vol_primary.get_multiply( this->ratio_vol_secondary, np );
        if ( p > 0 )
        {
            this->result.phase_polar = true;
        } else {
            this->result.phase_polar = false;
        }
        // double ratio;               // 变比  OK
        // double ratio_error;         // 比值差
        // double turns_ratio;    // 额定匝数比,    OK?
        // double turns_ratio_error;    // 匝数比误差
        // double phase_error;          // 相位差
        // bool   phase_polar;          // 极性     OK
    }
}

void ctpt_ctrl_private::calculate_burden( void )
{
    // setup output && read reasult.
    // double freq          = 0;
    // double vol_start     = 0;
    // double vol_step      = 0;
    // double vol_stop      = 0;
    // double limit_current = 0;


}

bool ctpt_ctrl_private::auto_setup_output( double &vol, double freq, double vol_min, double current_max, double current_need )
{
    bool ok = true;
    return ok;
}

bool ctpt_ctrl_private::calculate_fs( void )
{
    bool ok = true;

    // 一次电流最大值/额定一次电流.  一次电流的最大值：符合复合误差<10%的一次电流的最大值

    return ok;
}

int ctpt_ctrl::init( void )
{
    return 0;
}

int ctpt_ctrl::reinit( void )
{
    return init();
}

int ctpt_ctrl::config( const struct ct_setting &setting )
{
    int err = 0;

    this->_private->ct_setting = setting;
    this->_private->is_ct_mode = true;
    return err;
}


int ctpt_ctrl::config( const struct pt_setting &setting )
{
    int err = 0;

    this->_private->pt_setting = setting;
    this->_private->is_ct_mode = false;
    return err;
}


int ctpt_ctrl::start( void )
{
    int err = 0;

    return err;
}

int ctpt_ctrl::stop( void )
{
    int err = 0;

    return err;
}

int ctpt_ctrl::get_user_test_result( double *voltage, double *current, double *freq_measured )
{
    int err = 0;

    return err;
}

void ctpt_ctrl::register_real_time_listener( rt_listener *listener )
{
    this->_private->listener = listener;
}

rt_listener::rt_listener()
{

}

rt_listener::~rt_listener()
{

}


void rt_listener::process( int percent )
{

}

void rt_listener::scanning(double voltage, double current )
{

}

static double g_vols[] = {
    3035.582,
    2991.274,
    2942.158,
    2887.990,
    2831.644,
    2776.292,
    2723.646,
    2674.532,
    2617.916,
    2573.107,
    2530.741,
    2490.957,
    2452.228,
    2413.986,
    2374.669,
    2332.147,
    2278.93 ,
    2219.358,
    2145.934,
    2053.327,
    1937.806,
    1799.365,
    1637.469,
    1454.242,
    1215.045,
    934.7974,
    607.0804,
    303.2987,
    152.6674,
    86.314  ,
    51.3115 ,
    32.5341 ,
    20.4377 ,
    13.4156 ,
    8.8319  ,


    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
};

static double g_curs[] = {
    2.650518 ,
    2.033945 ,
    1.556123 ,
    1.176793 ,
    0.881148 ,
    0.65781  ,
    0.490574 ,
    0.373693 ,
    0.270774 ,
    0.200924 ,
    0.150503 ,
    0.113441 ,
    0.085847 ,
    0.066019 ,
    0.05139  ,
    0.040665 ,
    0.031957 ,
    0.025355 ,
    0.020227 ,
    0.01611  ,
    0.012798 ,
    0.010175 ,
    0.008103 ,
    0.006553 ,
    0.005132 ,
    0.003853 ,
    0.002889 ,
    0.002166 ,
    0.00163  ,
    0.001221 ,
    0.000915 ,
    0.000688 ,
    0.000514 ,
    0.000389 ,
    0.000289 ,

    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,
    // 100,    100,    100,    100,    100,    100,    100,    100,    100,    100,

};

int ctpt_ctrl::get_ctpt_result(ctpt_result &out)
{
    assert( sizeof(g_curs)                 == sizeof(g_vols) );

    // 负载, 电阻, 内阻
    out.rated_resitive_burden               = 0.1; // 额定电阻负荷, in
    out.secondary_winding_resistance        = 0.02; // 二次绕组电阻, out  --ok

    // 励磁
    out.knee_point_voltage                  = 1925.003; // 拐点电压
    out.knee_point_current                  = 0.01252; // 拐点电流
    out.secondary_limiting_emf              = 2; // 二次极限感应电势
    out.inductance_unsaturated              = 693.39; // 不饱和电感
    out.inductance                          = 11.235;
    out.rated_secondary_loop_time_constant  = 0.4; // 额定二次回路时间常数 Ts = Ls / Rs, out
    out.composite_error                     = 0.001; // 复合误差
    out.remanence_factor                    = 0.804; // 剩磁系数,

    out.accuracy_limit_factor               = 8.8; // P,PR 准确限值系数

    out.rated_knee_point_emf                = 1000; // PX 额定拐点电势, Ek
    out.rated_knee_point_current            = 1000; // PX 额定拐点电势, ie_ek

    out.kare_static                         = 1; // TPY，TPX,TPZ 暂态面积系数
    out.kssc                                = 1; // TPY，TPS,TPX,TPZ 所得励磁电流I_exc与额定二次电流I_sn和仪表保安系数FS乘积之比用百分数表示时，其值应等于或者大于额定复合误差限值，即10%
    out.peek_error                          = 1; // TPY，TPX,TPZ 峰瞬误差

    out.instrument_security_factor          = 10; // 计量类/measured 仪表保安系数

    out.dimensioning_factor                 = 1; // 计算系数， PX

    out.u_al                                = 1; // TPS 额定Ual
    out.i_al_u_al                           = 1; // TPS Ual对应的Ial

    // 变比
    out.ratio                               = 100; // 变比
    out.ratio_error                         = 30; // 比值差
    out.turns_ratio                         = 20; // 额定匝数比,
    out.turns_ratio_error                   = 50; // 匝数比误差
    out.phase_error                         = 16; // 相位差
//    bool   phase_polar;          // 极性

    out.npoint_exciting                     = sizeof(g_vols)/sizeof(g_vols[0]);
    // std::cout<< "out.npoint_exciting=" << out.npoint_exciting << std::endl;
    memcpy(out.exciting_current, g_curs, sizeof(g_curs) );
    memcpy(out.exciting_voltage, g_vols, sizeof(g_vols) );

    for(int i=0; i<out.npoint_exciting; i++)
    {

        // out.exciting_current[i]          = 30*i;
        // out.exciting_voltage[i]          = i*i;
        //     qDebug()<<QString::number(out.exciting_current[i],'g',6);
        out.error5_resistance[i]            = i*30;
        out.error5_factor[i]                = i*i/2;
        out.error10_factor[i]               = -i*i;
        out.error10_resistance[i]           = 30*i+6;

    }
//    // 励磁曲线
//     out.npoint_exciting                  = 0;
//    double *exciting_current;   // x coordinate
//    double *exciting_voltage;   // y coordinate

//    // 5%误差曲线
    out.npoint_error5                       = out.npoint_exciting;
//    double *error5_resistance;  // 负载电阻
//    double *error5_factor;      // 5%误差限值

//    // 10%误差曲线
    out.npoint_error10                      = out.npoint_exciting;
//    double *error10_resistance;  // 负载电阻
//    double *error10_factor;      // 10%误差限值
    return 1;
}
