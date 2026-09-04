# STM32-WATCH

基于 STM32F103C8T6、STM32 HAL 和 CMake 的 128×64 OLED 手表固件。

当前固件已接通 SH1106、MPU6050、RTC、电池采样、三按键、电源保持和手电筒 LED，并实现首页、菜单、运动、水平仪、秒表、手电筒与设置页面。应用代码按 BSP、Components、Services、App、UI、Assets 分层，方便两人协作维护。

## 快速编译

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

烧录文件会自动生成到：

```text
build/Debug/Watch_project.hex
build/Debug/Watch_project.bin
```

使用 BIN 时，烧录起始地址为 `0x08000000`。推荐直接使用 HEX。

硬件引脚、按键操作、首次上电检查、CubeMX 重新生成注意事项和当前限制，请查看[项目详细说明](说明.md)。
