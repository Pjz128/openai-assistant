rm -rf build
mkdir build && cd build
cmake ..
make -j 10
make install