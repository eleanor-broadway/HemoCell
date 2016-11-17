/* This file is part of the Palabos library.
 * Copyright (C) 2009, 2010 Jonas Latt
 * E-mail contact: jonas@lbmethod.org
 * The most recent release of Palabos can be downloaded at
 * <http://www.lbmethod.org/palabos/>
 *
 * The library Palabos is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ficsion.h"
#include "trombosit/trombocit.h"

typedef double T;
typedef Array<T,3> Velocity;
#define DESCRIPTOR descriptors::ForcedD3Q19Descriptor
using namespace trombocit;


void readFicsionXML(XMLreader & documentXML,std::string & caseId, plint & rbcModel, T & shellDensity, T & k_rest,
        T & k_shear, T & k_bend, T & k_stretch, T & k_WLC, T & eqLengthRatio, T & k_rep, T & k_elastic, T & k_volume, T & k_surface, T & eta_m,
        T & rho_p, T & u, plint & flowType, T & Re, T & shearRate, T & stretchForce, Array<T,3> & eulerAngles, T & Re_p, T & N, T & lx, T & ly, T & lz,
        plint & forceToFluid, plint & ibmKernel, plint & ibmScheme, plint & shape, std::string & cellPath, T & radius, T & deflationRatio, pluint & relaxationTime,
        plint & minNumOfTriangles, pluint & tmax, plint & tmeas, T & hct, plint & npar, plint & flowParam, bool & checkpointed, T & Delta,
        plint & ChooseBondType, plint & ChooseBondPotential, T & K_scale, T & R_attach, T & R_break)
    {
    T nu_p, tau, dx;
    T dt, nu_lb;
    std::string firstField = (*(documentXML.getChildren( documentXML.getFirstId() )[0])).getName(); // VERY COMPLICATED! Hope I could find sth easier!
    if (firstField=="ficsion") { checkpointed = 0; }
    else { checkpointed = 1; }

    XMLreaderProxy document = checkpointed?documentXML["Checkpoint"]["ficsion"]:documentXML["ficsion"];
    document["caseId"].read(caseId);
    document["cellModel"]["rbcModel"].read(rbcModel);
    document["cellModel"]["shellDensity"].read(shellDensity);
    document["cellModel"]["kWLC"].read(k_WLC);
    document["cellModel"]["eqLengthRatio"].read(eqLengthRatio);
    document["cellModel"]["kRep"].read(k_rep);
    document["cellModel"]["kElastic"].read(k_elastic);
    document["cellModel"]["kBend"].read(k_bend);
    document["cellModel"]["kVolume"].read(k_volume);
    document["cellModel"]["kSurface"].read(k_surface);
    document["cellModel"]["etaM"].read(eta_m);
    document["cellModel"]["kRest"].read(k_rest);
    document["cellModel"]["kShear"].read(k_shear);
    document["cellModel"]["kStretch"].read(k_stretch);
    document["parameters"]["flowType"].read(flowType);
    document["parameters"]["Re"].read(Re);
    document["parameters"]["shearRate"].read(shearRate);
    document["parameters"]["stretchForce"].read(stretchForce); // In picoNewton
    stretchForce *= 1e-12;
    std::vector<T> ea;
    document["parameters"]["eulerAngles"].read(ea);
    if (ea.size() != 3) {
        ea.resize(3, 0.0);
    }
    eulerAngles = Array<T,3>(ea[0], ea[1], ea[2]);
    eulerAngles[0] *= pi/180.;
    eulerAngles[1] *= pi/180.;
    eulerAngles[2] *= pi/180.;
    document["parameters"]["deflationRatio"].read(deflationRatio);
    document["parameters"]["relaxationTime"].read(relaxationTime);
    document["parameters"]["Delta"].read(Delta);
    document["parameters"]["ChooseBondType"].read(ChooseBondType);
    document["parameters"]["ChooseBondPotential"].read(ChooseBondPotential);
    document["parameters"]["K_scale"].read(K_scale);
    document["parameters"]["R_attach"].read(R_attach);
    document["parameters"]["R_break"].read(R_break);
    document["ibm"]["forceToFluid"].read(forceToFluid);
    document["ibm"]["ibmKernel"].read(ibmKernel);
    document["ibm"]["shape"].read(shape);
    if (2 == shape) {
        document["ibm"]["cellPath"].read(cellPath);
    }
    document["ibm"]["radius"].read(radius);
    document["ibm"]["minNumOfTriangles"].read(minNumOfTriangles);
    try {
        document["ibm"]["ibmScheme"].read(ibmScheme);
    } catch(const plb::PlbIOException & message) {
        ibmScheme=0;
    }
    document["domain"]["rhoP"].read(rho_p);
    document["domain"]["nuP"].read(nu_p);
    document["domain"]["tau"].read(tau);
    document["domain"]["dx"].read(dx);
    // Read lx, ly, lz --or nx, ny, nz
    try {
        document["domain"]["lx"].read(lx);
        document["domain"]["ly"].read(ly);
        document["domain"]["lz"].read(lz);
    } catch(const plb::PlbIOException & message) {
        T nx, ny, nz;
        document["domain"]["nx"].read(nx);
        document["domain"]["ny"].read(ny);
        document["domain"]["nz"].read(nz);
        lx = nx * dx;
        ly = ny * dx;
        lz = nz * dx;
    }
    document["sim"]["tmax"].read(tmax);
    document["sim"]["tmeas"].read(tmeas);
    try {
        document["sim"]["npar"].read(npar);
        hct = 0;
    } catch(const plb::PlbIOException & message) {
        document["sim"]["hct"].read(hct);
        npar = 0;
    }
    hct /= 100.0;

    radius = radius*1.0/dx; // Transform from [m] to [LU]
    nu_lb = (tau-0.5)/3.0;
    dt = (nu_lb/nu_p)*dx*dx;
    u = dt*1.0/dx;
    Re_p = 1.0/nu_p;
    N = int(1.0/dx);
    flowParam = 0; // flowType/10;
//    flowType = flowType%10;
    if ( (flowType == 3) or (flowType == 4) or (flowType == 5) or (flowType == 8) ) { // Cell Stretching Analysis
        shearRate = 0;
        Re = 0;
    } else if (flowType == 6) { // Tumbling Tank Treading Measurements
        if (shearRate > 0) {
            tmax  = 100.0/(shearRate*dt); // Measurements for 100 Strain rates
            tmeas = 0.02/(shearRate*dt); // 50 Measurements per Strain rates
        }
    }
    if (flowParam == 7) {
        tmax += 20.0/dt; // Add 20 seconds to the total time for Fischer2004.
    }
}



int main(int argc, char* argv[])
{
    plbInit(&argc, &argv);
    global::timer("ficsion_init").start();

    global::directories().setOutputDir("./tmp/");
    global::directories().setLogOutDir("./log/");
    global::directories().setInputDir("./");

    global::IOpolicy().activateParallelIO(true);
    global::IOpolicy().setStlFilesHaveLowerBound(false);
/*
    Use trombocit module
*/
    // trombocit::useTrombosit = true;

//    global::IOpolicy().setLowerBoundForStlFiles(-1.);
//    testInPlane(); PLB_ASSERT(false);
    std::string outputDir = global::directories().getOutputDir();
    std::string inputDir = global::directories().getInputDir();
    std::string logOutDir = global::directories().getLogOutDir();
    mkpath((outputDir + "/hdf5/").c_str(), 0777);
    mkpath(logOutDir.c_str(), 0777);

    plint forceToFluid, shape, cellNumTriangles, ibmKernel, ibmScheme;
    plint rbcModel;
    std::string caseId;
    std::string cellPath;
    pluint tmax;
    plint tmeas;
    T hct = 0;
    plint npar = 0;
//    T dtIteration = 0;
    T shellDensity, k_rest, k_shear, k_bend, k_stretch, k_WLC, k_rep, k_elastic,  k_volume, k_surface, eta_m;
    T eqLengthRatio;
    T u, Re, Re_p, N, lx, ly, lz;
    T Delta;
    plint ChooseBondType;
    plint ChooseBondPotential;
    T K_scale;
    T R_attach;
    T R_break;
    T poiseuilleForce=0;
    T rho_p;
    T radius;
    T deflationRatio;
    pluint relaxationTime;
    plint flowType;
    T shearRate, shearRate_p;
    Array<T,3> stretchForce(0,0,0);
    T stretchForceScalar;
    T stretchForce_p;
    Array<T,3> eulerAngles;
    plint flowParam;
    bool checkpointed=0;

// change output folder name if desired--
    string outputFolderName;
    global::argv(2).read(outputFolderName);
    if (sizeof(outputFolderName)>0) { 
        global::directories().setOutputDir(outputFolderName);
    }
//---------------------------------------

    string paramXmlFileName;
    global::argv(1).read(paramXmlFileName);
    XMLreader document(paramXmlFileName);
    pcout << "(main) reading.." <<std::endl;
    readFicsionXML(document, caseId, rbcModel, shellDensity,
            k_rest, k_shear, k_bend, k_stretch, k_WLC, eqLengthRatio, k_rep, k_elastic, k_volume, k_surface, eta_m,
            rho_p, u, flowType, Re, shearRate_p, stretchForce_p, eulerAngles, Re_p, N, lx, ly, lz,  forceToFluid, ibmKernel, ibmScheme, shape, cellPath, radius, deflationRatio, relaxationTime,
            cellNumTriangles, tmax, tmeas, hct, npar, flowParam, checkpointed,
            Delta,
            ChooseBondType, ChooseBondPotential, K_scale, R_attach, R_break);
    IncomprFlowParam<T> parameters(
            u, // u
            Re_p, // Inverse viscosity (1/nu_p)
            N,   // N
            lx,        // lx
            ly,        // ly
            lz         // lz
    );

    plint nx = parameters.getNx();
    plint ny = parameters.getNy();
    plint nz = parameters.getNz();
    T tau = parameters.getTau();
    T dx = parameters.getDeltaX();
    T dt = parameters.getDeltaT();
    T dm = rho_p * (dx*dx*dx);
    dNewton = (dm*dx/(dt*dt)) ;
     kBT = kBT_p / ( dNewton * dx );
    shearRate = shearRate_p * dt;
    stretchForceScalar = stretchForce_p / dNewton;
    pcout << "(main) dx = " << dx << ", " <<
             "dt = " << dt << ", " <<
             "dm = " << dt << ", " <<
             "kT = " << kBT <<
             std::endl;

    /* ------------------ *
     * Initialize Lattice *
     * ------------------ */
    plint extendedEnvelopeWidth=4;
//    if (ibmKernel==2) {
//        extendedEnvelopeWidth = 1;  // Because we might use ibmKernel with width 2.
//    }
//    else {
//        extendedEnvelopeWidth = 2;  // Because we might use ibmKernel with width 2.
//    }
    MultiBlockLattice3D<T, DESCRIPTOR> lattice(
        defaultMultiBlockPolicy3D().getMultiBlockManagement(nx, ny, nz, extendedEnvelopeWidth),
        defaultMultiBlockPolicy3D().getBlockCommunicator(),
        defaultMultiBlockPolicy3D().getCombinedStatistics(),
        defaultMultiBlockPolicy3D().getMultiCellAccess<T,DESCRIPTOR>(),
        new GuoExternalForceBGKdynamics<T,DESCRIPTOR>(parameters.getOmega()));
    lattice.periodicity().toggleAll(true);
    /*
     * Choose case (Square Poiseuille, Couette etc) *
     */
    OnLatticeBoundaryCondition3D<T,DESCRIPTOR>* boundaryCondition
        = createLocalBoundaryCondition3D<T,DESCRIPTOR>();
    pcout << std::endl << "Initializing lattice: " << nx << "x" << ny << "x" << nz << ": tau=" << tau << std::endl;
    lattice.toggleInternalStatistics(false);
    Array<plint,3> forceIds;
    forceIds[0] = lattice.internalStatSubscription().subscribeSum();
    forceIds[1] = lattice.internalStatSubscription().subscribeSum();
    forceIds[2] = lattice.internalStatSubscription().subscribeSum();
    plint nMomentumExchangeCells=0;
    if (flowType == 0 or flowType == 3) {
        T L_tmp = parameters.getNy();
        T nu_tmp = parameters.getLatticeNu();
        poiseuilleForce = 8 * (nu_tmp*nu_tmp) * Re / (L_tmp*L_tmp*L_tmp) ;
        pcout << "(main) Using iniLatticePoiseuilleWithBodyForce. "<< flowType << std::endl;
        iniLatticePoiseuilleWithBodyForce<T, DESCRIPTOR>(lattice, parameters, *boundaryCondition, poiseuilleForce);
    }
    else if (flowType == 1) {
        poiseuilleForce = 0;
        pcout << "(main) Using iniLatticeSquareCouette. "<< flowType << std::endl;
        iniLatticeSquareCouette<T, DESCRIPTOR>(lattice, parameters, *boundaryCondition, shearRate);
    }
    else if (flowType == 11) {
        poiseuilleForce = 0;
        pcout << "(main) Using iniLatticeSquareCouetteMeasureStress. "<< flowType << " yd=" << shearRate/dt << std::endl;
        lattice.toggleInternalStatistics(true);
        iniLatticeSquareCouetteMeasureStress<T, DESCRIPTOR>(lattice, parameters, *boundaryCondition, shearRate, forceIds, nMomentumExchangeCells);
        lattice.toggleInternalStatistics(false);
    }
    else if (flowType == 2) {
        poiseuilleForce = 0;
        pcout << "(main) Using iniLatticeFullyPeriodic. "<< flowType << std::endl;
//        envelope-update
        iniLatticeFullyPeriodic<T, DESCRIPTOR>(lattice, parameters, Array<T,3>(0.02, 0.00, 0.00));
    }


    /*
     * Initialize model *
     */

    /* CREATE MESH */
//    Array<T,3> eqShapeRotationEulerAngles = eulerAngles;
    if (flowParam == 9) { eulerAngles= Array<T,3>(0.,0.,0.); }
    // Radius in LU


    T persistenceLengthFine = 7.5e-9 ; // In meters
	k_rest= 0;

    std::vector<ConstitutiveModel<T, DESCRIPTOR>* > cellModels;
    std::vector<CellField3D<T, DESCRIPTOR>* > cellFields;

    std::vector<T> eqVolumes;
//    =======================  Create RBC
    TriangleBoundary3D<T> Cells = constructMeshElement(shape, radius, cellNumTriangles, dx, cellPath, eulerAngles);
    TriangularSurfaceMesh<T> meshElement = Cells.getMesh();
    MeshMetrics<T> meshmetric(meshElement);    meshmetric.write();
    // eqVolumes.push_back(meshmetric.getVolume());
    plint numVerticesPerCell = meshElement.getNumVertices();
    /* The Maximum length of two vertices should be less than 2.0 LU (or not)*/
	// cellModels.push_back(new ShapeMemoryModel3D<T, DESCRIPTOR>(shellDensity, k_rest, k_shear, k_bend, k_stretch, k_WLC, k_elastic, k_volume, k_surface, eta_m,
	// 	persistenceLengthFine, eqLengthRatio, dx, dt, dm,meshElement));
 //    cellFields.push_back(new CellField3D<T, DESCRIPTOR>(lattice, meshElement, hct*0, cellModels[0], ibmKernel, "RBC"));

//    =======================  Create Platelet
        T pltRadius = 1.15e-6/dx;
        T aspectRatio = 1.0 / (2*pltRadius);
        TriangleBoundary3D<T> PLTCells = constructMeshElement(6, pltRadius, cellNumTriangles, dx, cellPath, eulerAngles, aspectRatio);
        TriangularSurfaceMesh<T> pltMeshElement = PLTCells.getMesh();
        eqVolumes.push_back(MeshMetrics<T>(pltMeshElement).getVolume());
        cellModels.push_back(new ShapeMemoryModel3D<T, DESCRIPTOR>(shellDensity, k_rest, k_shear, k_bend, k_stretch, k_WLC,
            k_elastic, k_volume, k_surface, eta_m, persistenceLengthFine, eqLengthRatio, dx, dt, dm, pltMeshElement) );
        cellFields.push_back(new CellField3D<T, DESCRIPTOR>(lattice, pltMeshElement, 0.005, cellModels[cellModels.size()-1], ibmKernel, "PLT"));



    // CellField3D<T, DESCRIPTOR> & RBCField = *cellFields[0];
    CellField3D<T, DESCRIPTOR> & PLTField = *cellFields[0];

    FcnCheckpoint<T, DESCRIPTOR> checkpointer(document);
    plint initIter=0;
    checkpointer.load(document, lattice, cellFields, initIter);
    if (not checkpointer.wasCheckpointed()) {
        pcout << "(main) initializing"<< std::endl;
        std::vector<Array<T,3> > cellsOrigin;
//        cellsOrigin.push_back( Array<T,3>(nx*0.5-pltRadius*aspectRatio-0.6, ny*0.5, nz*0.5) );
        cellsOrigin.push_back( Array<T,3>(nx*0.5-pltRadius*aspectRatio-Delta-0.5, ny*0.5, nz*0.5) );
        cellsOrigin.push_back( Array<T,3>(nx*0.5, ny*0.5, nz*0.5) );
        PLTField.initialize(cellsOrigin);
        checkpointer.save(lattice, cellFields, initIter);
    }
    for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
    	cellFields[iCell]->setParticleUpdateScheme(ibmScheme);
    }

//    if (rbcModel == 3) {
//        // Has a problem with checkpointing
//    	(dynamic_cast<RestModel3D<T,DESCRIPTOR>*>(cellModel))->freezeVertices(RBCField);
//    }
	pcout << std::endl ;
    for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
		plint nCells = cellFields[iCell]->getNumberOfCells_Global();
		pcout << "(main) Hematocrit [x100%]: " << nCells*eqVolumes[iCell]*100.0/(nx*ny*nz) << std::endl;
		pcout << "(main) nCells (global) = " << nCells << ", pid: " << global::mpi().getRank() ;
		pcout << ", Volume = " << eqVolumes[iCell] << std::endl;
    }
//    MultiParticleField3D<DenseParticleField3D<T,DESCRIPTOR> > * boundaryParticleField3D =
//                                                        createBoundaryParticleField3D(lattice);

    /* Repulsive force */
    T k_int = 0 * 2 * 2.5e-7, DeltaX=1.0, R=0.2, k=2.;
    PowerLawForce<T> PLF(k_int, DeltaX, R, k);


    /* ************* BOND DYNAMICS ************************/
      
    /* Default */
    T De_adh =  2 * 2.5e-7, beta_adh =1.0, r0_adh=0.2, rcut_adh=5.0;
    AdhesiveMorsePotential<T> AdhForce(De_adh, beta_adh, r0_adh, rcut_adh);

   /*Define BondPotential From config */
   if (ChooseBondPotential == 0) {
      T De_adh, beta_adh, r0_adh, rcut_adh;
      De_adh= K_scale * 0.3 * kBT; 
      beta_adh =3.0;
      r0_adh=R_attach;
      rcut_adh=R_break;
      AdhesiveMorsePotential<T> AdhForce(De_adh, beta_adh, r0_adh, rcut_adh); 
      pcout << std::endl << "(main) Using AdhesiveMorsePotential " << std::endl;
   }
   else if (ChooseBondPotential == 1) {
      T epsilonLJ_adh, sigmaLJ_adh, rcut_adh;
      epsilonLJ_adh = K_scale * 550 * kBT;
      sigmaLJ_adh = 1.5 * R_attach;
      rcut_adh=R_break;
      AdhesiveLennardJonesPotential<T> AdhForce(epsilonLJ_adh, sigmaLJ_adh, rcut_adh); 
            pcout << std::endl << "(main) Using AdhesiveLennardJonesPotential " << std::endl;
   }
   else if (ChooseBondPotential == 2) {
      T H_adh, el_max_adh, r0_adh, rcut_adh;
      H_adh = K_scale * 1000/dx * kBT; 
      el_max_adh =10.0; 
      r0_adh=R_attach; 
      rcut_adh=R_break;
      AdhesiveFENEForce<T> AdhForce(H_adh, el_max_adh, r0_adh, rcut_adh, false, 0.0, 0.0); 
      pcout << std::endl << "(main) Using  AdhesiveFENEForce " << std::endl;
   }
   else {
      /* Default */
       T De_adh =  2 * 2.5e-7, beta_adh =1.0, r0_adh=0.2, rcut_adh=5.0;
       AdhesiveMorsePotential<T> AdhForce(De_adh, beta_adh, r0_adh, rcut_adh);
           pcout << std::endl << "(main) Using DEFAULT AdhesiveMorsePotential " << std::endl;
   }



   /* Default */
   trombocit::SimpleUnsaturatedBond<T,DESCRIPTOR> bondType(AdhForce, R_attach, R_break, true);

   /* Define BondType from config */
   if (ChooseBondType == 0) {
        trombocit::SimpleUnsaturatedBond<T,DESCRIPTOR> bondType(AdhForce, R_attach, R_break, true);
        pcout << std::endl << "(main) Using SimpleUnsaturatedBond " << std::endl;
   } 
   else if (ChooseBondType == 1) {
        T delta_saturation = 1, max_saturation = 5;
        trombocit::SimpleSaturatedBond<T,DESCRIPTOR> bondType(AdhForce, R_attach, R_break, delta_saturation, max_saturation,  true);
        pcout << std::endl << "(main) Using SimpleSaturatedBond " << std::endl;
   }
   else if (ChooseBondType == 2) {
        T delta_saturation0 = 1, max_saturation0 = 5, delta_saturation1 = 1, max_saturation1 = 5;
        trombocit::SimpleAsymmetricSaturatedBond<T,DESCRIPTOR> bondType(AdhForce, R_attach, R_break, delta_saturation0, max_saturation0, delta_saturation1, max_saturation1, true);
        pcout << std::endl << "(main) Using SimpleAsymmetricSaturatedBond " << std::endl;
   }
   else {
        /* Default */
        trombocit::SimpleUnsaturatedBond<T,DESCRIPTOR> bondType(AdhForce, R_attach, R_break, true);
        pcout << std::endl << "(main) Using DEFAULT SimpleUnsaturatedBond " << initIter << std::endl;
   }


  //  trombocit::SimpleUnsaturatedBond<T,DESCRIPTOR> bondType(PLF, 0.3, 1, true);
  //  trombocit::SimpleUnsaturatedBond<T,DESCRIPTOR> bondType(AM, 0.5, 2.0, true);
    BondField3D<T,DESCRIPTOR> bondField(PLTField, bondType);
    BondFieldWrapper3D<T,DESCRIPTOR> bondDynamics(bondField);
    bondDynamics.update();
    writeBondParticleField3D_HDF5(bondField.getBondParticles3D(), dx, dt, initIter, "BondFieldParticles");


    /* ****************************************************/

    /*      Sync all quantities    */
    SyncRequirements everyCCR(allReductions);
    for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
    	cellFields[iCell]->synchronizeCellQuantities(everyCCR);
    }
    /*            I/O              */
    global::timer("HDFOutput").start();
    bool invertXZ_for_XDMF=true;
    writeHDF5(lattice, parameters, initIter, invertXZ_for_XDMF);
    for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
        writeCellField3D_HDF5(*cellFields[iCell], dx, dt, initIter);
        writeCell3D_HDF5(*cellFields[iCell], dx, dt, initIter);
    }

    global::timer("HDFOutput").stop();

    SimpleFicsionProfiler simpleProfiler(tmeas);
    simpleProfiler.writeInitial(nx, ny, nz, -1, numVerticesPerCell);
    /* --------------------------- */
    const Array<T,3> headOnForce(stretchForceScalar, 0.0, 0.0);
    std::vector<Array<T,3> > forcesToApply(2);
    std::vector<plint> PLTCellIds(2);
    forcesToApply[0] = -headOnForce;
    forcesToApply[1] =  headOnForce;
    PLTCellIds[0] = 0; 
    PLTCellIds[1] = 1;
    std::vector<std::vector<plint> > verticesToStretch(2);

    verticesToStretch[0] = meshVerticesFromDirection(PLTField.getMesh(), 0,  1, 0.2);
    verticesToStretch[1] = meshVerticesFromDirection(PLTField.getMesh(), 0, -1, 0.2);

    global::timer("mainLoop").start();
	pcout << std::endl << "(main) Starting simulation i=" << initIter << std::endl;
    for (pluint iter=initIter; iter<tmax+1; ++iter) {
        // #1# Membrane Model
//       RBCField.applyConstitutiveModel();
//       RBCField.applyCellCellForce(PLF, R);
        for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
     	   cellFields[iCell]->applyConstitutiveModel();
        }
        // Pull force
//        applyForceToCells(PLTField, PLTCellIds, forcesToApply);
//        applySameCellFieldForces(PLTField, PLF, R*2);
        bondDynamics.update();
        applyForceToCells(PLTField, PLTCellIds, verticesToStretch, forcesToApply);
       // PLTField.applyCellCellForce(PLF, R*2);

        // #2# IBM Spreading
       cellFields[0]->setFluidExternalForce(poiseuilleForce);
       for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
    	   cellFields[iCell]->spreadForceIBM();
       }
        // #3# LBM
        if ((iter+1)%tmeas==0 && flowType==11) { lattice.toggleInternalStatistics(true); }
        global::timer("LBM").start();
        lattice.collideAndStream();
        global::timer("LBM").stop();
        for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
			// #4# IBM Interpolation
        	cellFields[iCell]->interpolateVelocityIBM();
			// #5# Position Update
        	cellFields[iCell]->advanceParticles();
        }

        // #6# Output
        if ((iter+1)%tmeas==0) {
            SyncRequirements everyCCR(allReductions);
            for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
            	cellFields[iCell]->synchronizeCellQuantities(everyCCR);
            }
            global::timer("HDFOutput").start();
            writeHDF5(lattice, parameters, iter+1, invertXZ_for_XDMF);
            for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
            	writeCellField3D_HDF5(*cellFields[iCell], dx, dt, iter+1);
            	writeCell3D_HDF5(*cellFields[iCell], dx, dt, iter+1);
                writeBondParticleField3D_HDF5(bondField.getBondParticles3D(), dx, dt, iter+1, "BondFieldParticles");
            }
            global::timer("HDFOutput").stop();
            if ((iter+1)%(2*tmeas)==0) {
                global::timer("Checkpoint").start();
                checkpointer.save(lattice, cellFields, iter+1);
                global::timer("Checkpoint").stop();
            }
            T dtIteration = global::timer("mainLoop").stop();
            simpleProfiler.writeIteration(iter+1);
            pcout << "(main) Iteration:" << iter + 1 << "; time "<< dtIteration*1.0/tmeas ;
//            pcout << "; Volume (" << RBCField[0]->getVolume() << ")";
            if (flowType==11) {
                T nu_lb = parameters.getLatticeNu();
                T coeff = nu_lb * nMomentumExchangeCells * shearRate; // * nMomentumExchangeCells;
                T drag =  lattice.getInternalStatistics().getSum(forceIds[0]) / coeff;
                T lift =  lattice.getInternalStatistics().getSum(forceIds[1]) / coeff;
                T other =  lattice.getInternalStatistics().getSum(forceIds[2]) / coeff;
                pcout << "; drag=" << drag
                      << "; lift=" << lift
                      << "; other=" << other
                      << "; nMomentumExchangeCells=" << nMomentumExchangeCells*1.0/(nx*nz);
            }
            pcout << std::endl;
        } else {
            for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
            	cellFields[iCell]->synchronizeCellQuantities();
            }
        }
        if ((iter+1)%tmeas==0 && flowType==11) { lattice.toggleInternalStatistics(false); }
    }
    for (pluint iCell=0; iCell<cellFields.size(); ++iCell) {
    	delete cellFields[iCell];
    	delete cellModels[iCell];
    }
    simpleProfiler.writeIteration(tmax+1);
    pcout << "Simulation finished." << std::endl;
}