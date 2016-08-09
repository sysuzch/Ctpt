
#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

// Ipsc 短路电流稳态值的有效值
static
inline
double get_i_psc( double u, double rp, double w, double lp )
{
    double i_psc;

    i_psc = u/(sqrt( rp*rp + w*w*lp*lp) );
    return i_psc;
}


double calc_composite_current( double n, );

double calculate_fs( double n, double ip, double is )


#endif /* FUNCTIONS_H */
