
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fpga.hpp"
#include "raw_wave.hpp"
#include <stdio.h>
#include <vector>


extern  bool is_outoff_range(double v, const struct adc_range *range);


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

class CUserTest {
public:
    CUserTest();
    ~CUserTest();

    bool isFinished() {return m_bFinished;}
    bool isStarted() {return m_bADCStarted;}

    void printMenu();
    void exeMenu(char menuItem);
    void displayResult();

private:
    void printRange(int chnl);

    void setRange();
    void startADC();
    void stopADC();
    void exitTest();

    
    void setResultType();

    void get_rms();

    void testOutput();

    
private:
    bool m_bFinished;
    bool m_bADCStarted;

    int  m_resultType;

};

CUserTest::CUserTest()
    : m_bFinished(false)
    , m_bADCStarted(false)
    , m_resultType(0)
{

}

CUserTest::~CUserTest()
{

}

void CUserTest::printMenu()
{
    std::cout << "please select your menu" << std::endl;
    std::cout << "1 -> Set Range for AD Channel" << std::endl;
    std::cout << "2 -> Start AD Sample" << std::endl;
    std::cout << "3 -> Stop AD Sample" << std::endl;
    std::cout << "4 -> Set Display Result" << std::endl;
	std::cout << "5 -> Test Output volte"<<std::endl;
    std::cout << "E -> Exit" << std::endl;
}

void CUserTest::exeMenu(char menuItem)
{
    switch(menuItem)  {
    case '1':
        setRange();
        break;
    case '2':
        startADC();
        break;
    case '3':
        stopADC();
        break;
    case '4':
        setResultType();
        break;
    case '5':
        testOutput();
        break;
    case 'e':
    case 'E':
        exitTest();
        break;
    default:
        break;
    }
}

void CUserTest::displayResult()
{
    get_rms();
}

void CUserTest::printRange(int chnl)
{
    if (chnl == fpga_dev::LOAD_CUR_CHNL) {
        const struct adc_range* range = fpga.adc_range_info(fpga_dev::LOAD_CUR_CHNL);
        printf("0: load Cur (reg:0x%x range_min:%6.4f range_max:%6.4f scale:%6.4f\n",
               range->fpga_reg_value, range->range_min, range->range_max, range->scale);
    }
    else if (chnl == fpga_dev::SRC_VOL_CHNL) {
        const struct adc_range* range = fpga.adc_range_info(fpga_dev::SRC_VOL_CHNL);
        printf("1: src Vol (reg:0x%x range_min:%6.4f range_max:%6.4f scale:%6.4f \n",
               range->fpga_reg_value, range->range_min, range->range_max, range->scale);
    }
    else if (chnl == fpga_dev::PRI_VOL_CHNL) {
        const struct adc_range* range = fpga.adc_range_info(fpga_dev::PRI_VOL_CHNL);
        printf("2: pri Vol (reg:0x%x range_min:%6.4f range_max:%6.4f scale:%6.4f\n",
               range->fpga_reg_value, range->range_min, range->range_max, range->scale);
    }
    else if (chnl == fpga_dev::LOAD_VOL_CHNL) {
        const struct adc_range* range = fpga.adc_range_info(fpga_dev::LOAD_VOL_CHNL);
        printf("3: load Vol (reg:0x%x range_min:%6.4f range_max:%6.4f scale:%6.4f\n",
               range->fpga_reg_value, range->range_min, range->range_max, range->scale);
    }
}

void CUserTest::setRange()
{
    for(int chnl = 0; chnl < fpga_dev::MAX_CHNL_NUM; chnl++) {
        printRange(chnl);
    }

    int chnl;
    float val;
    std::cout << "please enter chnl num and vol/cur rms value:";
    std::cin >> chnl >> val;

    if (chnl < 0 || chnl >= fpga_dev::MAX_CHNL_NUM) {
        std::cout << "AD Channel Error" << std::endl;
    }
    else {
        fpga.select_range_by_value(chnl, val);
        printRange(chnl);
    }
}

void CUserTest::startADC()
{
    fpga.adc_start_sample();
    m_bADCStarted = true;
    m_bFinished = true;
    std::cout << "start ADC" << std::endl;
}

void CUserTest::stopADC()
{
    fpga.adc_stop_sample();
    m_bADCStarted = false;
    fpga.select_range_by_value(0, 10.0);
    fpga.select_range_by_value(1, 100.0);
    fpga.select_range_by_value(2, 100.0);
    fpga.select_range_by_value(3, 100.0);
    std::cout << "stop ADC" << std::endl;
}

void CUserTest::exitTest()
{
    stopADC();
    m_bFinished = true;
}

void CUserTest::setResultType()
{
    std::cout << "0 -> Display RMS" << std::endl;
}

void CUserTest::testOutput()
{
    int ret = 0;
    bool isExit = false;
    double v;
    
    raw_wave ch1;               // load current
    raw_wave ch2;               // src voltage
    raw_wave ch3;               // primary voltage
    raw_wave ch4;               // load voltage
            
 
    fpga.adc_start_sample();
    fpga.start_output();
    while(!isExit)
    {
	std::cout << "Please input the Output Volate('-1' for exit test!!):" << std::endl;
	std::cin>>v;
	if(v  == -1)
	{
	    isExit = true;
	}
	if(v<0 || v> 120)
	{
	    printf("Input out of range\n");
	    continue;
	}
	
	ret = fpga.safe_output(v, true);
	if (ret != 0) {
	    break;
	}
	printf("++wait_output_steaby++\n");
	ret = fpga.wait_output_steaby();
	if (ret != 0) {
	    break;
	}
	for(int i = 0; i<10; i++)
	{
	    printf("++get_active_sample_wave++\n");
	    ret = fpga.get_active_sample_wave(&ch1, &ch2, &ch3, &ch4 );
	    if (ret != 0) {
		break;
	    } 
	    printf("chnl1:%f chnl2:%f chnl3:%f chnl4:%f------\n", ch1.get_base_rms(), ch2.get_base_rms(), ch3.get_base_rms(), ch4.get_base_rms());
	    sleep(1);
	}
    }
    while(1)
    {
	printf("press CTRL+C to stop test\n");
	fpga.stop_output();
        fpga.adc_stop_sample();
	sleep(2);
    }
    
}

void CUserTest::get_rms()
{
    if (isStarted()) {
        raw_wave wave_i_load;
        raw_wave wave_u_src;
        raw_wave wave_u_pri;
        raw_wave wave_u_load;

        fpga.read(&wave_i_load, &wave_u_src, &wave_u_pri, &wave_u_load);
        wave_i_load.calculate_rms();
        wave_u_src.calculate_rms();
        wave_u_pri.calculate_rms();;
        wave_u_load.calculate_rms();


        float rms[4];
        rms[0] = wave_i_load.get_rms();
        rms[1] = wave_u_src.get_rms();
        rms[2] = wave_u_pri.get_rms();
        rms[3] = wave_u_load.get_rms();

        int n = wave_u_src.get_np();
        std::vector<double> & chd1= wave_i_load.get_writable_buffer();
        std::vector<double> & chd2= wave_u_src.get_writable_buffer();
        std::vector<double> & chd3 = wave_u_pri.get_writable_buffer();
        std::vector<double> & chd4 = wave_u_load.get_writable_buffer();


        float max, min;
        max = min = chd1[0];
        for(int i = 0; i < n; i++) {
            if (chd1[i] > max) {
                max = chd1[i];
            }
            if (chd1[i] < min) {
                min = chd1[i];
            }
        }
        printf("load cur -> rms:%6.4f max:%6.4f min:%6.4f\n", rms[0], max, min);

        if (is_outoff_range(rms[0], fpga.adc_range_info(fpga_dev::LOAD_CUR_CHNL))) {
            printf("-----------------------\n");
            //fpga.adc_stop_sample();
            fpga.select_range_by_value(0, rms[0]);
            int n = 0;
            while(n < 10) {
                fpga.read(&wave_i_load, &wave_u_src, &wave_u_pri, &wave_u_load);
                wave_i_load.calculate_rms();
                wave_u_src.calculate_rms();
                wave_u_pri.calculate_rms();;
                wave_u_load.calculate_rms();
                float rms = wave_i_load.get_rms();
                printf("rms: %6.4f \n", rms);
                usleep(50000);
                n++;
            }
            //fpga.adc_start_sample();
        }

//        max = min = chd2[0];
//        for(int i = 0; i < n; i++) {
//            if (chd2[i] > max) {
//                max = chd2[i];
//            }
//            if (chd2[i] < min) {
//                min = chd2[i];
//            }
//        }
//        printf("src vol -> rms:%6.4f max:%6.4f min:%6.4f\n", rms[1], max, min);

//        max = min = chd3[0];
//        for(int i = 0; i < n; i++) {
//            if (chd3[i] > max) {
//                max = chd3[i];
//            }
//            if (chd3[i] < min) {
//                min = chd3[i];
//            }
//        }
//        printf("pri vol -> rms:%6.4f max:%6.4f min:%6.4f\n", rms[2], max, min);

//        max = min = chd4[0];
//        for(int i = 0; i < n; i++) {
//            if (chd4[i] > max) {
//                max = chd4[i];
//            }
//            if (chd4[i] < min) {
//                min = chd4[i];
//            }
//        }
//        printf("load vol -> rms:%6.4f max:%6.4f min:%6.4f\n", rms[3], max, min);

        printf("\n");
    }
}

 
double test_rang = 1.0;
void user_test()
{

    printf("begin test\n");

    fpga.init(0, 4 );
    fpga.dc_ac_f_set( 50 );
    fpga.set_np_per_cycle( 512 );

    fpga.select_range_by_value(0, 10.0);
    fpga.select_range_by_value(1, 100.0);
    fpga.select_range_by_value(2, 100.0);
    fpga.select_range_by_value(3, 100.0);

    CUserTest tester;

    char menu = -1;
    do {
        tester.printMenu();
        std::cin >> menu;
        tester.exeMenu(menu);
    }while(!tester.isFinished());

    while(tester.isStarted()) {
        tester.displayResult();
        sleep(1);
    }

    printf("end test\n");
}

int main(int argc, char *argv[])
{
    user_test();

    fflush( stdout );

    return 0;
}
