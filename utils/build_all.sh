#!/bin/sh

function check_retval() {
	if [ $? -ne 0 ]; then
		echo "BUILD FAILED"
		cd "$my_dir"
		exit 1
	fi
}

script_dir=$(dirname "$(readlink -e "$0")")
my_dir=$(pwd)


echo "Installing libopencm3"
mkdir -p "${script_dir}/../build/install"
check_retval

"${script_dir}/../libsrc/mouros/build_libopencm3.sh" "${script_dir}/../build/install"
check_retval

echo
echo "Building Ratfist - STM32F072 Discovery Debug"
mkdir -p "${script_dir}/../build/ratfist-stm32f0-debug"
check_retval

cd "${script_dir}/../build/ratfist-stm32f0-debug"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DBOARD_TYPE=stm32f072discovery" "-DCHIP_FAMILY=STM32F0" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/arm-none-eabi-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Debug" "-DINCLUDE_SPINNER=1" ../..
check_retval

cmake --build .
check_retval

cmake --build . --target ratfist-doc
check_retval

cmake --build . --target install
check_retval



echo
echo "Building Ratfist - STM32F072 Discovery Release"
mkdir -p "${script_dir}/../build/ratfist-stm32f0-release"
check_retval

cd "${script_dir}/../build/ratfist-stm32f0-release"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DBOARD_TYPE=stm32f072discovery" "-DCHIP_FAMILY=STM32F0" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/arm-none-eabi-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Release" "-DINCLUDE_SPINNER=1" ../..
check_retval

cmake --build .
check_retval

cmake --build . --target ratfist-doc
check_retval

cmake --build . --target install
check_retval


echo
echo "Building Ratfist - STM32F411 Discovery Debug"
mkdir -p "${script_dir}/../build/ratfist-stm32f4-debug"
check_retval

cd "${script_dir}/../build/ratfist-stm32f4-debug"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DBOARD_TYPE=stm32f411discovery" "-DCHIP_FAMILY=STM32F4" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/arm-none-eabi-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Debug" "-DINCLUDE_SPINNER=1" ../..
check_retval

cmake --build .
check_retval

cmake --build . --target ratfist-doc
check_retval

cmake --build . --target install
check_retval


echo
echo "Building Ratfist - STM32F411 Discovery Release"
mkdir -p "${script_dir}/../build/ratfist-stm32f4-release"
check_retval

cd "${script_dir}/../build/ratfist-stm32f4-release"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DBOARD_TYPE=stm32f411discovery" "-DCHIP_FAMILY=STM32F4" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/arm-none-eabi-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Release" "-DINCLUDE_SPINNER=1" ../..
check_retval

cmake --build .
check_retval

cmake --build . --target ratfist-doc
check_retval

cmake --build . --target install
check_retval



echo
echo "Building Ratfist - Unit Tests - STM32F072 Discovery"
mkdir -p "${script_dir}/../build/ratfist-tests-stm32f072"
check_retval

cd "${script_dir}/../build/ratfist-tests-stm32f072"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/gcc-linux-x86-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Debug" "-DBOARD_TYPE=stm32f072discovery" ../../tests
check_retval

cmake --build .
check_retval

ctest -V .
check_retval

cmake --build . --target coverage
check_retval



echo
echo "Building Ratfist - Unit Tests - STM32F411 Discovery"
mkdir -p "${script_dir}/../build/ratfist-tests-stm32f411"
check_retval

cd "${script_dir}/../build/ratfist-tests-stm32f411"
check_retval

cmake "-DCMAKE_INSTALL_PREFIX=../install" "-DCMAKE_TOOLCHAIN_FILE=../../libsrc/mouros/cmake/gcc-linux-x86-toolchain.cmake" "-DCMAKE_BUILD_TYPE=Debug" "-DBOARD_TYPE=stm32f411discovery" ../../tests
check_retval

cmake --build .
check_retval

ctest -V .
check_retval

cmake --build . --target coverage
check_retval



echo "Successfully built Ratfist & tests"
