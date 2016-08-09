

export PATH=/home/lyzh/bin/gcc-linaro-arm-linux-gnueabihf-4.8-2014.02_linux/bin:$PATH

CC=arm-linux-gnueabihf-gcc CFLAGS="-fPIC" ../fftw-3.3.4/configure --prefix=$PWD/install --enable-static --host=arm-linaro-linux-gnueabi





