
git clone https://github.com/eleanor-broadway/HemoCell.git --branch dev
cd HemoCell 
git clone https://gitlab.com/eleanorb/palabos-hybrid-hemo-cell.git --branch dev
mv palabos-hybrid-hemo-cell palabos 

cd patch 
bash patchPLB.sh 
cd .. 

module load intel-tbb-19
module load nvidia/nvhpc/22.11
module load cmake/3.17.3
module load hdf5parallel/1.12.0-nvhpc-openmpi

export PATH=/mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi4/openmpi-4.0.5/bin:$PATH
export LD_LIBRARY_PATH=/mnt/lustre/indy2lfs/sw/nvidia/hpcsdk-2211/Linux_x86_64/22.11/comm_libs/openmpi4/openmpi-4.0.5/bin:$LD_LIBRARY_PATH

mkdir build && cd build
cmake ..
cmake --build . --target oneCellShear

