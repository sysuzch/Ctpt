/**
 *  @file          wave-signal.h
 *  @date          Time-stamp: <2014-07-08 07:38:01, by lyzh>
 *  @brief         
 *  @version       1.0.0
 *  @author        Liangyaozhan
 *  @copyright     guirock team
 *                 ivws02@126.com
 */


#ifndef __WAVE_SIGNAL_H__
#define __WAVE_SIGNAL_H__


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif                                                                  /* __cplusplus                  */

#define USE_CSV_FILE    1


struct wave_signal
{
    double *p;
    double scale;
    double fs;                          /*!  sample_rate */
    int npoint;
};

struct system_wave_signal
{
    struct wave_signal u_a;
    struct wave_signal u_b;
    struct wave_signal u_c;
    struct wave_signal u_n;

    struct wave_signal i_a;
    struct wave_signal i_b;
    struct wave_signal i_c;
    struct wave_signal i_n;

};

void wave_signal_init( struct wave_signal *_this );
void wave_signal_set( struct wave_signal *_this, double scale, double sample_rate, int npoint );
void wave_signal_add( struct wave_signal *_this, double amp, double dc, double freq, double angle_deg );
void wave_signal_clear( struct wave_signal *_this );
void wave_signal_get_float( struct wave_signal *_this, float *f );
double wave_signal_get_rms( struct wave_signal *_this );
double wave_signal_get_rms_skip( struct wave_signal *_this, int skip );
double wave_signal_get_line_rms( struct wave_signal *_this, struct wave_signal *ref );
void wave_signal_get_double( struct wave_signal *_this, double *d );
void wave_signal_get_int16( struct wave_signal *_this, int16_t *o );
void wave_signal_destroy( struct wave_signal *_this );

void system_wave_signal_init( struct system_wave_signal *_this );
void system_wave_signal_uset( struct system_wave_signal *_this, double scale, double sample_rate, int npoint );
void system_wave_signal_iset( struct system_wave_signal *_this, double scale, double sample_rate, int npoint );
#if USE_CSV_FILE>0
int system_wave_signal_write_csv( struct system_wave_signal *_this, const char *filename );
#endif
void system_wave_signal_destroy( struct system_wave_signal *_this );

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __WAVE-SIGNAL_H__           */
