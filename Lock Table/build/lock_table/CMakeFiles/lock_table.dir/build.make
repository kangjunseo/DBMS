# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /snap/cmake/956/bin/cmake

# The command to remove a file.
RM = /snap/cmake/956/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/psf/Home/Desktop/project4

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/psf/Home/Desktop/project4/build

# Include any dependencies generated for this target.
include lock_table/CMakeFiles/lock_table.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include lock_table/CMakeFiles/lock_table.dir/compiler_depend.make

# Include the progress variables for this target.
include lock_table/CMakeFiles/lock_table.dir/progress.make

# Include the compile flags for this target's objects.
include lock_table/CMakeFiles/lock_table.dir/flags.make

lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o: lock_table/CMakeFiles/lock_table.dir/flags.make
lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o: ../lock_table/src/lock_table.cc
lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o: lock_table/CMakeFiles/lock_table.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/psf/Home/Desktop/project4/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o"
	cd /media/psf/Home/Desktop/project4/build/lock_table && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o -MF CMakeFiles/lock_table.dir/src/lock_table.cc.o.d -o CMakeFiles/lock_table.dir/src/lock_table.cc.o -c /media/psf/Home/Desktop/project4/lock_table/src/lock_table.cc

lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lock_table.dir/src/lock_table.cc.i"
	cd /media/psf/Home/Desktop/project4/build/lock_table && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/psf/Home/Desktop/project4/lock_table/src/lock_table.cc > CMakeFiles/lock_table.dir/src/lock_table.cc.i

lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lock_table.dir/src/lock_table.cc.s"
	cd /media/psf/Home/Desktop/project4/build/lock_table && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/psf/Home/Desktop/project4/lock_table/src/lock_table.cc -o CMakeFiles/lock_table.dir/src/lock_table.cc.s

# Object files for target lock_table
lock_table_OBJECTS = \
"CMakeFiles/lock_table.dir/src/lock_table.cc.o"

# External object files for target lock_table
lock_table_EXTERNAL_OBJECTS =

lib/liblock_table.a: lock_table/CMakeFiles/lock_table.dir/src/lock_table.cc.o
lib/liblock_table.a: lock_table/CMakeFiles/lock_table.dir/build.make
lib/liblock_table.a: lock_table/CMakeFiles/lock_table.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/psf/Home/Desktop/project4/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../lib/liblock_table.a"
	cd /media/psf/Home/Desktop/project4/build/lock_table && $(CMAKE_COMMAND) -P CMakeFiles/lock_table.dir/cmake_clean_target.cmake
	cd /media/psf/Home/Desktop/project4/build/lock_table && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lock_table.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
lock_table/CMakeFiles/lock_table.dir/build: lib/liblock_table.a
.PHONY : lock_table/CMakeFiles/lock_table.dir/build

lock_table/CMakeFiles/lock_table.dir/clean:
	cd /media/psf/Home/Desktop/project4/build/lock_table && $(CMAKE_COMMAND) -P CMakeFiles/lock_table.dir/cmake_clean.cmake
.PHONY : lock_table/CMakeFiles/lock_table.dir/clean

lock_table/CMakeFiles/lock_table.dir/depend:
	cd /media/psf/Home/Desktop/project4/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/psf/Home/Desktop/project4 /media/psf/Home/Desktop/project4/lock_table /media/psf/Home/Desktop/project4/build /media/psf/Home/Desktop/project4/build/lock_table /media/psf/Home/Desktop/project4/build/lock_table/CMakeFiles/lock_table.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : lock_table/CMakeFiles/lock_table.dir/depend

