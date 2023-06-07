/*
This file is part of the HemoCell library

HemoCell is developed and maintained by the Computational Science Lab
in the University of Amsterdam. Any questions or remarks regarding this library
can be sent to: info@hemocell.eu

When using the HemoCell library in scientific work please cite the
corresponding paper: https://doi.org/10.3389/fphys.2017.00563

The HemoCell library is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define USE_NVIDIA_HPC_SDK // Enables cudaMalloc for communication buffers and NVTX ranges.

#include "hemocell.h"
#include "rbcHighOrderModel.h"
#include "helper/hemocellInit.hh"
#include "helper/cellInfo.h"

#include "palabos3D.h"
#include "palabos3D.hh"

using namespace hemo;

int main(int argc, char* argv[])
{

	// Init the Palabos stuff
	// plbInit(&argc, &argv);
	defaultMultiBlockPolicy3D().toggleBlockingCommunication(true);


	if(argc < 2)
	{
			cout << "Usage: " << argv[0] << " <configuration.xml>" << endl;
			return -1;
	}


	HemoCell hemocell(argv[1], argc, argv);
	Config * cfg = hemocell.cfg;


	// ----------------- Read in config file & calc. LBM parameters ---------------------------
	pcout << "(OneCellShear) (Parameters) calculating shear flow parameters" << endl;
	plint nz = 10.0*(1e-6/(*cfg)["domain"]["dx"].read<T>());
  	plint nx = 2*nz;
  	plint ny = 2*nz;
 	param::lbm_shear_parameters((*cfg),ny);
 	param::printParameters();


	// ------------------------ Init lattice --------------------------------
	pcout << "(CellStretch) Initializing lattice: " << nx <<"x" << ny <<"x" << nz << " [lu]" << std::endl;
	plint extendedEnvelopeWidth = 2;  // Because we might use ibmKernel with with 2.

	// LOG of adding the hemocell.lattice back in...
	// Assignment of the lattice, working in the stipped test case
	// MultiBlockLattice3D<T,DESCRIPTOR> lattice(nx,ny,nz, new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0/param::tau) );

	// Attempting to copy the "hemocell.lattice" instatntiation of the lattice.
	// MultiBlockLattice3D<T,DESCRIPTOR>* lattice = nullptr;
	// lattice = new MultiBlockLattice3D<T,DESCRIPTOR>(nx,ny,nz, new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0/param::tau) );

	// Restoring the original assignment of the lattice
	hemocell.lattice = new MultiBlockLattice3D<T,DESCRIPTOR>(
	             defaultMultiBlockPolicy3D().getMultiBlockManagement(nx, ny, nz, extendedEnvelopeWidth),
	             defaultMultiBlockPolicy3D().getBlockCommunicator(),
	             defaultMultiBlockPolicy3D().getCombinedStatistics(),
	             defaultMultiBlockPolicy3D().getMultiCellAccess<T, DESCRIPTOR>(),
	             new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0/param::tau));

	pcout << "(OneCellShear) Re corresponds to u_max = " << (param::re * param::nu_p)/(hemocell.lattice->getBoundingBox().getNy()*param::dx) << " [m/s]" << endl;


	// -------------------------- Define boundary conditions ---------------------
	OnLatticeBoundaryCondition3D<T,DESCRIPTOR>* boundaryCondition = createLocalBoundaryCondition3D<T,DESCRIPTOR>();
	hemocell.lattice->toggleInternalStatistics(false);

	// This is the line which causes the "ERROR: Model "Boundary_RegularizedVelocity_2_-1 >> BGK_ExternalForce_Guo" not implemented." to appear.
	// It then segmentation faults and exits. Found in helper/hemocellInit.hh
	//iniLatticeSquareCouette(*hemocell.lattice, nx, ny, nz, *boundaryCondition, param::shearrate_lbm);

	// Inlining...

	plb::Box3D top = plb::Box3D(0, nx-1, 0, ny-1, nz-1, nz-1);
	plb::Box3D bottom = plb::Box3D(0, nx-1, 0, ny-1, 0, 0);

	hemocell.lattice->periodicity().toggle(0, true);
  hemocell.lattice->periodicity().toggle(1, true);
  hemocell.lattice->periodicity().toggle(2, false);

	// When these lines are added we see the "ERROR: Model" lines but it continues on to the compute
	boundaryCondition->setVelocityConditionOnBlockBoundaries(*hemocell.lattice, top);
  boundaryCondition->setVelocityConditionOnBlockBoundaries(*hemocell.lattice, bottom);

	T vHalf = (nz-1)*param::shearrate_lbm*0.5;
  setBoundaryVelocity(*hemocell.lattice, top, plb::Array<T,3>(-vHalf,0.0,0.0));
  setBoundaryVelocity(*hemocell.lattice, bottom, plb::Array<T,3>(vHalf,0.0,0.0));

	// This line still goes on to the compute (i.e. no memory error)
	setExternalVector(*hemocell.lattice, hemocell.lattice->getBoundingBox(),DESCRIPTOR<T>::ExternalField::forceBeginsAt, plb::Array<T,DESCRIPTOR<T>::d>(0.0,0.0,0.0));


	// NOW create the AccelerateLattice and assign
	AcceleratedLattice3D<T, DESCRIPTOR>* accLattice = nullptr;
	accLattice = new AcceleratedLattice3D<T, DESCRIPTOR>(*hemocell.lattice);

	accLattice->initialize();
	hemocell.lattice->initialize();
	// END of iniLatticeSquareCouette

	// This is done twice...
	// hemocell.lattice->initialize();
	hemocell.outputInSiUnits = true;


	// ----------------------- Init cell models --------------------------
	hemocell.initializeCellfield();
	hemocell.addCellType<RbcHighOrderModel>("RBC", RBC_FROM_SPHERE);
	vector<int> outputs = {OUTPUT_POSITION,OUTPUT_TRIANGLES,OUTPUT_FORCE,OUTPUT_FORCE_VOLUME,OUTPUT_FORCE_BENDING,OUTPUT_FORCE_LINK,OUTPUT_FORCE_AREA, OUTPUT_FORCE_VISC};
	hemocell.setOutputs("RBC", outputs);

	outputs = {OUTPUT_VELOCITY};
	hemocell.setFluidOutputs(outputs);


	// ---------------------- Initialise particle positions if it is not a checkpointed run ---------------

	//loading the cellfield
  	if (not cfg->checkpointed) {
    	hemocell.loadParticles();
    	hemocell.writeOutput();
  	} else {
    	pcout << "(OneCellShear) CHECKPOINT found!" << endl;
    	hemocell.loadCheckPoint();
  	}

  	if (hemocell.iter == 0) {
    	pcout << "(OneCellShear) fresh start: warming up cell-free fluid domain for "  << (*cfg)["parameters"]["warmup"].read<plint>() << " iterations..." << endl;

    for (plint itrt = 0; itrt < (*cfg)["parameters"]["warmup"].read<plint>(); ++itrt) {
      hemocell.lattice->collideAndStream();
    }
  	}

	pcout << "(OneCellShea) Shear rate: " << (*cfg)["domain"]["shearrate"].read<T>() << " s^-1." << endl;

	unsigned int tmax = (*cfg)["sim"]["tmax"].read<unsigned int>();
  unsigned int tmeas = (*cfg)["sim"]["tmeas"].read<unsigned int>();
  unsigned int tcheckpoint = (*cfg)["sim"]["tcheckpoint"].read<unsigned int>();

	// Get undeformed cell values
	CellInformationFunctionals::calculateCellVolume(&hemocell);
	CellInformationFunctionals::calculateCellArea(&hemocell);
	T volume_eq = (CellInformationFunctionals::info_per_cell[0].volume)/pow(1e-6/param::dx,3);
	T surface_eq = (CellInformationFunctionals::info_per_cell[0].area)/pow(1e-6/param::dx,2);

	T D0 = 2.0 * (*cfg)["ibm"]["radius"].read<T>() * 1e6;

	// Creating output log file
	plb_ofstream fOut;
	if(cfg->checkpointed)
 		 fOut.open("stretch.log", std::ofstream::app);
	else
  	fOut.open("stretch.log");


	// ----------------------------------- Compute -----------------------------------

	while (hemocell.iter < tmax) {
		// Using new acciterate, created for the accelerated lattice
		hemocell.acciterate(accLattice);
		// accLattice -> collideAndStream(): 


	// This is how we were previously testing...
	// for (int ijk = 0; ijk < tmax; ijk++) {
	// 		accLattice -> collideAndStream();

		if (hemocell.iter % tmeas == 0) {
				pcout << hemocell.iter << endl;
		}
	}

  fOut.close();

  pcout << "(OneCellShear) Simulation finished :)" << std::endl;
  return 0;
}
