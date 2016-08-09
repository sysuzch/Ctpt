
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fpga.hpp"
#include "raw_wave.hpp"
#include <stdio.h>
#include <vector>



#define SHF(n) printf(#n "=%f\n", n)
#define SHI(n) printf(#n "=%d\n", n)

fpga_dev fpga;

static void at_exit1(void)
{
    printf("at_exit stopped.\n");
    //fpga.stop_output();
    //fpga.adc_stop_sample();

    fflush(stdout);
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


/***************************************************
 * *
 * * Test the range selection
 * *
 * **************************************************/
void test_range(void)
{
    int channel = -1;
    float range = 100;
    bool isStop = false;

    fpga.init(0, 4 );
    fpga.dc_ac_f_set( 50 );
    fpga.set_np_per_cycle( 512 );

    while(!isStop)
    {
        std::cout<<"input the channel and range, exit when channel = -2. for example: 1 10"<<std::endl;
        std::cin>>channel>>range;

         if(channel == -2)
            break;

        if((channel<0 || channel >3)&&(channel !=-2))
        {
            std::cout<<"invalid channel input"<<std::endl;
            continue;
        }


        printf("the input channel is %d and range is %f \n",channel,range);

        fpga.select_range_by_value(channel, range);

    }

}




double test_rang = 1.0;
void user_test(double maxI, double maxV, double start_v, double freq, int cycle)
{
#if 1
    double step_v = 0.1;
    raw_wave ch1;               // load current
    raw_wave ch2;               // src voltage
    raw_wave ch3;               // primary voltage
    raw_wave ch4;               // load voltage
    int ret = 0;

    if (maxI > 4.0) {
        maxI = 4.0;
    }
    if (maxV > 120.0) {
        maxV = 120.0;
    }

    fpga.init(0, cycle );
    fpga.dc_ac_f_set( freq );
    fpga.set_np_per_cycle( 512 );

    fpga.select_range_by_value(2, 30.0);
    fpga.adc_start_sample();

    struct test_result result;
    std::vector<struct test_result> vector_result;

    fpga.start_output();
    do {
        printf("++set_v %f++\n", start_v);
        if (start_v < 1.0) {
            step_v = 1;
        }
        else if (start_v < 20.0) {
            step_v = 2;
        }
        else if (start_v > 50.0) {
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
        vector_result.push_back(result);
        if (result.vol >= maxV || result.cur >= maxI) {
            break;
        }
        start_v += step_v;
    } while(1);

    printf("vol\tcur\n");
    for(size_t i = 0; i < vector_result.size(); i++) {
        printf("%8.4f,\t%8.4f\n",vector_result[i].vol, vector_result[i].cur);
    }

#else
    raw_wave ch4;               // load voltage
    fpga.init(0, 200.0, cycle );
    fpga.select_range_by_value(fpga_dev::PRI_VOL_CHNL, test_rang);
    printf("++++%f+++\n",test_rang);
    fpga.adc_start_sample();
    sleep(1);
    for(int i = 0; i<3; i++)
        fpga.read(0, 0, &ch4, 0);

#endif
}

int main(int argc, char *argv[])
{
    int   cycle   = 4;
    int   delay   = 10;
    float freq    = 50;
    float   range1  = 100.0;
    float   range2  = 1000;
    float   range3  = 10;
    float   range4  = 1000;
    float voltage = 1.0;
    int silent = 0;
    int demagnetization = 1;

    float maxI = 1.0;
    float maxV = 10;

    int userTest = 1;

    int ret = -1;

    atexit(at_exit1);

    printf("example: ./fpga -d 21 -f 51.5 -v 4.0 -D 1 -u 1 -MI 1.0 -MV 10.0 >data.csv");

    printf("default:\n");
    SHI(cycle);
    SHI(delay);
    SHF(freq);
    SHF(range1);
    SHF(range2);
    SHF(range3);
    SHF(range4);
    SHF(voltage);
    SHI(silent);
    SHI(demagnetization);
    SHF(maxI);
    SHF(maxV);
    SHI(userTest);


    printf("\nafter:\n");

#define ISP_float(var,flag)                     \
    do {                                        \
        if (0==strcmp(flag,argv[i])) {          \
            sscanf(argv[i+1], "%f", &var);      \
        }                                       \
    }while (0)


#define ISP_int(var,flag)                       \
    do {                                        \
        if (0==strcmp(flag,argv[i])) {          \
            var = atoi(argv[i+1]);              \
        }                                       \
    }while (0)

    for (int i=0; i<argc; i++) {
        ISP_int(cycle,  "-c");
        ISP_int(delay,  "-d");
        ISP_int(silent,  "-s");
        ISP_float(freq,   "-f");
        ISP_float(voltage,"-v");
        ISP_float(range1, "-R1");
        ISP_float(range2, "-R2");
        ISP_float(range3, "-R3");
        ISP_float(range4, "-R4");
        ISP_int(demagnetization,  "-D");
        ISP_float(maxI, "-MI");
        ISP_float(maxV, "-MV");
        ISP_int(userTest,  "-u");
    }
    SHI(cycle);
    SHI(delay);
    SHF(freq);
    SHF(range1);
    SHF(range2);
    SHF(range3);
    SHF(range4);
    SHF(voltage);
    SHI(silent);
    SHI(demagnetization);
    SHF(maxI);
    SHF(maxV);
    SHI(userTest);

    test_rang = range3;

    if (!userTest) {
#if 1
        fpga.init(0, cycle );

        // 4Hz ~ 300Hz
        fpga.dc_ac_f_set( freq );
        fpga.set_np_per_cycle( 512 );

    #if 1
        fpga.select_range_by_value(0, range1);
        fpga.select_range_by_value(1, range2);
        fpga.select_range_by_value(2, range3);
        fpga.select_range_by_value(3, range2);
    #else
    #   if !EN_PRI_VOL_CHNL_ADAPTIVE
            fpga.select_range_by_value(2, 30.0);
    #   endif
    #endif

        fpga.start_output();
        fpga.safe_output(voltage);
        fpga.adc_start_sample();

        ret = fpga.wait_output_steaby();
        printf("wait_for_steaby_output %d\n", ret);
#endif
        //fpga.auto_select_range(true, false, true, true);

        sleep(10);
        printf("Haha,I'm back!");
#if 0
{
        printf("will me display?");
        raw_wave ch1;               // secondary current
        raw_wave ch2;               // secondary voltage
        raw_wave ch3;               // secondary voltage (connect point)
        raw_wave ch4;               // primary   voltage
        for (int i=0; i<delay; i++) {

            ret = fpga.read( &ch1, &ch2, &ch3, &ch4 );

            ch1.calculate_all();
            ch2.calculate_all();
            ch3.calculate_all();
            ch4.calculate_all();

            printf( "index%d,  \tIload, \tUsrc,  \tUpri , \tUload\n"
                    "rms:,     \t%8.4f, \t%8.4f, \t%8.4f, \t%8.4f\n"
                    "rmsbase:, \t%8.4f, \t%8.4f, \t%8.4f, \t%8.4f\n"
                    "thd1:,    \t%8.4f, \t%8.4f, \t%8.4f, \t%8.4f\n",
                    i,
                    ch1.get_rms(),
                    ch2.get_rms(),
                    ch3.get_rms(),
                    ch4.get_rms(),

                    ch1.get_base_rms(),
                    ch2.get_base_rms(),
                    ch3.get_base_rms(),
                    ch4.get_base_rms(),

                    ch1.get_thd(),
                    ch2.get_thd(),
                    ch3.get_thd(),
                    ch4.get_thd()
                );
            sleep(1);
        }

        fpga.stop_output();
        fpga.adc_stop_sample();

        std::cout << "ret=" << ret << " rms:" << ch1.get_rms() << std::endl;

        std::cout << "newer version end get_np() = " << ch1.get_np() << std::endl;
        int n = ch1.get_np();
        std::vector<double> &chd1 = ch1.get_writable_buffer();
        std::vector<double> &chd2 = ch2.get_writable_buffer();
        std::vector<double> &chd3 = ch3.get_writable_buffer();
        std::vector<double> &chd4 = ch4.get_writable_buffer();
        if (!silent){
            printf("ch1,ch2,ch3,ch4\n");
            for (int i=0; i<n; i++ ) {
                printf("%6.4f, %6.4f, %6.4f, %6.4f\n ",
                       chd1[i],
                       chd2[i],
                       chd3[i],
                       chd4[i]
                    );
            }

        }

}
#endif

    }
    else {
        //user_test(maxI, maxV, voltage, freq, cycle);
        test_range();
    }

#if 0
    for (int i=0; i<n; i++ ){
        if (i%16 == 0){
            printf("\n");
        }
        printf("%6.1f ", chx[i] );
    }

    {
        printf("ch2:\n");
        std::vector<double> &chx = ch2.get_writable_buffer();
        for (int i=0; i<n; i++ ){
            if (i%16 == 0){
                printf("\n");
            }
            printf("%6.1f ", chx[i] );
        }
    }
    {
        printf("ch3:\n");
        std::vector<double> &chx = ch3.get_writable_buffer();
        for (int i=0; i<n; i++ ){
            if (i%16 == 0){
                printf("\n");
            }
            printf("%6.1f ", chx[i] );
        }
    }

    {
        printf("ch4:\n");
        std::vector<double> &chx = ch4.get_writable_buffer();
        for (int i=0; i<n; i++ ){
            if (i%16 == 0){
                printf("\n");
            }
            printf("%6.1f ", chx[i] );
        }
    }
#endif

    fflush( stdout );

    return 0;
}
