# Prerequisites
* Intel QMSI library from https://github.com/quark-mcu/qmsi.git
* Intel System Studio for Quark D2000 (can't be linked directly - make an account)

# Setup
Set environment variables:
```
export QMSI_DIR=/path/to/qmsi
export IAMCU_TOOLCHAIN_DIR=/path/to/intel/issm_2016.0.019/tools/compiler/bin
```

Build qmsi:
```
cd $QMSI_DIR
make
```

## OpenOCD Configuration

Now go to this project's directory.

Copy quark_d2000_onboard.cfg from the Intel tools to openocd.cfg and edit it to suit your programmer.
Any FTDI part with MPSSE, like the FT2232 series, should work.
Remove the reset_config line if you don't connect TRST, which is optional.

Add the following code to openocd.cfg:
```
proc flash_app { app_file } {
	init
	clk32M
	load_image $app_file 0x180000
	verify_image $app_file 0x180000
	reset halt
}

proc flash_and_run { app_file } {
	flash_app $app_file
	resume
}
```

# Building

Build the code and program the target:
```
make prog
```

# Debugging

You can get an OpenOCD for debugging with:
```
make openocd
```

Connect gdb to it with:
```
gdb
(gdb) target remote localhost:3333
```

Shut down the OpenOCD from within gdb with:
```
(gdb) monitor shutdown
```
