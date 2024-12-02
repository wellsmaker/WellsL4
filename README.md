# wells for L4
The L4 microkernel follows the l4.x2.rev interface standard

本项目参考Zephyr和seL4，尝试将微内核架构引入到微控制器领域，并有意解决微内核普遍存在的性能问题，采取一系列优化机制，例如fastIPC，Shmem等。接口文档参考l4-x2.pdf。

This project, with reference to Zephyr and seL4, attempts to introduce microkernel architecture into the field of microcontrollers, and intentionally solves the performance problems common in microcores by adopting a series of optimization mechanisms, such as fastIPC, Shmem, etc. Interface documentation refer to l4-x2.pdf.

# install

环境：Ubuntu20.04或以上

```
sudo apt install gperf cmake ninja-build gcc-arm-none-eabi

export WELLSL4_ROOT=${workspaceFolder}
export WELLSL4_BASE=${workspaceFolder}/wellsl4
export WELLSL4_TOOLCHAIN_VARIANT=cross-compile
export CROSS_COMPILE=arm-none-eabi-

cd ${workspaceFolder}/project/test1

${workspaceFolder}为WellsL4-OS的顶层目录，最好是绝对地址
```

示例1：
```
cmake -S . -Bbuild -DBOARD=stm32f429i_disc1 -DTOOLCHAIN_HOME=/usr/bin -GNinja
cd build
ninja
```

示例二：
```
cmake -S . -Bbuild -DBOARD=stm32f429i_disc1 -DTOOLCHAIN_HOME=/usr/bin
cd build
make
```

最后将build/wellsl4目录下的wellsl4.bin烧写到开发板上即可验证，开发板为STM32F429 Discovery Kit
