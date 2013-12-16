#ifndef MESH_METRICS_HH
#define MESH_METRICS_HH

#include "meshMetrics.h"


template<typename T>
MeshMetrics<T>::MeshMetrics(TriangleBoundary3D<T> const& Cells)
    : meshQualityFile((global::directories().getLogOutDir() + "plbMeshQuality.log").c_str())
    {

    minArea=10000000; minLength=10000000; minAngle=10000000; minNn=10000000;
    maxArea=0; maxLength=0; maxAngle=0; maxNn=0;

    TriangularSurfaceMesh<T>  mesh = Cells.getMesh();
    Nv = mesh.getNumVertices();
    Nt = mesh.getNumTriangles();

    Nn=0; Nn6=0; Nn5=0; Nn7=0;
    area=0; length=0; angle=0;
    sigmaArea=0; sigmaLength=0; sigmaAngle=0; sigmaNn=0;
    T tmp;
    // Compute Mean Values
    pluint NEdges = 0;
    for (int iV = 0; iV < Nv; ++iV) {
        tmp = mesh.computeVertexArea(iV);
        minArea = minArea>tmp?tmp:minArea;
        maxArea = maxArea<tmp?tmp:maxArea;
        area += tmp;
        std::vector<plint> nvid = mesh.getNeighborVertexIds(iV);
        T NumNeighbors = nvid.size();
        Nn += nvid.size();
        minNn = minNn>NumNeighbors?NumNeighbors:minNn;
        maxNn = maxNn<NumNeighbors?NumNeighbors:maxNn;
        for (int ijV = 0; ijV < NumNeighbors; ++ijV) {
            int jV = nvid[ijV];
            tmp = mesh.computeEdgeLength(iV, jV);
            minLength = minLength>tmp?tmp:minLength;
            maxLength = maxLength<tmp?tmp:maxLength;
            length += tmp;
            tmp = calculateSignedAngle(mesh, iV, jV) * 180/3.14159;
            minAngle = minAngle>tmp?tmp:minAngle;
            maxAngle = maxAngle<tmp?tmp:maxAngle;
            angle += tmp;
            NEdges++;
        }
        if (NumNeighbors == 5) {
            Nn5 += 1.0;
        } else if (NumNeighbors == 6) {
            Nn6 += 1.0;
        } else if (NumNeighbors == 7) {
            Nn7 += 1.0;
        }
    }
    for (int iT = 0; iT < Nt; ++iT) {
        area += mesh.computeTriangleArea(iT);
    }
    Nn /= Nv; area /= Nt; length /= NEdges; angle /= NEdges;
    // Compute Sigmas
    for (int iV = 0; iV < Nv; ++iV) {
        tmp = mesh.computeVertexArea(iV) - area;
        sigmaArea += tmp*tmp;
        std::vector<plint> nvid = mesh.getNeighborVertexIds(iV);
        T NumNeighbors = nvid.size();
        tmp = nvid.size()-Nn;
        sigmaNn += tmp*tmp;
        for (int ijV = 0; ijV < NumNeighbors; ++ijV) {
            int jV = nvid[ijV];
            tmp=(mesh.computeEdgeLength(iV, jV)-length);
            sigmaLength += tmp*tmp;
            tmp = (calculateSignedAngle(mesh, iV, jV) * 180/3.14159 - angle);
            sigmaAngle += tmp*tmp;
        }
    }
    for (int iT = 0; iT < Nt; ++iT) {
        tmp = mesh.computeTriangleArea(iT) - area;
        sigmaArea += tmp*tmp;
    }
    sigmaNn = sqrt(sigmaNn/Nv);
    sigmaArea = sqrt(sigmaArea/Nt);
    sigmaLength = sqrt(sigmaLength/NEdges);
    sigmaAngle = sqrt(sigmaAngle/NEdges);
}


template<typename T>
MeshMetrics<T>::~MeshMetrics() {

};

template<typename T>
void MeshMetrics<T>::write(plb_ofstream & meshFile) {
    meshFile << "# Deviation in %, defined as 100*sigma(l)/mean(l), sl =  0" << std::endl;

    meshFile << "Number of vertices, Nv =  " << Nv << std::endl;
    meshFile << "Number of triangles, Nt =  " << Nt << std::endl;
    meshFile << std::endl;
    meshFile << "Mean Area per face, A =  " << area << std::endl;
    meshFile << "Deviation of Area %, sA =  " << 100*sigmaArea/area << std::endl;
    meshFile << "Max Area of face, maxA =  " << maxArea << std::endl;
    meshFile << "Min Area of face, minA =  " << minArea << std::endl;
    meshFile << std::endl;
    meshFile << "Mean Length per vertex, L =  " << length << std::endl;
    meshFile << "Deviation of Length %, sL =  " << 100*sigmaLength/length << std::endl;
    meshFile << "Max Length of edge, maxA =  " << maxLength << std::endl;
    meshFile << "Min Length of edge, minA =  " << minLength << std::endl;
    meshFile << std::endl;
    meshFile << "Mean Angle per vertex, theta =  " << angle << std::endl;
    meshFile << "Deviation of Angle %, sTheta =  " << fabs(100*sigmaAngle/angle) << std::endl;
    meshFile << "Max Angle of edge, maxA =  " << maxAngle << std::endl;
    meshFile << "Min Angle of edge, minA =  " << minAngle << std::endl;
    meshFile << std::endl;
    meshFile << "Mean number of neighbours, Nn =  " << Nn << std::endl;
    meshFile << "Deviation for number of Neighbours %, sNn =  " << 100*sigmaNn/Nn << std::endl;
    meshFile << "Number of 5 neighbour in %, Nn5 =  " << 100*Nn5/Nv << std::endl;
    meshFile << "Number of 6 neighbour in %, Nn6 =  " << 100*Nn6/Nv << std::endl;
    meshFile << "Number of 7 neighbour in %, Nn7 =  " << 100*Nn7/Nv << std::endl;
};


#endif  // MESH_METRICS_HH