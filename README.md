# RTSP Camera Server

RTSP Server for USB 3.0 Industrial camera

To build use following command:
```shell
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=ninja -G Ninja -S . -B ./build
cmake --build ./build --target all -j 20
```
