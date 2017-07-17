#ifndef VOXELIZEDOMAIN_CPP
#define VOXELIZEDOMAIN_CPP

#include "voxelizeDomain.h"
// ----------------------- Copy from neighbour ------------------------------------
void CopyFromNeighbor::process(
        Box3D domain, ScalarField3D<int> &field1) {
    for (plint iX = domain.x0; iX <= domain.x1; ++iX) {
        for (plint iY = domain.y0; iY <= domain.y1; ++iY) {
            for (plint iZ = domain.z0; iZ <= domain.z1; ++iZ) {
                field1.get(iX, iY, iZ) = field1.get(iX + offset[0], iY + offset[1], iZ + offset[2]);
            }
        }
    }
}

CopyFromNeighbor *CopyFromNeighbor::clone() const {
    return new CopyFromNeighbor(*this);
}

void CopyFromNeighbor::getTypeOfModification(std::vector<modif::ModifT> &modified) const {
    modified[0] = modif::allVariables;
}

BlockDomain::DomainT CopyFromNeighbor::appliesTo() const {
    return BlockDomain::bulk;
}


// ---------------------- Read in STL geometry ---------------------------------

void getFlagMatrixFromSTL(std::string meshFileName, plint extendedEnvelopeWidth, plint refDirLength, plint refDir,
                          VoxelizedDomain3D<double> *&voxelizedDomain, MultiScalarField3D<int> *&flagMatrix, plint blockSize) {
    plint extraLayer = 0;   // Make the bounding box larger; for visualization purposes
                            //   only. For the simulation, it is OK to have extraLayer=0.
    plint borderWidth = 1;  // Because the Guo boundary condition acts in a one-cell layer.
    
    // Requirement: margin>=borderWidth.
    plint margin = 1;  // Extra margin of allocated cells around the obstacle.

    TriangleSet<double> *triangleSet = new TriangleSet<double>(meshFileName, DBL);

    DEFscaledMesh<double> *defMesh =
            new DEFscaledMesh<double>(*triangleSet, refDirLength, refDir, margin, extraLayer);
    TriangleBoundary3D<double> boundary(*defMesh);
    delete defMesh;
    boundary.getMesh().inflate();

    voxelizedDomain = new VoxelizedDomain3D<double>(
            boundary, voxelFlag::inside, extraLayer, borderWidth, extendedEnvelopeWidth, blockSize);
    
    // Print out some info
    pcout << "(main) Voxelisation is done. Resulting domain parameters are: " << endl;
    pcout << getMultiBlockInfo(voxelizedDomain->getVoxelMatrix()) << std::endl;


    flagMatrix = new MultiScalarField3D<int>((MultiBlock3D &) voxelizedDomain->getVoxelMatrix());

    setToConstant(*flagMatrix, voxelizedDomain->getVoxelMatrix(),
                  voxelFlag::inside, flagMatrix->getBoundingBox(), 1);
    setToConstant(*flagMatrix, voxelizedDomain->getVoxelMatrix(),
                  voxelFlag::innerBorder, flagMatrix->getBoundingBox(), 1);


	// Since the domain is closed, open up the two ends by copying the slice before it.
    Box3D domainBox = flagMatrix->getBoundingBox();
    plint nx = domainBox.getNx();
    plint ny = domainBox.getNy();
    plint nz = domainBox.getNz();

    Box3D domain(0, 1, 0, ny - 1, 0, nz - 1);
    applyProcessingFunctional(new CopyFromNeighbor(hemo::Array<plint, 3>({1, 0, 0})), domain, *flagMatrix);

    domain = Box3D(nx - 2, nx - 1, 0, ny - 1, 0, nz - 1);
    applyProcessingFunctional(new CopyFromNeighbor(hemo::Array<plint, 3>({-1, 0, 0})), domain, *flagMatrix);

}

#endif
