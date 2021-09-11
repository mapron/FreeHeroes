originalDir=`pwd`
mkdir -p build && cd build
cmake -DLUPDATE_TOOL=lupdate -P ../cmake/translationsRelease.cmake
cd $originalDir
