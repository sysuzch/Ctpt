
#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <string>

class calculator_private;
class raw_wave;

class calculator
{
public:
    const std::string name;

public:
    calculator();
    calculator( const char *nm );
    ~calculator();

public:
    virtual bool reset( void );
    virtual int input( raw_wave *a, raw_wave *b, raw_wave *c = 0, raw_wave *d = 0 ) = 0;

private:
    calculator_private *_private;
};

#endif /* CALCULATOR_H */
