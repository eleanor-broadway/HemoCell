Development and Testing of the HemoCell GPU Port:
=================================================

This directory contains:

* DEFAULT oneCellShear.cpp (i.e. the version which is compiled) is the current development test case. This is bridging the gap between the original and the stripped, restoring the test case but using the new accelerated lattice at the compute stage.
* The original-unchanged test case.
* The "stripped" test case. This has been stripped to the bare bones to test the implementation of the ```GuoExternalForceBGKdynamics``` collision model. The output and NSight profile can be found in ```oneCellShear/stripped-output```.
* A secondary development test case. This includes the stripped test case with the full HemoCell set-up restored. Logic for porting applications: do set-up as usual but use the accelerated lattice for compute. When ran, this gives the same error as the "first" development test case. 


Re-compile on Cirrus
--------------------

Replace ```oneCellShear.cpp``` with the iteration of the test case you would like to use:  

```bash
module load intel-tbb-19
module load nvidia/nvhpc/22.11
module load cmake/3.17.3
module load hdf5parallel/1.12.0-nvhpc-openmpi

./compile.sh
```


Original README: OneCellShear
------------------------------

The ``OneCellShear`` case is used for validation of the material models used in
HemoCell. A single cell is initialized in a domain with periodicity turned on
in all directions. Then the domain is sheared such that the top moves in the
positive x-direction and the bottom in the negative x-direction. The largest
diameter is reported and validated against experimental data.

The most important adjustable variables are in the ``config.xml`` file.

- ``<domain><shearrate>`` the shear rate in reciprocal seconds, used to
  calculate the top and bottom velocity boundary conditions.

Furthermore, it is possible to do tanktreading by adjusting ``''RBC''``
throughout the oneCellShear.cpp file to ``''RBC_tt''``. This loads the
different .pos file present within the directory (RBC.xml and RBC_tt.xml)
are the same.
