# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/hd0/Distrib/Projects/azad

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/hd0/Distrib/Projects/azad/build

# Include any dependencies generated for this target.
include CMakeFiles/azad.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/azad.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/azad.dir/flags.make

CMakeFiles/azad.dir/src/audio.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/audio.c.o: ../src/audio.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/azad.dir/src/audio.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/audio.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/audio.c

CMakeFiles/azad.dir/src/audio.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/audio.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/audio.c > CMakeFiles/azad.dir/src/audio.c.i

CMakeFiles/azad.dir/src/audio.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/audio.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/audio.c -o CMakeFiles/azad.dir/src/audio.c.s

CMakeFiles/azad.dir/src/bcm2835.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/bcm2835.c.o: ../src/bcm2835.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/azad.dir/src/bcm2835.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/bcm2835.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/bcm2835.c

CMakeFiles/azad.dir/src/bcm2835.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/bcm2835.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/bcm2835.c > CMakeFiles/azad.dir/src/bcm2835.c.i

CMakeFiles/azad.dir/src/bcm2835.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/bcm2835.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/bcm2835.c -o CMakeFiles/azad.dir/src/bcm2835.c.s

CMakeFiles/azad.dir/src/debug.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/debug.c.o: ../src/debug.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/azad.dir/src/debug.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/debug.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/debug.c

CMakeFiles/azad.dir/src/debug.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/debug.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/debug.c > CMakeFiles/azad.dir/src/debug.c.i

CMakeFiles/azad.dir/src/debug.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/debug.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/debug.c -o CMakeFiles/azad.dir/src/debug.c.s

CMakeFiles/azad.dir/src/dummy.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/dummy.c.o: ../src/dummy.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/azad.dir/src/dummy.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/dummy.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/dummy.c

CMakeFiles/azad.dir/src/dummy.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/dummy.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/dummy.c > CMakeFiles/azad.dir/src/dummy.c.i

CMakeFiles/azad.dir/src/dummy.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/dummy.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/dummy.c -o CMakeFiles/azad.dir/src/dummy.c.s

CMakeFiles/azad.dir/src/flv-demuxer.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/flv-demuxer.c.o: ../src/flv-demuxer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/azad.dir/src/flv-demuxer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/flv-demuxer.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/flv-demuxer.c

CMakeFiles/azad.dir/src/flv-demuxer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/flv-demuxer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/flv-demuxer.c > CMakeFiles/azad.dir/src/flv-demuxer.c.i

CMakeFiles/azad.dir/src/flv-demuxer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/flv-demuxer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/flv-demuxer.c -o CMakeFiles/azad.dir/src/flv-demuxer.c.s

CMakeFiles/azad.dir/src/flv-muxer.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/flv-muxer.c.o: ../src/flv-muxer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/azad.dir/src/flv-muxer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/flv-muxer.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/flv-muxer.c

CMakeFiles/azad.dir/src/flv-muxer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/flv-muxer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/flv-muxer.c > CMakeFiles/azad.dir/src/flv-muxer.c.i

CMakeFiles/azad.dir/src/flv-muxer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/flv-muxer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/flv-muxer.c -o CMakeFiles/azad.dir/src/flv-muxer.c.s

CMakeFiles/azad.dir/src/globals.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/globals.c.o: ../src/globals.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/azad.dir/src/globals.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/globals.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/globals.c

CMakeFiles/azad.dir/src/globals.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/globals.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/globals.c > CMakeFiles/azad.dir/src/globals.c.i

CMakeFiles/azad.dir/src/globals.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/globals.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/globals.c -o CMakeFiles/azad.dir/src/globals.c.s

CMakeFiles/azad.dir/src/gpio.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/gpio.c.o: ../src/gpio.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/azad.dir/src/gpio.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/gpio.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/gpio.c

CMakeFiles/azad.dir/src/gpio.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/gpio.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/gpio.c > CMakeFiles/azad.dir/src/gpio.c.i

CMakeFiles/azad.dir/src/gpio.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/gpio.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/gpio.c -o CMakeFiles/azad.dir/src/gpio.c.s

CMakeFiles/azad.dir/src/ir_control.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/ir_control.c.o: ../src/ir_control.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/azad.dir/src/ir_control.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/ir_control.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/ir_control.c

CMakeFiles/azad.dir/src/ir_control.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/ir_control.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/ir_control.c > CMakeFiles/azad.dir/src/ir_control.c.i

CMakeFiles/azad.dir/src/ir_control.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/ir_control.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/ir_control.c -o CMakeFiles/azad.dir/src/ir_control.c.s

CMakeFiles/azad.dir/src/main.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/main.c.o: ../src/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/azad.dir/src/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/main.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/main.c

CMakeFiles/azad.dir/src/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/main.c > CMakeFiles/azad.dir/src/main.c.i

CMakeFiles/azad.dir/src/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/main.c -o CMakeFiles/azad.dir/src/main.c.s

CMakeFiles/azad.dir/src/nal_to_rtp.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/nal_to_rtp.c.o: ../src/nal_to_rtp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/azad.dir/src/nal_to_rtp.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/nal_to_rtp.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/nal_to_rtp.c

CMakeFiles/azad.dir/src/nal_to_rtp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/nal_to_rtp.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/nal_to_rtp.c > CMakeFiles/azad.dir/src/nal_to_rtp.c.i

CMakeFiles/azad.dir/src/nal_to_rtp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/nal_to_rtp.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/nal_to_rtp.c -o CMakeFiles/azad.dir/src/nal_to_rtp.c.s

CMakeFiles/azad.dir/src/network.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/network.c.o: ../src/network.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/azad.dir/src/network.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/network.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/network.c

CMakeFiles/azad.dir/src/network.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/network.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/network.c > CMakeFiles/azad.dir/src/network.c.i

CMakeFiles/azad.dir/src/network.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/network.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/network.c -o CMakeFiles/azad.dir/src/network.c.s

CMakeFiles/azad.dir/src/nfc.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/nfc.c.o: ../src/nfc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object CMakeFiles/azad.dir/src/nfc.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/nfc.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/nfc.c

CMakeFiles/azad.dir/src/nfc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/nfc.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/nfc.c > CMakeFiles/azad.dir/src/nfc.c.i

CMakeFiles/azad.dir/src/nfc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/nfc.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/nfc.c -o CMakeFiles/azad.dir/src/nfc.c.s

CMakeFiles/azad.dir/src/omx_client.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/omx_client.c.o: ../src/omx_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object CMakeFiles/azad.dir/src/omx_client.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/omx_client.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/omx_client.c

CMakeFiles/azad.dir/src/omx_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/omx_client.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/omx_client.c > CMakeFiles/azad.dir/src/omx_client.c.i

CMakeFiles/azad.dir/src/omx_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/omx_client.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/omx_client.c -o CMakeFiles/azad.dir/src/omx_client.c.s

CMakeFiles/azad.dir/src/onvif.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/onvif.c.o: ../src/onvif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object CMakeFiles/azad.dir/src/onvif.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/onvif.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/onvif.c

CMakeFiles/azad.dir/src/onvif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/onvif.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/onvif.c > CMakeFiles/azad.dir/src/onvif.c.i

CMakeFiles/azad.dir/src/onvif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/onvif.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/onvif.c -o CMakeFiles/azad.dir/src/onvif.c.s

CMakeFiles/azad.dir/src/pthread2threadx.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/pthread2threadx.c.o: ../src/pthread2threadx.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building C object CMakeFiles/azad.dir/src/pthread2threadx.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/pthread2threadx.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/pthread2threadx.c

CMakeFiles/azad.dir/src/pthread2threadx.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/pthread2threadx.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/pthread2threadx.c > CMakeFiles/azad.dir/src/pthread2threadx.c.i

CMakeFiles/azad.dir/src/pthread2threadx.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/pthread2threadx.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/pthread2threadx.c -o CMakeFiles/azad.dir/src/pthread2threadx.c.s

CMakeFiles/azad.dir/src/rc522.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/rc522.c.o: ../src/rc522.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Building C object CMakeFiles/azad.dir/src/rc522.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/rc522.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/rc522.c

CMakeFiles/azad.dir/src/rc522.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/rc522.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/rc522.c > CMakeFiles/azad.dir/src/rc522.c.i

CMakeFiles/azad.dir/src/rc522.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/rc522.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/rc522.c -o CMakeFiles/azad.dir/src/rc522.c.s

CMakeFiles/azad.dir/src/rtsp.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/rtsp.c.o: ../src/rtsp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_18) "Building C object CMakeFiles/azad.dir/src/rtsp.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/rtsp.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/rtsp.c

CMakeFiles/azad.dir/src/rtsp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/rtsp.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/rtsp.c > CMakeFiles/azad.dir/src/rtsp.c.i

CMakeFiles/azad.dir/src/rtsp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/rtsp.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/rtsp.c -o CMakeFiles/azad.dir/src/rtsp.c.s

CMakeFiles/azad.dir/src/streamer.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/streamer.c.o: ../src/streamer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_19) "Building C object CMakeFiles/azad.dir/src/streamer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/streamer.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/streamer.c

CMakeFiles/azad.dir/src/streamer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/streamer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/streamer.c > CMakeFiles/azad.dir/src/streamer.c.i

CMakeFiles/azad.dir/src/streamer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/streamer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/streamer.c -o CMakeFiles/azad.dir/src/streamer.c.s

CMakeFiles/azad.dir/src/system.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/system.c.o: ../src/system.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_20) "Building C object CMakeFiles/azad.dir/src/system.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/system.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/system.c

CMakeFiles/azad.dir/src/system.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/system.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/system.c > CMakeFiles/azad.dir/src/system.c.i

CMakeFiles/azad.dir/src/system.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/system.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/system.c -o CMakeFiles/azad.dir/src/system.c.s

CMakeFiles/azad.dir/src/text_func.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/text_func.c.o: ../src/text_func.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_21) "Building C object CMakeFiles/azad.dir/src/text_func.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/text_func.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/text_func.c

CMakeFiles/azad.dir/src/text_func.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/text_func.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/text_func.c > CMakeFiles/azad.dir/src/text_func.c.i

CMakeFiles/azad.dir/src/text_func.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/text_func.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/text_func.c -o CMakeFiles/azad.dir/src/text_func.c.s

CMakeFiles/azad.dir/src/tfp625a.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/tfp625a.c.o: ../src/tfp625a.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_22) "Building C object CMakeFiles/azad.dir/src/tfp625a.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/tfp625a.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/tfp625a.c

CMakeFiles/azad.dir/src/tfp625a.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/tfp625a.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/tfp625a.c > CMakeFiles/azad.dir/src/tfp625a.c.i

CMakeFiles/azad.dir/src/tfp625a.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/tfp625a.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/tfp625a.c -o CMakeFiles/azad.dir/src/tfp625a.c.s

CMakeFiles/azad.dir/src/weather.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/weather.c.o: ../src/weather.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_23) "Building C object CMakeFiles/azad.dir/src/weather.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/weather.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/weather.c

CMakeFiles/azad.dir/src/weather.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/weather.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/weather.c > CMakeFiles/azad.dir/src/weather.c.i

CMakeFiles/azad.dir/src/weather.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/weather.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/weather.c -o CMakeFiles/azad.dir/src/weather.c.s

CMakeFiles/azad.dir/src/weather_func.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/weather_func.c.o: ../src/weather_func.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_24) "Building C object CMakeFiles/azad.dir/src/weather_func.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/weather_func.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/weather_func.c

CMakeFiles/azad.dir/src/weather_func.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/weather_func.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/weather_func.c > CMakeFiles/azad.dir/src/weather_func.c.i

CMakeFiles/azad.dir/src/weather_func.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/weather_func.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/weather_func.c -o CMakeFiles/azad.dir/src/weather_func.c.s

CMakeFiles/azad.dir/src/web.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/web.c.o: ../src/web.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_25) "Building C object CMakeFiles/azad.dir/src/web.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/web.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/web.c

CMakeFiles/azad.dir/src/web.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/web.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/web.c > CMakeFiles/azad.dir/src/web.c.i

CMakeFiles/azad.dir/src/web.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/web.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/web.c -o CMakeFiles/azad.dir/src/web.c.s

CMakeFiles/azad.dir/src/widgets.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/widgets.c.o: ../src/widgets.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_26) "Building C object CMakeFiles/azad.dir/src/widgets.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/widgets.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/widgets.c

CMakeFiles/azad.dir/src/widgets.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/widgets.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/widgets.c > CMakeFiles/azad.dir/src/widgets.c.i

CMakeFiles/azad.dir/src/widgets.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/widgets.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/widgets.c -o CMakeFiles/azad.dir/src/widgets.c.s

CMakeFiles/azad.dir/src/writer.c.o: CMakeFiles/azad.dir/flags.make
CMakeFiles/azad.dir/src/writer.c.o: ../src/writer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_27) "Building C object CMakeFiles/azad.dir/src/writer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/azad.dir/src/writer.c.o   -c /mnt/hd0/Distrib/Projects/azad/src/writer.c

CMakeFiles/azad.dir/src/writer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/azad.dir/src/writer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/hd0/Distrib/Projects/azad/src/writer.c > CMakeFiles/azad.dir/src/writer.c.i

CMakeFiles/azad.dir/src/writer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/azad.dir/src/writer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/hd0/Distrib/Projects/azad/src/writer.c -o CMakeFiles/azad.dir/src/writer.c.s

# Object files for target azad
azad_OBJECTS = \
"CMakeFiles/azad.dir/src/audio.c.o" \
"CMakeFiles/azad.dir/src/bcm2835.c.o" \
"CMakeFiles/azad.dir/src/debug.c.o" \
"CMakeFiles/azad.dir/src/dummy.c.o" \
"CMakeFiles/azad.dir/src/flv-demuxer.c.o" \
"CMakeFiles/azad.dir/src/flv-muxer.c.o" \
"CMakeFiles/azad.dir/src/globals.c.o" \
"CMakeFiles/azad.dir/src/gpio.c.o" \
"CMakeFiles/azad.dir/src/ir_control.c.o" \
"CMakeFiles/azad.dir/src/main.c.o" \
"CMakeFiles/azad.dir/src/nal_to_rtp.c.o" \
"CMakeFiles/azad.dir/src/network.c.o" \
"CMakeFiles/azad.dir/src/nfc.c.o" \
"CMakeFiles/azad.dir/src/omx_client.c.o" \
"CMakeFiles/azad.dir/src/onvif.c.o" \
"CMakeFiles/azad.dir/src/pthread2threadx.c.o" \
"CMakeFiles/azad.dir/src/rc522.c.o" \
"CMakeFiles/azad.dir/src/rtsp.c.o" \
"CMakeFiles/azad.dir/src/streamer.c.o" \
"CMakeFiles/azad.dir/src/system.c.o" \
"CMakeFiles/azad.dir/src/text_func.c.o" \
"CMakeFiles/azad.dir/src/tfp625a.c.o" \
"CMakeFiles/azad.dir/src/weather.c.o" \
"CMakeFiles/azad.dir/src/weather_func.c.o" \
"CMakeFiles/azad.dir/src/web.c.o" \
"CMakeFiles/azad.dir/src/widgets.c.o" \
"CMakeFiles/azad.dir/src/writer.c.o"

# External object files for target azad
azad_EXTERNAL_OBJECTS =

azad: CMakeFiles/azad.dir/src/audio.c.o
azad: CMakeFiles/azad.dir/src/bcm2835.c.o
azad: CMakeFiles/azad.dir/src/debug.c.o
azad: CMakeFiles/azad.dir/src/dummy.c.o
azad: CMakeFiles/azad.dir/src/flv-demuxer.c.o
azad: CMakeFiles/azad.dir/src/flv-muxer.c.o
azad: CMakeFiles/azad.dir/src/globals.c.o
azad: CMakeFiles/azad.dir/src/gpio.c.o
azad: CMakeFiles/azad.dir/src/ir_control.c.o
azad: CMakeFiles/azad.dir/src/main.c.o
azad: CMakeFiles/azad.dir/src/nal_to_rtp.c.o
azad: CMakeFiles/azad.dir/src/network.c.o
azad: CMakeFiles/azad.dir/src/nfc.c.o
azad: CMakeFiles/azad.dir/src/omx_client.c.o
azad: CMakeFiles/azad.dir/src/onvif.c.o
azad: CMakeFiles/azad.dir/src/pthread2threadx.c.o
azad: CMakeFiles/azad.dir/src/rc522.c.o
azad: CMakeFiles/azad.dir/src/rtsp.c.o
azad: CMakeFiles/azad.dir/src/streamer.c.o
azad: CMakeFiles/azad.dir/src/system.c.o
azad: CMakeFiles/azad.dir/src/text_func.c.o
azad: CMakeFiles/azad.dir/src/tfp625a.c.o
azad: CMakeFiles/azad.dir/src/weather.c.o
azad: CMakeFiles/azad.dir/src/weather_func.c.o
azad: CMakeFiles/azad.dir/src/web.c.o
azad: CMakeFiles/azad.dir/src/widgets.c.o
azad: CMakeFiles/azad.dir/src/writer.c.o
azad: CMakeFiles/azad.dir/build.make
azad: CMakeFiles/azad.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/hd0/Distrib/Projects/azad/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_28) "Linking C executable azad"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/azad.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/azad.dir/build: azad

.PHONY : CMakeFiles/azad.dir/build

CMakeFiles/azad.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/azad.dir/cmake_clean.cmake
.PHONY : CMakeFiles/azad.dir/clean

CMakeFiles/azad.dir/depend:
	cd /mnt/hd0/Distrib/Projects/azad/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/hd0/Distrib/Projects/azad /mnt/hd0/Distrib/Projects/azad /mnt/hd0/Distrib/Projects/azad/build /mnt/hd0/Distrib/Projects/azad/build /mnt/hd0/Distrib/Projects/azad/build/CMakeFiles/azad.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/azad.dir/depend

