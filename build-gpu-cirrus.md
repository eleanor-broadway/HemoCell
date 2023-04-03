Compiling GPU-HemoCell on Cirrus:
=================================

Obtain the source code:
----------------------
```bash
git clone https://github.com/eleanor-broadway/HemoCell.git --branch dev
cd HemoCell
git clone https://gitlab.com/eleanorb/palabos-hybrid-hemo-cell.git --branch dev
mv palabos-hybrid-hemo-cell palabos
```

Set-up environment:
--------------------
```bash
module load intel-tbb-19
module load nvidia/nvhpc/22.11
module load cmake/3.17.3
module load hdf5parallel/1.12.0-nvhpc-openmpi
```
<!-- export PATH=/mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi4/openmpi-4.0.5/bin:$PATH
export LD_LIBRARY_PATH=/mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi4/openmpi-4.0.5/bin:$LD_LIBRARY_PATH -->

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
cmake ..
cmake --build . --target oneCellShear
```

*****

cmake log for reference (using OpenMPI-3.1.5):

```bash
-- The CXX compiler identification is PGI 22.11.0
-- The C compiler identification is PGI 22.11.0
-- Check for working CXX compiler: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/compilers/bin/nvc++
-- Check for working CXX compiler: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/compilers/bin/nvc++ - works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Check for working C compiler: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/compilers/bin/nvc
-- Check for working C compiler: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/compilers/bin/nvc - works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- HEMOCELL_DIR: .
-- PALABOS_DIR : ./palabos
-- Setting up `googletests` release-1.10.0...
-- Found PythonInterp: /usr/bin/python3.6 (found version "3.6.8")
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- parmetis: PARMETIS_INCLUDE_DIR-NOTFOUND
-- parmetis: PARMETIS_LIBRARY-NOTFOUND
-- metis: METIS_INCLUDE_DIR-NOTFOUND
-- metis: METIS_LIBRARY-NOTFOUND
-- Found MPI_C: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi/openmpi-3.1.5/lib/libmpi.so (found version "3.1")
-- Found MPI_CXX: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi/openmpi-3.1.5/lib/libmpi_cxx.so (found version "3.1")
-- Found MPI: TRUE (found version "3.1")  
-- HDF5: Using hdf5 compiler wrapper to determine C configuration
-- Found HDF5: /mnt/lustre/indy2lfs/sw/hdf5parallel/1.12.0-nvhpc/lib/libhdf5.so;/usr/lib64/libz.so;/usr/lib64/libdl.so;/usr/lib64/libm.so (found version "1.12.0") found components: C HL
-- Compiler: /mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/mpi/bin/mpicxx, PGI, 22.11.0.
nvc++
-- Configuring done
-- Generating done
-- Build files have been written to: /work/d411/d411/eleanor/default-mpi/HemoCell/build
```
