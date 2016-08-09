

#include <math.h>

#include "functions.hpp"


double calc_composite_current( double n, double ip, double is, double alpha )
{
    double ic;
    double is2;
    double ipn2;
    dobule xy2;

    is2 = is * is;
    ipn2 = ip/n;
    ipn2 = ipn2*ipn2;
    xy2 = 2 * is * ip/n * cos(alpha);
    ic = sqrt( is2 + ipn2 - xy2 );
    return ic;
}

double calculate_fs( double n, double ip, double is, double ipsc,  )
{
    
}


