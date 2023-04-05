Compiling GPU-HemoCell on Cirrus:
=================================

Compile HDF5 for NVHPC:
------------------------

Set-up environment:
```bash
ssh eleanor@doornode.surfsara.nl


module load 2021
module load OpenMPI/4.1.1-NVHPC-22.3
module load NVHPC/22.3
```

Obtain the source code:
```bash
wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.tar.gz
tar -xvf hdf5-1.12.0.tar.gz
cd hdf5-1.12.0/
```

```bash
export LDFLAGS=-fPIC
export FCFLAGS=-fPIC
./configure --prefix=/home/eleanor/hdf5parallel --enable-parallel --enable-fortran --enable-build-mode=production
```

Edit the ./fortran/testpar/ptest.f90 file: Delete the line that reads USE MPI and add INCLUDE "mpif.h" immediately after the IMPLICIT NONE statement.

```bash
make -j 8
make -j 8 install
make clean
```

Obtain the source code for HemoCell and Palabos:
------------------------------------------------
```bash
cd

git clone https://github.com/eleanor-broadway/HemoCell.git --branch dev
cd HemoCell
git clone https://gitlab.com/eleanorb/palabos-hybrid-hemo-cell.git --branch dev
mv palabos-hybrid-hemo-cell palabos
```

Set-up environment (some may be the same as above):
--------------------
```bash
module load 2021
module load NVHPC/22.3
module load tbb/2020.3-GCCcore-10.3.0
module load CMake/3.20.1-GCCcore-10.3.0
module load OpenMPI/4.1.1-NVHPC-22.3
```

Patch:
-----
This should work but will report various line offsets (i.e. not as clean)
```bash
cd patch
bash patchPLB.sh
cd ..
```

Build:
```bash
mkdir build && cd build

export CXX=nvc++
export CC=nvc
export PATH=$PATH:/home/eleanor/hdf5parallel
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/eleanor/hdf5parallel/lib

cmake ..
cmake --build . --target oneCellShear
```

*****

cmake log for reference:

```bash
-- The CXX compiler identification is NVHPC 22.3.0
-- The C compiler identification is NVHPC 22.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /sw/arch/Centos8/EB_production/2021/software/NVHPC/22.3/Linux_x86_64/22.3/compilers/bin/nvc++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /sw/arch/Centos8/EB_production/2021/software/NVHPC/22.3/Linux_x86_64/22.3/compilers/bin/nvc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- HEMOCELL_DIR: .
-- PALABOS_DIR : ./palabos
-- Setting up `googletests` release-1.10.0...
CMake Deprecation Warning at build/_deps/googletest-src/CMakeLists.txt:4 (cmake_minimum_required):
  Compatibility with CMake < 2.8.12 will be removed from a future version of
  CMake.

  Update the VERSION argument <min> value or use a ...<max> suffix to tell
  CMake that the project does not need compatibility with older versions.


CMake Deprecation Warning at build/_deps/googletest-src/googlemock/CMakeLists.txt:45 (cmake_minimum_required):
  Compatibility with CMake < 2.8.12 will be removed from a future version of
  CMake.

  Update the VERSION argument <min> value or use a ...<max> suffix to tell
  CMake that the project does not need compatibility with older versions.


CMake Deprecation Warning at build/_deps/googletest-src/googletest/CMakeLists.txt:56 (cmake_minimum_required):
  Compatibility with CMake < 2.8.12 will be removed from a future version of
  CMake.

  Update the VERSION argument <min> value or use a ...<max> suffix to tell
  CMake that the project does not need compatibility with older versions.


-- Found PythonInterp: /usr/bin/python3.9 (found version "3.9.13")
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- parmetis: PARMETIS_INCLUDE_DIR-NOTFOUND
-- parmetis: PARMETIS_LIBRARY-NOTFOUND
-- metis: METIS_INCLUDE_DIR-NOTFOUND
-- metis: METIS_LIBRARY-NOTFOUND
-- Found MPI_C: /sw/arch/Centos8/EB_production/2021/software/OpenMPI/4.1.1-NVHPC-22.3/lib/libmpi.so (found version "3.1")
-- Found MPI_CXX: /sw/arch/Centos8/EB_production/2021/software/OpenMPI/4.1.1-NVHPC-22.3/lib/libmpi.so (found version "3.1")
-- Found MPI: TRUE (found version "3.1")  
-- Found HDF5: /home/eleanor/hdf5parallel/lib/libhdf5.so;/sw/arch/Centos8/EB_production/2021/software/zlib/1.2.11-GCCcore-10.3.0/lib/libz.so;/usr/lib64/libdl.so;/usr/lib64/libm.so (found version "1.12.0") found components: C HL
-- Compiler: /sw/arch/Centos8/EB_production/2021/software/OpenMPI/4.1.1-NVHPC-22.3/bin/mpicxx, NVHPC, 22.3.0.
nvc++
-- Configuring done
-- Generating done
-- Build files have been written to: /home/eleanor/HemoCell/build
```


Submission script:
```bash

#!/bin/bash

#SBATCH --partition=gpu
#SBATCH --nodes=1
#   SBATCH --exclusive
#SBATCH --ntasks=1
#SBATCH --gpus=1
#SBATCH --cpus-per-task=10
#SBATCH --time=01:00:00

cat $0

module load 2021
module load OpenMPI/4.1.1-NVHPC-22.3
module load NVHPC/22.3

export PATH=$PATH:/home/eleanor/hdf5parallel
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/eleanor/hdf5parallel/lib

export OMP_NUM_THREADS=10

srun $HOME/HemoCell/examples/oneCellShear/oneCellShear config.xml

```
