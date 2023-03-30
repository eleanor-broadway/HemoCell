#!/bin/bash

#SBATCH --partition=gpu
#SBATCH --qos=gpu
#SBATCH --gres=gpu:1
#SBATCH --nodes=1
# SBATCH --exclusive
#SBATCH --time=01:00:00

# Replace [budget code] below with your project code (e.g. t01)
#SBATCH --account=d411-hemocellgpu

# Load the required module
module load nvidia/nvhpc
cat $0

# Total number of GPUs
NGPUS=1
# Total number of GPUs per node
NGPUS_PER_NODE=1
# Number of CPUs per task
CPUS_PER_TASK=10

export OMP_NUM_THREADS=10
export OMP_PLACES=cores

export SLURM_NTASKS_PER_NODE=${NGPUS_PER_NODE}
export SLURM_TASKS_PER_NODE="${NGPUS_PER_NODE}(x${SLURM_NNODES})"
#export UCX_MEMTYPE_CACHE=n
export OMPI_MCA_mca_base_component_show_load_errors=0

time mpirun -n ${NGPUS} -N ${NGPUS_PER_NODE} nsys profile -o oCS-stripped-profile ./oneCellShear config.xml

