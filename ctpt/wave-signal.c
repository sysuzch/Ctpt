/**
 *  @file          wave-signal.c
 *  @date          Time-stamp: <2014-07-08 07:37:49, by lyzh>
 *  @brief         
 *  @version       1.0.0
 *  @copyright     guirock team
 *                 ivws02@126.com
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "wave-signal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI  ( 3.1415926 )
#endif

void wave_signal_init( struct wave_signal *_this )
{
    memset( _this, 0x00, sizeof( *_this ) );
}


void wave_signal_set( struct wave_signal *_this, double scale, double sample_rate, int npoint )
{
    if ( _this->npoint != npoint )
    {
        if ( _this->p )
        {
            free( _this->p );
        }
        _this->p = ( double* )malloc( sizeof( *_this->p ) * npoint );
        if ( !_this->p )
        {
            return;
        }
    }
    _this->npoint = npoint;
    _this->fs     = sample_rate;
    _this->scale  = scale;
    wave_signal_clear( _this );
}


void wave_signal_add( struct wave_signal *_this, double amp, double dc, double freq, double angle_deg )
{
    double *p;
    int     i;
    int     n;
    double  w0 = angle_deg * M_PI / 180;
    double amp_ad;
    double fs = _this->fs;
    double x = M_PI * 2.0 * freq / fs;
    p = _this->p;
    n = _this->npoint;

    amp_ad = amp * sqrt( 2 )/_this->scale;
    dc /= _this->scale;
    for ( i=0; i<n; i++ )
    {
        *p++ += amp_ad * sin( x * i + w0 ) + dc;
    }
}


void wave_signal_clear( struct wave_signal *_this )
{
    memset( _this->p, 0x00, sizeof( *_this->p )*_this->npoint );
}

void wave_signal_get_float( struct wave_signal *_this, float *f )
{
    int i;
    int n = _this->npoint;
    double *p = _this->p;

    for ( i=0; i<n; i++)
    {
        *f++ = *p++;
    }
}

double wave_signal_get_rms( struct wave_signal *_this )
{
    int i;
    double s = 0;
    double *p = _this->p;
    int n = _this->npoint;
    
    for ( i=0; i<n; i++ )
    {
        double t = *p++;
        s += t * t;
    }
    return sqrt( s/n ) * _this->scale;
}

double wave_signal_get_rms_skip( struct wave_signal *_this, int skip )
{
    int i;
    double s = 0;
    double *p = _this->p + skip;
    int n = _this->npoint - skip;
    
    if ( n <= 0 )
    {
        return 0;
    }
    
    for ( i=0; i<n; i++ )
    {
        double t = *p++;
        s += t * t;
    }
    return sqrt( s/n ) * _this->scale;
}

double wave_signal_get_line_rms( struct wave_signal *_this, struct wave_signal *ref )
{
    int i;
    double s = 0;
    double *p = _this->p;
    double *pref = ref->p;
    double scale1 = _this->scale;
    double scale2 = ref->scale;
    int n = _this->npoint;
    
    for ( i=0; i<n; i++ )
    {
        double t = *p++ * scale1 - *pref++ * scale2;
        s += t * t;
    }
    return sqrt( s/n );
}

void wave_signal_get_double( struct wave_signal *_this, double *d )
{
    memcpy( d, _this->p, sizeof( *_this->p )*_this->npoint );
}

void wave_signal_get_int16( struct wave_signal *_this, int16_t *o )
{
    int i;
    int n = _this->npoint;
    double *p = _this->p;

    for ( i=0; i<n; i++)
    {
        *o++ = *p++;
    }    
}

void wave_signal_destroy( struct wave_signal *_this )
{
    if ( _this && _this->p )
    {
        free( _this->p );
        _this->p = NULL;
    }
}


void system_wave_signal_init( struct system_wave_signal *_this )
{
    memset( _this, 0x00, sizeof( *_this ) );
    wave_signal_init( &_this->u_a );
    wave_signal_init( &_this->u_b );
    wave_signal_init( &_this->u_c );
    wave_signal_init( &_this->u_n );
    wave_signal_init( &_this->i_a );
    wave_signal_init( &_this->i_b );
    wave_signal_init( &_this->i_c );
    wave_signal_init( &_this->i_n );
}

void system_wave_signal_uset( struct system_wave_signal *_this, double scale, double sample_rate, int npoint )
{
    wave_signal_set( &_this->u_a, scale, sample_rate, npoint );
    wave_signal_set( &_this->u_b, scale, sample_rate, npoint );
    wave_signal_set( &_this->u_c, scale, sample_rate, npoint );
    wave_signal_set( &_this->u_n, scale, sample_rate, npoint );
}

void system_wave_signal_iset( struct system_wave_signal *_this, double scale, double sample_rate, int npoint )
{
    wave_signal_set( &_this->i_a, scale, sample_rate, npoint );
    wave_signal_set( &_this->i_b, scale, sample_rate, npoint );
    wave_signal_set( &_this->i_c, scale, sample_rate, npoint );
    wave_signal_set( &_this->i_n, scale, sample_rate, npoint );
}

#if USE_CSV_FILE>0
#include <stdio.h>
int system_wave_signal_write_csv( struct system_wave_signal *_this, const char *filename )
{
    FILE *fp;
    int i;
    fp = fopen( filename, "w" );
    if ( !fp )
    {
        return 0;
    }
    fprintf( fp, "Ua,Ub,Uc,Un,Ia,Ib,Ic,In\n" );
    for ( i=0; i<_this->u_a.npoint && i<32768; i++ )
    {
        fprintf( fp, "%f,%f,%f,%f,%f,%f,%f,%f\n",
                 _this->u_a.p[ i ], _this->u_b.p[ i ], _this->u_c.p[ i ], _this->u_n.p[ i ],
                 _this->i_a.p[ i ], _this->i_b.p[ i ], _this->i_c.p[ i ], _this->i_n.p[ i ]  );
    }
    fprintf( fp, "\n" );
    fclose( fp );
    return 1;
}

#endif


void system_wave_signal_destroy( struct system_wave_signal *_this )
{
    wave_signal_destroy( &_this->u_a );
    wave_signal_destroy( &_this->u_b );
    wave_signal_destroy( &_this->u_c );
    wave_signal_destroy( &_this->u_n );
    wave_signal_destroy( &_this->i_a );
    wave_signal_destroy( &_this->i_b );
    wave_signal_destroy( &_this->i_c );
    wave_signal_destroy( &_this->i_n );
}

