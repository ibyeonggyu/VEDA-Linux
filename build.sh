#!/bin/bash

set -e

# ************************ */
# 사용자 변경 시 데이터 변경 */
RPI_USER=aposeseco
RPI_IP=100.65.223.14
RPI_DEST=/home/aposeseco/Project
# ************************ */

if [ "$1" == "clean" ]; then
    echo "🧹 Cleaning build directory..."
    rm -rf build_server
    rm -f client_exe

    echo "✨ Clean complete!"
    exit 0
fi

echo "Building Project..."

echo "==============================================="
echo "🔨 [Client] Build..."
echo "==============================================="
gcc -o client_exe client/main.c

echo "==============================================="
echo "🚀 [Server] Build for RaspberryPi4..."
echo "==============================================="
mkdir -p build_server && cd build_server
cmake -DCMAKE_TOOLCHAIN_FILE=../raspberrypi4_toolchain.cmake ..
cmake --build .
cd ..

echo "==============================================="
echo "🚚 Sending files to Raspberry Pi4..."
echo "==============================================="

ssh ${RPI_USER}@${RPI_IP} "mkdir -p ${RPI_DEST}"

mkdir -p temp
cp build_server/server/server_exe temp/
cp build_server/devices/*.so temp/
scp -r temp/* ${RPI_USER}@${RPI_IP}:${RPI_DEST}/
rm -rf temp

echo "==============================================="
echo "✅ Sent to Raspberry Pi4(${RPI_USER}@${RPI_IP}:${RPI_DEST}/) successfully!"
echo "✨ Build & Transfer Complete!"
echo "==============================================="
