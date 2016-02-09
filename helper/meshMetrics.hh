#ifndef MESH_METRICS_HH
#define MESH_METRICS_HH

#include "meshMetrics.h"




template<typename T>
MeshMetrics<T>::MeshMetrics(MeshMetrics<T> const& rhs) : mesh(rhs.mesh)  {
    init();
}


template<typename T>
MeshMetrics<T>::MeshMetrics(TriangleBoundary3D<T> const& Cells) : mesh(Cells.getMesh())  {
    init();
}


template<typename T>
MeshMetrics<T>::MeshMetrics(TriangularSurfaceMesh<T> const& mesh_) : mesh(mesh_)  {
    init();
}

template<typename T>
void MeshMetrics<T>::init()    {
    minArea=std::numeric_limits<T>::max(); minLength=std::numeric_limits<T>::max();
    minAngle=std::numeric_limits<T>::max(); minNn=std::numeric_limits<T>::max();
    maxArea=0; maxLength=0; maxAngle=0; maxNn=0;
    meanVertexPosition.resetToZero();

    Nv = mesh.getNumVertices();
    Nt = mesh.getNumTriangles();
    Array<T,2> xRange;
    Array<T,2> yRange;
    Array<T,2> zRange;
    mesh.computeBoundingBox (xRange, yRange, zRange);
    cellRadius = max(xRange[1] - xRange[0], yRange[1] - yRange[0]);
    cellRadius = max(cellRadius , zRange[1] - zRange[0]) * 0.5;

    Nn=0; Nn6=0; Nn5=0; Nn7=0;
    area=0; length=0; angle=0;
    T varArea=0, varLength=0, varAngle=0, varNn=0;
    T tmp;
    // Vertices
    pluint NEdges = 0;
    volume = 0.0;
    for (int iV = 0; iV < Nv; ++iV) {
        meanVertexPosition += mesh.getVertex(iV);
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
            tmp = calculateSignedAngle(mesh, iV, jV);
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
        std::vector<plint> neighborTriangleIds = mesh.getNeighborTriangleIds(iV);
        for (pluint iB = 0; iB < neighborTriangleIds.size(); ++iB) {
            plint iTriangle = neighborTriangleIds[iB];
            Array<T,3> v0 = mesh.getVertex(iTriangle, 0);
            Array<T,3> v1 = mesh.getVertex(iTriangle, 1);
            Array<T,3> v2 = mesh.getVertex(iTriangle, 2);
            Array<T,3> tmp;
            crossProduct(v1, v2, tmp);
            T triangleVolumeT6 =  VectorTemplateImpl<T,3>::scalarProduct(v0,tmp);
            volume += triangleVolumeT6/6.0/3.0; // every volume is evaluated 3 times
        }
    }
    Nn /= Nv; length /= NEdges; angle /= NEdges;
    meanVertexPosition /= Nv;

// Compute vars
    for (int iV = 0; iV < Nv; ++iV) {
        std::vector<plint> nvid = mesh.getNeighborVertexIds(iV);
        T NumNeighbors = nvid.size();
        tmp = nvid.size()-Nn;
        varNn += tmp*tmp;
        for (int ijV = 0; ijV < NumNeighbors; ++ijV) {
            int jV = nvid[ijV];
            tmp=(mesh.computeEdgeLength(iV, jV)-length);
            varLength += tmp*tmp;
            tmp = (calculateSignedAngle(mesh, iV, jV) - angle);
            varAngle += tmp*tmp;
        }
    }
// Triangles
    area=0;
    for (int iT = 0; iT < Nt; ++iT) {
        tmp = mesh.computeTriangleArea(iT);
        minArea = minArea>tmp?tmp:minArea;
        maxArea = maxArea<tmp?tmp:maxArea;
        area += tmp;
    }
    area /= Nt; 
    for (int iT = 0; iT < Nt; ++iT) {
        tmp = mesh.computeTriangleArea(iT) - area;
        varArea += tmp*tmp;
    }
    sigmaNn = sqrt(varNn/Nv);
    sigmaArea = sqrt(varArea/Nt);
    sigmaLength = sqrt(varLength/NEdges);
    sigmaAngle = sqrt(varAngle/NEdges);
}

template<typename T>
MeshMetrics<T>::~MeshMetrics() {

};

template<typename T>
void MeshMetrics<T>::write(plb_ofstream & meshFile) {
    meshFile << "# Deviation in %, defined as 100*sigma(l)/mean(l), sl =  0" << std::endl;

    meshFile << "Number of vertices, Nv =  " << Nv << std::endl;
    meshFile << "Number of triangles, Nt =  " << Nt << std::endl;
    meshFile << "Surface, S =  " << getSurface() << std::endl;
    meshFile << "Volume, V =  " << getVolume() << std::endl;
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



template<typename T>
void writeSurfaceMeshAsciiSTL(TriangularSurfaceMesh<T> const& mesh, std::string fname)
{
	T dx = 1;
    // Output only from one MPI process.

    FILE *fp = fopen(fname.c_str(), "w");
    PLB_ASSERT(fp != NULL);

    char fmt1[64] = "  facet normal ";
    char fmt2[64] = "      vertex ";
    if (sizeof(T) == sizeof(long double)) {
        strcat(fmt1, "% Le % Le % Le\n");
        strcat(fmt2, "% Le % Le % Le\n");
    }
    else if (sizeof(T) == sizeof(float) ||
             sizeof(T) == sizeof(double)) {
        strcat(fmt1, "% e % e % e\n");
        strcat(fmt2, "% e % e % e\n");
    }
    else {
        PLB_ASSERT(false);
    }

    fprintf(fp, "solid surface\n");
    for (plint i = 0; i < mesh.getNumTriangles(); i++) {
        Array<T,3> n = mesh.computeTriangleNormal(i);
        Array<T,3> v;
        fprintf(fp, fmt1, n[0], n[1], n[2]);
        fprintf(fp, "    outer loop\n");
        v = dx * mesh.getVertex(i, 0);
        fprintf(fp, fmt2, v[0], v[1], v[2]);
        v = dx * mesh.getVertex(i, 1);
        fprintf(fp, fmt2, v[0], v[1], v[2]);
        v = dx * mesh.getVertex(i, 2);
        fprintf(fp, fmt2, v[0], v[1], v[2]);
        fprintf(fp, "    endloop\n");
        fprintf(fp, "  endfacet\n");
    }
    fprintf(fp, "endsolid surface\n");

    fclose(fp);
}


#endif  // MESH_METRICS_HH
