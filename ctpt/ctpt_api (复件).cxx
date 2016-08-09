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
#include <unistd.h>

#include <algorithm>

#include <list>
#include "ctpt_api.hpp"
#include "data_source.hpp"
#include <pthread.h>
#include <semaphore.h>
#include "fpga.hpp"


#include <sstream>
#include <iostream>


static fpga_dev fpga;

#include "data_source_fpga.hpp"

// std::stringstream g_ctpt_log;
#ifndef NO_LOG
#define g_ctpt_log std::cout
#else
std::stringstream g_ctpt_log;
#endif

#define SIMULATION

#ifdef SIMULATION
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

    //13.4156 - 8.8319 =4.5837
    //0.000389 - 0.000289 = 0.0001;


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
#endif

class ctpt_ctrl_private
{
public:
    ctpt_ctrl_private();
    virtual ~ctpt_ctrl_private();


public:
    static void main_thread( ctpt_ctrl_private *p_this );
    /*
     * 测量励磁
     */
    void run_test( void );

	//测试变比
    int test_ratio(void);
	int test_excitation(void);
    int test_excitation_output(double freq, std::list<struct test_result> list_result);
	int test_resistance(void);

    bool find_knee_point( struct auto_scan_result &r );
	bool find_knee_point( void );
	
    void calculate_resistance( void );
    void calculate_all( void );
    void copy_exciting( void );

    bool calculate_5_percent_point(double resistance, double rated_current);
    bool calculate_10_percent_point(double resistance, double rated_current);

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

    sem_t sem;

    bool is_ct_mode;
    bool is_pt_mode;
    user_test_mode user_mode;
    volatile bool is_stopped;

    double user_vol;
    double user_cur;
    double user_freq;

    double user_result_vol;
    double user_result_cur;

    int npoint_exciting;
    double *exciting_current;   // x coordinate
    double *exciting_voltage;   // y coordinate
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

    this->ratio_error_npoint = np;
    this->ratio_error_va     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_factor = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_1      = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_5      = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_10     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_20     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->ratio_error_100    = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_npoint = np;
    this->phase_error_va     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_factor = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_1      = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_5      = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_10     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_20     = (double*)malloc( np * sizeof(this->exciting_current[0]) );
    this->phase_error_100    = (double*)malloc( np * sizeof(this->exciting_current[0]) );
}

#define __safe_free(p) do{if (p) {free(p);p=0;}}while (0)
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

    __safe_free(this->ratio_error_va     );
    __safe_free(this->ratio_error_factor );
    __safe_free(this->ratio_error_1      );
    __safe_free(this->ratio_error_5      );
    __safe_free(this->ratio_error_10     );
    __safe_free(this->ratio_error_20     );
    __safe_free(this->ratio_error_100    );
    __safe_free(this->phase_error_va     );
    __safe_free(this->phase_error_factor );
    __safe_free(this->phase_error_1      );
    __safe_free(this->phase_error_5      );
    __safe_free(this->phase_error_10     );
    __safe_free(this->phase_error_20     );
    __safe_free(this->phase_error_100    );

}

struct test_result {
    double vol;
    double cur;

    test_result(const test_result& copyin) {
        this->vol = copyin.vol; this->cur = copyin.cur;
    }
    test_result() {
        this->vol = 0.0;
        this->cur = 0.0;
    }

    test_result &operator=(const test_result &rhs) {this->vol = rhs.vol; this->cur = rhs.cur; return *this;}
    int operator==(const test_result &rhs) {
        if(this->vol != rhs.vol) return 0;
        if(this->cur != rhs.cur) return 0;
        return 1;
    }
};


void ctpt_ctrl_private::main_thread( ctpt_ctrl_private *p_this )
{
    int ret;
    raw_wave wave_i_load;
    raw_wave wave_u_src;
    raw_wave wave_u_pri;
    raw_wave wave_u_load;

    struct test_result result;
    std::list<struct test_result> list_result;

    // static data_source_fpga dsf;

    std::cout<< "main thread init" << std::endl;

#ifndef SIMULATION
    //comment this when simulation
    fpga.init(0, 4 );
#endif

    while(1) {
        sem_wait(&p_this->sem);

        if(p_this->user_mode == USER_TEST_SELFCHECK) {
            std::cout<< "slef check running test" << std::endl;
            const char *filename = "/mnt/mmcblk0p1/data.csv";
            FILE *fp = fopen(filename, "w");

            fpga.dc_ac_f_set( p_this->user_freq );
            fpga.select_range_by_value(fpga_dev::PRI_VOL_CHNL, 30.0);
            fpga.start_output();
            fpga.safe_output(p_this->user_vol, true);
            fpga.wait_output_steaby();
            fpga.adc_start_sample();

            do {
                ret = fpga.read( &wave_i_load, &wave_u_src, &wave_u_pri, &wave_u_load );
                if (ret== 0){
                    wave_i_load.calculate_all();
                    wave_u_src.calculate_all();
                    wave_u_pri.calculate_all();
                    wave_u_load.calculate_all();

                    p_this->user_result_vol = wave_u_pri.get_rms();
                    p_this->user_result_cur = wave_i_load.get_rms();

                    fp = fopen(filename, "w");
                    if (fp){
                        printf("ch1=%fA,ch2=%fV,ch3=%fV,ch4=%fV,\n",
                               wave_i_load.get_rms(),
                               wave_u_src.get_rms(),
                               wave_u_pri.get_rms(),
                               wave_u_load.get_rms()
                            );
                        fclose(fp);
                    }
                    int n = wave_i_load.get_np();
                    std::vector<double> &chd1 = wave_i_load.get_writable_buffer();
                    std::vector<double> &chd2 = wave_u_src.get_writable_buffer();
                    std::vector<double> &chd3 = wave_u_pri.get_writable_buffer();
                    std::vector<double> &chd4 = wave_u_load.get_writable_buffer();
                    for (int i=0; i<n; i++ ) {
                        printf("%6.4f, %6.4f, %6.4f, %6.4f\n ",
                               chd1[i],
                               chd2[i],
                               chd3[i],
                               chd4[i]
                            );
                    }
                }
                sleep(1);
            }while (!p_this->is_stopped);
            fpga.stop_output();
            fpga.adc_stop_sample();
        }
        else if (p_this->user_mode == USER_TEST_EXCITING){
            std::cout<< "exciting running test" << std::endl;

            double start_v = 0.1;
            double maxV = p_this->user_vol;
            double maxI = p_this->user_cur;
            double step_v = 0.3;

            raw_wave ch1;               // load current
            raw_wave ch2;               // src voltage
            raw_wave ch3;               // primary voltage
            raw_wave ch4;               // load voltage

            if (maxI > 4.0) {
                maxI = 4.0;
            }
            if (maxV > 120.0) {
                maxV = 120.0;
            }
            printf("++maxI: %f  maxV:  %f++\n", maxI, maxV);

            list_result.clear();
            p_this->npoint_exciting = 0;

            fpga.dc_ac_f_set( p_this->user_freq );
            fpga.set_np_per_cycle( 512 );

            fpga.select_range_by_value(2, 30.0);
            fpga.adc_start_sample();
            fpga.start_output();

            do {
                printf("++set_v %f++\n", start_v);
                if (start_v < 1.0) {
                    step_v = 1;
                }
                else if (start_v < 30.0) {
                    step_v = 2;
                }
                else if (start_v < 50.0) {
                    step_v = 0.3/*5.0*/;
                }
                else {
                    step_v = 0.5;
                }
                ret = fpga.safe_output(start_v, true);
                if (ret != 0) {
                    break;
                }
                printf("++wait_output_steaby++\n");
                ret = fpga.wait_output_steaby();
                if (ret != 0) {
                    break;
                }
                printf("++get_active_sample_wave++\n");
                ret = fpga.get_active_sample_wave(&ch1, &ch2, &ch3, &ch4 );
                if (ret != 0) {
                    break;
                }
                result.vol = ch2.get_base_rms();
                result.cur = ch1.get_base_rms();
                printf("chnl1:%f chnl2:%f chnl3:%f chnl4:%f------\n", ch1.get_base_rms(), ch2.get_base_rms(), ch3.get_base_rms(), ch4.get_base_rms());
                //printf("++vol:%f  cur:%f++\n", result.vol, result.cur);
                list_result.push_back(result);
                if (result.vol >= maxV || result.cur >= maxI || start_v >= maxV*2 || start_v >= 150) {
                    break;
                }
                start_v += step_v;
            } while(!p_this->is_stopped);

            fpga.stop_output();
            fpga.adc_stop_sample();

            //copy data to dst
            if (list_result.size() != 0) {
                double preV;
                double preC;
                double k = 0;
                int i = 0;
                printf("++clear++\n");
                int np = list_result.size();
                printf("++copy++\n");
                __safe_free(p_this->exciting_current );
                __safe_free(p_this->exciting_voltage );
                p_this->exciting_current   = (double*)malloc( np * sizeof(double) );
                if (!p_this->exciting_current) {
                    printf("++p_this->exciting_current error++\n");
                }
                p_this->exciting_voltage   = (double*)malloc( np * sizeof(double) );
                if (!p_this->exciting_voltage) {
                    printf("++p_this->exciting_voltage error++\n");
                }
                for(std::list<struct test_result>::iterator it = list_result.begin(); it != list_result.end(); it++) {
                    if (i == 0) {
                        preV = it->vol;
                        preC = it->cur;
                    }
                    else {
                        if (preV > it->vol || preC > it->cur*1.1) {
                            continue;
                        }
                        preV = it->vol;
                        preC = it->cur;
                    }
                    p_this->exciting_voltage[i] = it->vol;
                    p_this->exciting_current[i] = it->cur;
                    printf("++i: %d vol:%f  cur:%f++\n", i, it->vol, it->cur);
                    i++;
                }
                p_this->npoint_exciting = i;
            }
            p_this->is_stopped = true;
        }
        else {
            printf("p_this->user_mode == USER_TEST_RUN\n");

            p_this->run_test();
        }
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
    is_ct_mode(false),
    is_pt_mode(false),
    user_mode(USER_TEST_NONE),
    npoint_exciting(0),
    exciting_current(0),
    exciting_voltage(0)

{

    memset( &result, 0x00, sizeof(result) );
    result.exciting_current   = 0;
    result.exciting_voltage   = 0;
    result.error10_factor     = 0;
    result.error5_factor      = 0;
    result.error10_resistance = 0;
    result.error5_resistance  = 0;

    pthread_t tid = 0;
    sem_init(&this->sem, 0, 0);
    pthread_create( &tid, NULL, (void* (*)(void*))main_thread, this);
    g_ctpt_log << "inited tid ="<< tid << std::endl;
    assert(tid);
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
    sem_destroy(&this->sem);

    __safe_free(this->exciting_voltage     );
    __safe_free(this->exciting_current     );
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
 

void ctpt_ctrl_private::run_test( void )
{

	printf("begin test! \n");
        if ( this->listener )
        {
            this->listener->process(0);
        }

#ifdef SIMULATION
        printf("----------Simulation test!\n----------");
        for(int i=0; i<5; i++)
        {
            sleep(1);
            if ( this->listener )
                this->listener->process(i*20);
	}
        is_stopped = true;
        calculate_all();
#endif

	/*根据选定项目进行测试*/
    enum ctpt_test_item ti = ct_setting.test_item;
	//若已测励磁曲线，则不单独测变比
	bool isTestExcitation = false;
        if((ti & ITEM_RESISTANCE) == ITEM_RESISTANCE)
	{
		test_resistance();	
	}
        if((ti & ITEM_EXCITATION) == ITEM_EXCITATION)
	{
		test_excitation();	
		isTestExcitation = true;
	}
        if(((ti & ITEM_RATIO) == ITEM_RATIO)&& !isTestExcitation)
	{
                test_ratio();
	}
        if((ti & ITEM_BURDEN )== ITEM_BURDEN)
	{
			
	}

        if ( this->listener )
        {
           this->listener->process(100);
        }
	

}
/**********************************************
* @Desc:直流电阻测试，输出直流电压，测量绕组两端
*	   电压与电流，根据欧姆定律计算，取10组平均数
* @return:1正常返回，-1异常返回
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
int ctpt_ctrl_private::test_resistance(void)
{ 
    //模拟数据
    result.secondary_winding_resistance = 1;
    return 1;
	
    raw_wave wave_i_load;
    raw_wave wave_u_src;
    raw_wave wave_u_pri;
    raw_wave wave_u_load;
	
	float out_vol = 0.01;
	
    //之后以直流输出替换
    fpga.dc_ac_f_set( 1 );
	
    fpga.select_range_by_value(fpga_dev::SECOND_VOL_CHNL, 0.1);
    fpga.start_output();

    float res = 0;
    int count = 0;
    float result_vol = 0;
    float result_cur = 0;
    do {
        fpga.safe_output(out_vol, true);
        fpga.wait_output_steaby();
        fpga.adc_start_sample();
        int ret = fpga.read( &wave_i_load, &wave_u_src, &wave_u_pri, &wave_u_load );
        if (ret== 0){
            wave_i_load.calculate_all();
            wave_u_src.calculate_all();
            wave_u_pri.calculate_all();
            wave_u_load.calculate_all();


            printf("ch1=%fA,ch2=%fV,ch3=%fV,ch4=%fV,\n",
                    wave_i_load.get_rms(),
                    wave_u_src.get_rms(),
                    wave_u_pri.get_rms(),
                    wave_u_load.get_rms()
                );
			
            result_vol = wave_u_load.get_rms();
            result_cur = wave_i_load.get_rms();	
			
            printf("++ test resistance, volate: %f  current:  %f++\n", result_vol, result_cur);

            //电流过大,取原十分之一电压
            if(result_cur > 5)
            {
                    out_vol = out_vol / 10;
            }
            //按原来的50%增加电压
            out_vol += out_vol*0.5;

            //累加结果
            if(result_cur !=0 )
            {
                    res += result_vol / result_cur;
                    count++;
            }
            //测量10次求平均值
            if(count >10)
            {
                    result.secondary_winding_resistance = res/count;
                    return 1;
            }
        }    
        sleep(1);		
    }while (!is_stopped);
    return -1;
}

/**********************************************
* @Desc:测量变比，步进增加电压，测量绕组两端
*	   电压与电流，并计算变比
* @return:1正常返回，-1异常返回
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
int ctpt_ctrl_private::test_ratio(void)
{
	printf( "radio running test\n");

#ifdef SIMULATION
        //simulate data
        this->result.ratio = 1000;
        return 1;
#endif

	std::list<double> vol_primary; // 初级线圈电压
        std::list<double> vol_secondary; // 次级线圈电压
	 
        float ratio = 0;
	float vol_pri = 0;
	float vol_sec = 0;
        double start_v = 0.1;
        double maxV = 100;
        double maxI = 4.0;
	double step_v = 0.3;

	struct test_result tresult;
	raw_wave ch1;               // load current
	raw_wave ch2;               // src voltage
	raw_wave ch3;               // primary voltage
	raw_wave ch4;               // load voltage

	if (maxI > 4.0) {
		maxI = 4.0;
	}
        if (maxV > 100.0) {
                maxV = 100.0;
	}
	printf("++maxI: %f  maxV:  %f++\n", maxI, maxV);

	vol_primary.clear();
	vol_secondary.clear(); 

	fpga.dc_ac_f_set( 50 );
	fpga.set_np_per_cycle( 512 );

        fpga.select_range_by_value(fpga_dev::PRI_VOL_CHNL, 120.0);
        fpga.select_range_by_value(fpga_dev::SECOND_VOL_CHNL, 120.0);
        fpga.select_range_by_value(fpga_dev::LOAD_CUR_CHNL, 10.0);
        fpga.select_range_by_value(fpga_dev::SRC_VOL_CHNL, 120.0);

	fpga.adc_start_sample();
	fpga.start_output();

	do {
		printf("++set_v %f++\n", start_v);
		if (start_v < 1.0) {
			step_v = 1;
		}
		else if (start_v < 30.0) {
                        step_v = 5;
		}
		else if (start_v < 50.0) {
                        step_v = 5/*5.0*/;
		}
		else {
                        step_v = 5;
		}
                int ret = fpga.safe_output(start_v, true);
		if (ret != 0) {
			break;
		}
		printf("++wait_output_steaby++\n");
		ret = fpga.wait_output_steaby();
		if (ret != 0) {
			break;
		}
		printf("++get_active_sample_wave++\n");
		ret = fpga.get_active_sample_wave(&ch1, &ch2, &ch3, &ch4 );
		if (ret != 0) {
			break;
		}
	
                vol_pri = ch3.get_base_rms();
                vol_sec = ch4.get_base_rms();
                tresult.vol = ch2.get_base_rms();
                tresult.cur = ch1.get_base_rms();

		if ( this->listener )
        	{            
            		this->listener->scanning( tresult.vol, tresult.cur);
        	}
		
		vol_primary.push_back(vol_pri);
		vol_secondary.push_back(vol_sec);
		
		printf("chnl1:%f chnl2:%f chnl3:%f chnl4:%f------\n", ch1.get_base_rms(), ch2.get_base_rms(), ch3.get_base_rms(), ch4.get_base_rms());
		
                if (tresult.vol >= maxV || tresult.cur >= maxI || start_v >= maxV ) {
			break;
		}
		start_v += step_v;
        } while(!is_stopped);

	fpga.stop_output();
	fpga.adc_stop_sample();

        //异常停止
        if(is_stopped)
            return -1;
	
	//计算变比，多组值求平均值
	int n_size = vol_primary.size();
        printf("The size of sample ratio is: %f \n", n_size);
	for(int i=0; i<n_size; i++)
	{
                vol_pri = vol_primary.front();
                vol_sec = vol_secondary.front();
                ratio += vol_sec / vol_pri;
                vol_primary.pop_front();
                vol_secondary.pop_front();
	}

	printf("The ratio is: %f \n", ratio);
        this->result.ratio = ratio / n_size;
	return 1;
}

/**********************************************
* @Desc:绘制励磁曲线，步进增加电压，测量绕组两端
*	   电压与电流，并计算拐点
* @return:1正常返回，-1异常返回
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
int ctpt_ctrl_private::test_excitation_output(double freq, std::list<struct test_result> list_result)
{
 #ifdef SIMULATION
         return 1;
 #endif

     struct test_result result;
     std::list<struct test_result> list_result;
     double start_v = 0.1;
     double maxV = 120;
     double maxI = 4;
     double step_v = 0.3;

     raw_wave ch1;               // load current
     raw_wave ch2;               // src voltage
     raw_wave ch3;               // primary voltage
     raw_wave ch4;               // load voltage

     if (maxI > 4.0) {
             maxI = 4.0;
     }
     if (maxV > 120.0) {
             maxV = 120.0;
     }
     printf("++maxI: %f  maxV:  %f++\n", maxI, maxV);

     list_result.clear();
     npoint_exciting = 0;

     fpga.dc_ac_f_set( 50 );
     fpga.set_np_per_cycle( 512 );

     fpga.select_range_by_value(fpga_dev::PRI_VOL_CHNL, 120.0);
     fpga.select_range_by_value(fpga_dev::SECOND_VOL_CHNL, 120.0);
     fpga.select_range_by_value(fpga_dev::LOAD_CUR_CHNL, 10.0);
     fpga.select_range_by_value(fpga_dev::SRC_VOL_CHNL, 120.0);

     fpga.adc_start_sample();
     fpga.start_output();

     do {
         printf("++set_v %f++\n", start_v);
         if (start_v < 1.0) {
                 step_v = 1;
         }
         else if (start_v < 30.0) {
                 step_v = 5;
         }
         else if (start_v < 100.0) {
                 step_v = 10/*5.0*/;
         }
         else {
                 step_v = 1;
         }
         int ret = fpga.safe_output(start_v, true);
         if (ret != 0) {
                 break;
         }
         printf("++wait_output_steaby++\n");
         ret = fpga.wait_output_steaby();
         if (ret != 0) {
                 break;
         }
         printf("++get_active_sample_wave++\n");
         ret = fpga.get_active_sample_wave(&ch1, &ch2, &ch3, &ch4 );
         if (ret != 0) {
                 break;
         }
         result.vol = ch4.get_base_rms();
         result.cur = ch1.get_base_rms();
         printf("currnet:%f src_vol:%f pri_vol:%f sec_vol:%f------\n", ch1.get_base_rms(), ch2.get_base_rms(), ch3.get_base_rms(), ch4.get_base_rms());

         list_result.push_back(result);
         if (result.vol >= maxV || result.cur >= maxI || start_v >= maxV*2 || start_v >= 150) {
                 break;
         }
         start_v += step_v;
     } while(!is_stopped);

 /*******************************************************************/
 #if 0
     double deltaU = 0, deltaI = 0, k1=0,k2=0;
     double pre_vol = 0, pre_cur = 0;
     do{
         printf("++set_v %f++\n", start_v);

         int ret = fpga.safe_output(start_v, true);
         if (ret != 0) {
                 break;
         }
         printf("++wait_output_steaby++\n");
         ret = fpga.wait_output_steaby();
         if (ret != 0) {
                 break;
         }
         printf("++get_active_sample_wave++\n");
         ret = fpga.get_active_sample_wave(&ch1, &ch2, &ch3, &ch4 );
         if (ret != 0) {
                 break;
         }
         result.vol = ch4.get_base_rms();
         result.cur = ch1.get_base_rms();
         printf("currnet:%f src_vol:%f pri_vol:%f sec_vol:%f------\n", ch1.get_base_rms(), ch2.get_base_rms(), ch3.get_base_rms(), ch4.get_base_rms());

         list_result.push_back(result);
         if (result.vol >= maxV || result.cur >= maxI || start_v >= maxV*2 || start_v >= 150) {
                 break;
         }

         if (start_v < 1.0) {
                 step_v = 1;
         }
         else if (start_v < 30.0) {
                 step_v = 2.5;
         }
         else if (start_v < 100.0) {
                 step_v = 5.0;
         }
         else {
                 step_v = 10;
         }

         deltaU = result.vol - pre_vol;
         deltaI = result.cur - pre_cur;
         k1 = deltaU *  pre_cur;
         k2 = deltaI * pre_vol;
         if(k1 != 0 && k2 !=0)
         {
             incr = k1 / k2;
         }
         //Get to the Knee Point
         if ( incr <= 0.2)
         {
             step_v = start_v*0.02;
             printf("Get to the knee point\n");
         }

         start_v += step_v;

         pre_vol = result.vol;
         pre_cur = result.cur;
     }while(!is_stopped);
 #endif
  /*******************************************************************/


     fpga.stop_output();
     fpga.adc_stop_sample();

     //非正常停止
     if(is_stopped)
     {
         printf("stop unexpect!\n");
         return -1;
     }

     //copy data to dst
     if (list_result.size() != 0) {
             double preV;
             double preC;
             double k = 0;
             int i = 0;
             printf("++clear++\n");
             int np = list_result.size();
             printf("++copy++\n");
             __safe_free(exciting_current );
             __safe_free(exciting_voltage );
             exciting_current   = (double*)malloc( np * sizeof(double) );
             if (!exciting_current) {
                     printf("++p_this->exciting_current error++\n");
             }
             exciting_voltage   = (double*)malloc( np * sizeof(double) );
             if (!exciting_voltage) {
                     printf("++p_this->exciting_voltage error++\n");
             }
             for(std::list<struct test_result>::iterator it = list_result.begin(); it != list_result.end(); it++) {
                     if (i == 0) {
                             preV = it->vol;
                             preC = it->cur;
                     }
                     else {
                             if (preV > it->vol || preC > it->cur*1.1) {
                                     continue;
                             }
                             preV = it->vol;
                             preC = it->cur;
                     }
                     exciting_voltage[i] = it->vol;
                     exciting_current[i] = it->cur;
                     printf("++i: %d vol:%f  cur:%f++\n", i, it->vol, it->cur);
                     i++;
             }
             npoint_exciting = i;
             copy_exciting();
             find_knee_point();
             calculate_10_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);
             calculate_5_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);
     }
     return 1;

}

/**********************************************
* @Desc:绘制励磁曲线，步进增加电压，测量绕组两端
*	   电压与电流，并计算拐点
* @return:1正常返回，-1异常返回
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
int ctpt_ctrl_private::test_excitation(void)
{
   printf( "running exciting test\n");

#ifdef SIMULATION
        return 1;
#endif


    std::list<struct test_result> list_result;
    list_result.clear();
    npoint_exciting = 0;

    int ret = test_excitation_output(50.0, list_result);

    switch(ret)
    {
        case -1:
            printf("stop unexpect!\n");
            return -1;
        case 1:
            printf("exciting OK!\n");
            break;
        case 2:
            test_excitation_output(5.0);
            break;
        default:
            break;
    }

    //copy data to dst
    if (list_result.size() != 0) {
        double preV;
        double preC;
        int i = 0;
        int np = list_result.size();
        __safe_free(exciting_current );
        __safe_free(exciting_voltage );
        exciting_current   = (double*)malloc( np * sizeof(double) );
        if (!exciting_current) {
                printf("++p_this->exciting_current error++\n");
        }
        exciting_voltage   = (double*)malloc( np * sizeof(double) );
        if (!exciting_voltage) {
                printf("++p_this->exciting_voltage error++\n");
        }
        for(std::list<struct test_result>::iterator it = list_result.begin(); it != list_result.end(); it++) {
                if (i == 0) {
                        preV = it->vol;
                        preC = it->cur;
                }
                else {
                        if (preV > it->vol || preC > it->cur*1.1) {
                                continue;
                        }
                        preV = it->vol;
                        preC = it->cur;
                }
                exciting_voltage[i] = it->vol;
                exciting_current[i] = it->cur;
                printf("++i: %d vol:%f  cur:%f++\n", i, it->vol, it->cur);
                i++;
        }
        npoint_exciting = i;
        copy_exciting();
        find_knee_point();
        calculate_10_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);
        calculate_5_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);
    }
    return 1;
}
/**********************************************
* @Desc:复制励磁曲线到输出结果
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
void ctpt_ctrl_private::copy_exciting( void )
{
	int np = this->npoint_exciting;
	result.npoint_exciting = np;
	int size = np * sizeof( result.exciting_voltage[0] );
	result.exciting_voltage = (double*)realloc( result.exciting_voltage, size );
	result.exciting_current = (double*)realloc( result.exciting_current, size );
	if ( result.exciting_voltage && result.exciting_current )
	{
		for (int i=0; i<np; i++)
		{
			result.exciting_voltage[i] = this->exciting_voltage[i];
			result.exciting_current[i] = this->exciting_current[i];
		}
	} else {
		result.npoint_exciting = 0;
	}
}

/**********************************************
* @Desc:查找拐点
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
bool ctpt_ctrl_private::find_knee_point()
{
    int i;
    int n_vol = 0;
    int n_cur = 0;
    int n = npoint_exciting;

    knee_index = -1;  

    if ( n < 1 )
    {
        assert( 0 && "never be n<1 ");
        return false;
    }

    double pre_vol = this->exciting_voltage[0];
    double pre_cur = this->exciting_current[0];
			
    for (i = 1; i < n; i++)
    {
        double deltaU, deltaI;
        double vol, cur;
        double incr = 5;
        double k1,k2;
        vol = this->exciting_voltage[i];
        cur = this->exciting_current[i];
        deltaU = vol - pre_vol;
        deltaI = cur - pre_cur;
        k1 = deltaU *  pre_cur;
        k2 = deltaI * pre_vol;
        if(k1 != 0 && k2 !=0)
        {
            incr = k1 / k2;
        }
        if ( incr <= 0.2)
        {
            // ok, found
            this->knee_index = i;
            this->vol_preknee = pre_vol;
            this->cur_preknee = pre_cur;
            this->vol_knee = vol;
            this->cur_knee = cur;
			
            //将拐点结果复制到输出
            this->result.knee_point_voltage = pre_vol;
            this->result.knee_point_current = pre_cur;
            std::cout<< "found knee point @" << vol << "V " << cur << "A" << std::endl;
            return true;
        }
        pre_vol = vol;
        pre_cur = cur;
    }
    return false;
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
            std::cout<< "found knee point @" << vol << "V " << cur << "A" << std::endl;
            return true;
        }
        pre_vol = vol;
        pre_cur = cur;
    }

    return false;
}


/**********************************************
* @Desc:计算5%误差曲线点
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
bool ctpt_ctrl_private::calculate_5_percent_point(double resistance, double rated_current)
{
    int np = npoint_exciting;
    int n=0;
    //一次电流与额定一次电流比
    double Kalf = 0;
    //允许负载
    double ZLoad = 0;
    //励磁电流
    double Ie = 0;
    //二次感应电动势
    double Es = 0;
    //二次绕组阻抗，Z2 = Rct +jXct，Xct一般可忽略或取0.1或0.2
    double Z2 = 0;
    if(resistance < 1)
            Z2 = 1;
    else
            Z2 = resistance;

    //限定额定电流
    if(rated_current <1 || rated_current >5)
            rated_current = 5;

    if(np <1)
            return false;
    int size = np * sizeof( result.error5_resistance[0] );
    result.error5_resistance = (double*)realloc( result.error5_resistance, size );
    result.error5_factor = (double*)realloc( result.error5_factor, size );
    if ( !result.error5_resistance || !result.error5_factor )
        return false;

    for(int i =0; i<np; i++)
    {
        Ie = this->exciting_current[i];
        Es = this->exciting_voltage[i] - Ie*Z2;
        Kalf = 20*Ie / rated_current;
        ZLoad = Es/(19*Ie) - Z2;

        if(Kalf<1)
            continue;

        result.error5_resistance[n]            = ZLoad;
        result.error5_factor[n]                = Kalf;
        n++;
    }
    result.npoint_error5 = n;
    return true;
}
/**********************************************
* @Desc:计算10%误差曲线点
* @Author:zch
* @Date:2016.06.27
*
***********************************************/
bool ctpt_ctrl_private::calculate_10_percent_point(double resistance, double rated_current)
{
    int n=0;
    int np = npoint_exciting;

    //一次电流与额定一次电流比
    double Kalf = 0;
    //允许负载
    double ZLoad = 0;
    //励磁电流
    double Ie = 0;
    //二次感应电动势
    double Es = 0;
    //二次绕组阻抗，Z2 = Rct +jXct，Xct一般可忽略或取0.1或0.2
    double Z2 = 0;
    if(resistance <=0)
         Z2 = 1;
    else
        Z2 = resistance;

    //限定额定电流
    if(rated_current <1 || rated_current >5)
        rated_current = 5;

    if(np <1)
            return false;
    int size = np * sizeof( result.error10_resistance[0] );
    result.error10_resistance = (double*)realloc( result.error10_resistance, size );
    result.error10_factor = (double*)realloc( result.error10_factor, size );
    if ( !result.error10_resistance || !result.error10_factor )
        return false;

    for(int i =0; i<np; i++ )
    {
        Ie = this->exciting_current[i];
        Es = this->exciting_voltage[i] - Ie*Z2;
        Kalf = 10*Ie / rated_current;
        ZLoad = Es/(9*Ie) - Z2;

        if(Kalf<1)
            continue;

        result.error10_resistance[n]            = ZLoad;
        result.error10_factor[n]                = Kalf;
        n++;
        }
        result.npoint_error10 = n;
        return true;
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
    this->npoint_exciting = 35;

    int np = 35;

    __safe_free(exciting_current );
    __safe_free(exciting_voltage );
    exciting_current   = (double*)malloc( np * sizeof(double) );
    if (!exciting_current) {
            printf("++p_this->exciting_current error++\n");
    }
    exciting_voltage   = (double*)malloc( np * sizeof(double) );
    if (!exciting_voltage) {
            printf("++p_this->exciting_voltage error++\n");
    }

    for(int i=0; i<np; i++)
    {
       exciting_voltage[i] = g_vols[np-1-i];
       exciting_current[i] = g_curs[np-1-i];

    }

    find_knee_point();
    calculate_10_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);
    calculate_5_percent_point(this->result.secondary_winding_resistance,this->ct_setting.rated_secondary_current);

    result.knee_point_voltage = this->vol_knee; // 拐点电压
    result.knee_point_current = this->cur_knee; // 拐点电流

    // 二次极限感应电势, 依赖: 负载阻抗、内阻、保安系数和二次额定电流
    double fs = result.instrument_security_factor;            // FIXME: set fs
    result.secondary_limiting_emf = fs * ( std::abs( this->zburden + this->zresistance ) * this->ct_setting.rated_secondary_current );

    // result.inductance = -1; // 饱和电感
    // result.inductance_unsaturated = -1; // 不饱和电感
    result.rated_secondary_loop_time_constant = 5; // 额定二次回路时间常数 Ts = Ls / Rs, out
    result.composite_error = 0.5;        // 复合误差
    result.remanence_factor = 3.5;    // 剩磁系数,
    result.accuracy_limit_factor = 3.2; // P,PR 准确限值系数
    result.rated_knee_point_emf = 51.1; // PX 额定拐点电势, Ek
    result.rated_knee_point_current = 0.02; // PX 额定拐点电势, ie_ek
    result.kare_static = 5;         // TPY，TPX,TPZ 暂态面积系数
    result.kssc = 1; // TPY，TPS,TPX,TPZ 所得励磁电流I_exc与额定二次电流I_sn和仪表保安系数FS乘积之比用百分数表示时，其值应等于或者大于额定复合误差限值，即1-1%
    result.peek_error = 0.2;          // TPY，TPX,TPZ 峰瞬误差
    result.instrument_security_factor = 2.1; // 计量类/measured 仪表保安系数
    result.dimensioning_factor = 2.3;  // 计算系数， PX
    result.u_al = -1;    // TPS 额定Ual
    result.i_al_u_al = -1;// TPS Ual对应的Ial

    // 变比
    //result.ratio = 500;               // 变比
    result.ratio_error = 100;         // 比值差
    result.turns_ratio = 500;    // 额定匝数比,
    result.turns_ratio_error = 0.5;    // 匝数比误差
    result.phase_error = 01;          // 相位差
    result.phase_polar = 1;          // 极性

    // 励磁曲线
    copy_exciting();

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
    this->_private->is_pt_mode = false;
    this->_private->user_mode = USER_TEST_NONE;
    return err;
}


int ctpt_ctrl::config( double voltage, double current, double freq, user_test_mode testMode )
{
    this->_private->is_ct_mode = false;
    this->_private->is_pt_mode = false;
    this->_private->user_mode = testMode;

    this->_private->user_vol = voltage;
    this->_private->user_cur = current;
    this->_private->user_freq = freq;

    std::cout<< "config witch U="<< this->_private->user_vol
             << " I=" << this->_private->user_cur
             << " freq=" << this->_private->user_freq << std::endl;
    return 1;
}

int ctpt_ctrl::config( const struct pt_setting &setting )
{
    int err = 0;

    this->_private->is_ct_mode = false;
    this->_private->is_pt_mode = true;
    this->_private->user_mode = USER_TEST_NONE;
    this->_private->pt_setting = setting;
    return err;
}


int ctpt_ctrl::start( void )
{
    int err = 0;
    std::cout<< "+++start pressed++++" << std::endl;
    this->_private->is_stopped = true;
    if (!this->_private->is_stopped){
        return -1;
    }
    this->_private->is_stopped = false;


    sem_post(&this->_private->sem);
    return err;
}

int ctpt_ctrl::stop( void )
{
    int err = 0;
    std::cout<< "stop pressed" << std::endl;
    this->_private->is_stopped = true;

#ifndef SIMULATION
    fpga.stop_output();
    fpga.adc_stop_sample();
#endif
    return err;
}

int ctpt_ctrl::get_user_test_result( double *voltage, double *current, double *freq_measured )
{
    int err = 0;
    if(voltage){
        *voltage = this->_private->user_result_vol;
    }
    if(current){
        *current = this->_private->user_result_cur;
    }
    if (freq_measured){
        *freq_measured = this->_private->user_freq;
    }
    std::cout<< "get test result witch U="<< this->_private->user_vol << " freq=" << this->_private->user_freq
             << " I=" << this->_private->user_cur << std::endl;
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


int ctpt_ctrl::get_ctpt_result(ctpt_result &out)
{
    if (!this->_private->is_stopped || this->_private->npoint_exciting == 0) {
        out.npoint_exciting = 0;
        return -1;
    }

    printf("get ctpt result\n");

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

    out.npoint_exciting                     = this->_private->npoint_exciting;
    // std::cout<< "out.npoint_exciting=" << out.npoint_exciting << std::endl;
    if (this->_private->npoint_exciting) {
        printf("get exciting point:%d\n", this->_private->npoint_exciting);
        memcpy(out.exciting_current,
               this->_private->exciting_current,
               this->_private->npoint_exciting*sizeof(this->_private->exciting_current[0]) );
        memcpy(out.exciting_voltage,
               this->_private->exciting_voltage,
               this->_private->npoint_exciting*sizeof(this->_private->exciting_voltage[0]) );
    }
    out.knee_point_voltage                  = this->_private->result.knee_point_voltage; // 拐点电压
    out.knee_point_current                  = this->_private->result.knee_point_current; // 拐点电流

    for(int i=0; i<this->_private->result.npoint_error5; i++)
    {
        out.error5_resistance[i]            = this->_private->result.error5_resistance[i];
        out.error5_factor[i]                = this->_private->result.error5_factor[i];
    }
    for(int i=0; i<this->_private->result.npoint_error10; i++)
    {
        out.error10_factor[i]               = this->_private->result.error10_factor[i];
        out.error10_resistance[i]           = this->_private->result.error10_resistance[i];
    }

//    // 5%误差曲线
    out.npoint_error5                       = this->_private->result.npoint_error5;
//    double *error5_resistance;  // 负载电阻
//    double *error5_factor;      // 5%误差限值

//    // 10%误差曲线
    out.npoint_error10                      = this->_private->result.npoint_error10;
//    double *error10_resistance;  // 负载电阻
//    double *error10_factor;      // 10%误差限值

    //copy result to out
    out.ratio = this->_private->result.ratio;

    return 0;
}
