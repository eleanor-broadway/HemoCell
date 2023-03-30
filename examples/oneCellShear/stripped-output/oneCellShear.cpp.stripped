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

	plbInit(&argc, &argv);
	// defaultMultiBlockPolicy3D().toggleBlockingCommunication(true);

	if(argc < 2)
	{
			cout << "Usage: " << argv[0] << " <configuration.xml>" << endl;
			return -1;
	}


	// ----------------- Read in config file & calc. LBM parameters ---------------------------
	pcout << "(OneCellShear) (Parameters) calculating shear flow parameters" << endl;
	//	plint nz = 10.0*(1e-6/(*cfg)["domain"]["dx"].read<T>());

  plint nz = 10.0;
  plint nx = 2*nz;
  plint ny = 2*nz;
 	// param::lbm_shear_parameters((*cfg),ny);
 	// param::printParameters();

	// ------------------------ Init lattice --------------------------------

	pcout << "(CellStretch) Initializing lattice: " << nx <<"x" << ny <<"x" << nz << " [lu]" << std::endl;

	plint extendedEnvelopeWidth = 2;  // Because we might use ibmKernel with with 2.

	MultiBlockLattice3D<T,DESCRIPTOR> lattice(nx,ny,nz, new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0/param::tau) );

	// ----------------------------------------------------
	// Here is where the initialisation has finished and the compute is about to begin...

	AcceleratedLattice3D<T, DESCRIPTOR>* accLattice = nullptr;
	pcout << "Created accelerated lattice" << endl;

	// ----------------------------------------------------
	// Suspect that this isn't being done properly...

	accLattice = new AcceleratedLattice3D<T, DESCRIPTOR>(lattice);

	for (int ijk = 0; ijk < 1000000; ijk++) {

		accLattice -> collideAndStream();
  	 //pcout << "Iteration: " << ijk << endl;
	}

  pcout << "(OneCellShear) Simulation finished :)" << std::endl;
  return 0;
}

