

#include "calculator.hpp"
#include "raw_wave.hpp"

class calculator_private
{
public:
    calculator_private();
    virtual ~calculator_private();
};


calculator::calculator():name("unkown"), _private(NULL)
{
}

calculator::calculator( const char *m ): name(m), _private(NULL)
{
}

bool calculator::reset( void )
{
    return 0;
}
