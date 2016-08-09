

#ifndef __CTPT_API_H
#define __CTPT_API_H

#ifdef WIN32
#ifndef CPTAPI_DLL
#define CPTAPI __declspec(dllimport)
#else
#define CPTAPI __declspec(dllexport)
#endif
#else
#define CPTAPI
#endif


enum user_test_mode {
    USER_TEST_NONE = 0,
    USER_TEST_SELFCHECK,  //用户自测
    USER_TEST_EXCITING    //自测励磁曲线
};

enum ctpt_type
{
    TYPE_CT,
    TYPE_PT
};

enum ctpt_test_item
{   
    ITEM_RESISTANCE = 1,
    ITEM_EXCITATION = 0x02,
    ITEM_RATIO = 0x04,
    ITEM_BURDEN = 0x08,

};

enum ctpt_class
{
    CTC_P = 1,
    CTC_TPY,
    CTC_MEASURE,                // 量
    CTC_PR,
    CTC_PX,
    CTC_TPS,
    CTC_TPX,
    CTC_TPZ,
};

// 工作循环   WL_C_T1_O_Tfr_C_T2_O,
struct work_loop_WL_C_T1_O_Tfr_C_T2_O
{
    bool   is_valid;
    double t1;
    double tal1;
    double tfr;
    double t2;
    double tal2;
};

// 工作循环
struct work_loop_WL_C_T1_O
{
    bool is_valid;
    double t1;
    double tal1;
};

struct ct_setting
{
    /// {common
    enum ctpt_type type;
    enum ctpt_test_item test_item;

    char line[256];           // 线路名称
    char phase[256];          // 相名称
    char ct_number[256];      // CT 编号
    char winding_number[256]; // 绕组号

    double rated_secondary_current; // 额定二次电流
    enum ctpt_class accuracy_class;     // 级别

    double temp_current;        // 当前温度
    double rated_frequency;     // 额定频率
    double max_current;         // 最大测试电流

    double rated_primary_current;       // 额定一次电流, in
    double rated_burden;                // 额定负荷, in
    double power_factor;                //功率因数
    double rated_accuracy_limit_factor; // P ALF额定准确值系数
    /// }

    double dimensioning_factor;  // 计算系数， PX

    double rated_short_current_factor; // 额定对称短路电流系数
    double area_factor;          // TPY 暂态面积系数
    double primary_time_constant;   //
    double secondary_time_constant;

    struct work_loop_WL_C_T1_O_Tfr_C_T2_O wl1;
    struct work_loop_WL_C_T1_O wl2;
};


struct pt_setting
{
    /// {common
    enum ctpt_type type;
    enum ctpt_test_item test_item;

    char line[256];           // 线路名称
    char phase[256];          // 相名称
    char ct_number[256];      // CT 编号
    char winding_number[256]; // 绕组号

    double rated_secondary_voltage; // 额定二次
    enum ctpt_class ctpt_class;   // 级别

    double temp_current;        // 当前温度
    double rated_frequency;                  // 额定频率, in
    double max_voltage;                      // 最大测试电压
    double max_current;                      // 最大测试电流
};

struct ctpt_result
{
    CPTAPI ctpt_result();
    CPTAPI ~ctpt_result();

    // 负载, 电阻, 内阻
    double rated_resitive_burden;            // 额定电阻负荷, in
    double secondary_winding_resistance;     // 二次绕组电阻, out  --ok

    // 励磁
    double knee_point_voltage; // 拐点电压
    double knee_point_current; // 拐点电流
    double secondary_limiting_emf; // 二次极限感应电势
    double inductance; // 饱和电感
    double inductance_unsaturated; // 不饱和电感
    double rated_secondary_loop_time_constant; // 额定二次回路时间常数 Ts = Ls / Rs, out
    double composite_error;        // 复合误差
    double remanence_factor;    // 剩磁系数,

    double accuracy_limit_factor; // P,PR 准确限值系数

    double rated_knee_point_emf; // PX 额定拐点电势, Ek
    double rated_knee_point_current; // PX 额定拐点电势, ie_ek

    double kare_static;         // TPY，TPX,TPZ 暂态面积系数
    double kssc; // TPY，TPS,TPX,TPZ 所得励磁电流I_exc与额定二次电流I_sn和仪表保安系数FS乘积之比用百分数表示时，其值应等于或者大于额定复合误差限值，即10%
    double peek_error;          // TPY，TPX,TPZ 峰瞬误差

    double instrument_security_factor; // 计量类/measured 仪表保安系数

    double dimensioning_factor;  // 计算系数， PX

    double u_al;    // TPS 额定Ual
    double i_al_u_al;// TPS Ual对应的Ial

    // 变比
    double ratio;                    // 变比
    double ratio_left;               // 变比,分子
    double ratio_right;              // 变比,分母
    double ratio_error;         // 比值差
    double turns_ratio;    // 额定匝数比
    double turns_ratio_left;    // 额定匝数比,,分子
    double turns_ratio_right;    // 额定匝数比,,分母
    double turns_ratio_error;    // 匝数比误差
    double phase_error;          // 相位差
    bool   phase_polar;          // 极性

    // 励磁曲线
    int npoint_exciting;
    double *exciting_current;   // x coordinate
    double *exciting_voltage;   // y coordinate

    // 5%误差曲线
    int npoint_error5;
    double *error5_resistance;  // 负载电阻
    double *error5_factor;      // 5%误差限值
    int knee_index;             // 拐点位置

    // 10%误差曲线
    int     npoint_error10;
    double *error10_resistance;  // 负载电阻
    double *error10_factor;      // 10%误差限值

    // 比值差
    int     ratio_error_npoint; // 下面每项目数据各自有多少点.只读
    double *ratio_error_va;     // VA
    double *ratio_error_factor; // cos(phy)
    double *ratio_error_1;      // 1% 视在功率的情况下
    double *ratio_error_5;      // 5% 视在功率的情况下
    double *ratio_error_10;     // 10% 视在功率的情况下
    double *ratio_error_20;     // 20% 视在功率的情况下
    double *ratio_error_100;    // 100% 视在功率的情况下

    // 相位差
    int     phase_error_npoint; // 下面每项目数据各自有多少点.只读
    double *phase_error_va;     // VA
    double *phase_error_factor; // cos(phy)
    double *phase_error_1;      // 1% 视在功率的情况下
    double *phase_error_5;      // 5% 视在功率的情况下
    double *phase_error_10;     // 10% 视在功率的情况下
    double *phase_error_20;     // 20% 视在功率的情况下
    double *phase_error_100;    // 100% 视在功率的情况下
};

class rt_listener_private;
class rt_listener
{
public:
    CPTAPI rt_listener();
    CPTAPI virtual ~rt_listener();

public:
    CPTAPI virtual void scanning( double voltage, double current );
    CPTAPI virtual void process( int percent );

private:
    rt_listener_private *reserved;
};


class ctpt_ctrl_private;
class ctpt_ctrl
{
public:
    CPTAPI ctpt_ctrl();
    CPTAPI virtual ~ctpt_ctrl();

public:

    // init, reinit
    CPTAPI int init( void );
    CPTAPI int reinit( void );

    // config
    CPTAPI int config( const struct ct_setting & ); // 配置CT
    CPTAPI int config( const struct pt_setting & ); // 配置PT
    CPTAPI int config( double voltage,
                       double current,
                       double freq,
                       user_test_mode testMode = USER_TEST_EXCITING); // 用户自测配置

    // 开始试验
    CPTAPI int start( void );

    // 结束
    CPTAPI int stop( void );

    // 获取测试结果
    CPTAPI int get_ctpt_result(ctpt_result &out);


    // 获取 用户自测结果
    // return 0 成功，无错误
    // return -1, 测试失败
    CPTAPI int get_user_test_result( double *voltage,
                                     double *current,
                                     double *freq_measured = 0);

    CPTAPI void register_real_time_listener( rt_listener *listener );

private:
    ctpt_ctrl_private *_private;
};

#endif /* __CTPT_API_H */

