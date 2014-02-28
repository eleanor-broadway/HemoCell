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

#ifndef REDUCTION_PARTICLE_3D_H
#define REDUCTION_PARTICLE_3D_H

#include "palabos3D.h"
#include "palabos3D.hh"
#include <vector>
#include <map>
#include <algorithm>
#include "cellReductionTypes.h"
using namespace plb;
using namespace std;

namespace plb {

template<typename T, template<typename U> class Descriptor>
class ReductionParticle3D : public Particle3D<T,Descriptor> {
public:
    ReductionParticle3D();
    ReductionParticle3D(plint tag_, Array<T,3> const& position);
    ReductionParticle3D(plint tag_, Array<T,3> const& position,
                plint cellId_, plint processor_, plint nParticles_,
                std::map<plint, T > const& quantities1D_,
                std::map<plint, Array<T,3> > const& quantities3D_,
                std::map<plint, std::vector<T> > const& quantitiesND_);

    virtual void velocityToParticle(TensorField3D<T,3>& velocityField, T scaling=1.) { }
    virtual void rhoBarJtoParticle(NTensorField3D<T>& rhoBarJfield, bool velIsJ, T scaling=1.) { }
    virtual void fluidToParticle(BlockLattice3D<T,Descriptor>& fluid, T scaling=1.) { }
    /// Implements Euler integration with velocity alone.
    virtual void advance();
    virtual void serialize(HierarchicSerializer& serializer) const;
    virtual void unserialize(HierarchicUnserializer& unserializer);
    virtual ReductionParticle3D<T,Descriptor>* clone() const;

    plint getMpiProcessor() {
    	int myrank = 0;
#ifdef PLB_MPI_PARALLEL
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif
    	return plint(myrank); }
private:
    static int id ;
    plint cellId;
    plint processor;
    plint nParticles;
    std::map<plint, T > quantities1D; // quantities1D[CCR_VOLUME] = CELL_VOLUME
    std::map<plint, Array<T,3> > quantities3D;
    std::map<plint, std::vector<T> > quantitiesND;
public:
    void insert(plint ccrId, T value) { quantities1D[ccrId] = value; }
    void insert(plint ccrId, Array<T,3> value) { quantities3D[ccrId] = value; }
    void insert(plint ccrId, std::vector<T> value) { quantitiesND[ccrId] = value; }
    T const& get1D(plint ccrId) { return quantities1D[ccrId]; }
    Array<T,3> const& get3D(plint ccrId) { return quantities3D[ccrId]; }
    std::vector<T> const& getND(plint ccrId) { return quantitiesND[ccrId]; }

    std::map<plint, T >& getQuantities1D() { return quantities1D; }; // quantities1D[CCR_VOLUME] = CELL_VOLUME
    std::map<plint, Array<T,3> >& getQuantities3D() { return quantities3D; }; // quantities1D[CCR_VOLUME] = CELL_VOLUME
    std::map<plint, std::vector<T> >& getQuantitiesND() { return quantitiesND; }; // quantities1D[CCR_VOLUME] = CELL_VOLUME

public:
    virtual int getId() const;

    plint const& get_cellId() const { return cellId; }
    plint& get_cellId() { return cellId; }
    plint const& get_processor() const { return processor; }
    plint& get_processor() { return processor; }
    plint const& get_nParticles() const { return nParticles; }
    plint& get_nParticles() { return nParticles; }

    T const& getVolume() const { return quantities1D[CCR_VOLUME]; }
    T& getVolume() { return quantities1D[CCR_VOLUME]; }

    T const& getSurface() const { return quantities1D[CCR_SURFACE]; }
    T& getSurface() { return quantities1D[CCR_SURFACE]; }
// General framework for getting data from the particles:
    /// Return the cellId through a generic interface (vector id=0).
    virtual bool getScalar(plint whichScalar, T& scalar);
    std::string getScalarName(plint whichScalar);
    plint getScalarsNumber() ;
    /// Return the velocity, acceleration or pbcPosition through a generic interface (vector id=0,1,2).
    virtual bool getVector(plint whichVector, Array<T,3>& vector);
    std::string getVectorName(plint whichVector);
    plint getVectorsNumber()  ;
    /// Return the velocity, acceleration or pbcPosition through a generic interface (vector id=0,1,2).
    bool getTensor(plint whichTensor, std::vector<T>& tensor);
    std::string getTensorName(plint whichTensor);
    plint getTensorsNumber() ;
private:
    std::vector<plint> scalar_ccrIds, vector_ccrIds, tensor_ccrIds;
    void make_ccrIds();
};


template<typename T, template<typename U> class Descriptor>
int ReductionParticle3D<T,Descriptor>::id = meta::registerGenericParticle3D<T,Descriptor,ReductionParticle3D<T,Descriptor> >("ReductionParticle3D");

template<typename T>
void serializeVector(HierarchicSerializer& serializer, std::vector<T> const& vec);

template<typename T>
std::vector<T> unserializeVector(HierarchicUnserializer& unserializer);




template<typename T, template<typename U> class Descriptor>
void writeCellPointsVTK(
        std::vector<Particle3D<T,Descriptor>*> particles, T const& dx,
                    std::string fName)
{
    std::ofstream ofile(fName.c_str());
    plint nParticles = particles.size();

    std::vector<std::string> scalarNames;
    std::vector<std::string> vectorNames;
    std::vector<plint> cellIds;

    std::map<std::string, std::map<plint, T> > scalarData; // scalars["Position"][cellId] = value
    std::map<std::string, std::map<plint, Array<T,3> > > vectorData; // scalars["Position"][cellId] = vectorValues

    ReductionParticle3D<T,Descriptor>* p0 =
        dynamic_cast<ReductionParticle3D<T,Descriptor>*> (particles[0]);
    for (plint iVector = 0; iVector < p0->getVectorsNumber(); ++iVector) { vectorNames.push_back(p0->getVectorName(iVector)); }
    for (plint iScalar = 0; iScalar < p0->getScalarsNumber(); ++iScalar) { scalarNames.push_back(p0->getScalarName(iScalar)); }
    cellIds.push_back(p0->get_cellId());
    for (pluint i = 1; i < nParticles; ++i) {
        p0 = dynamic_cast<ReductionParticle3D<T,Descriptor>*> (particles[i]);
        plint cellId = p0->get_cellId();
        cellIds.push_back(cellId);
        for (plint iVector = 0; iVector < p0->getVectorsNumber(); ++iVector) { p0->getVector(iVector, vectorData[p0->getVectorName(iVector)][cellId]); }
        for (plint iScalar = 0; iScalar < p0->getScalarsNumber(); ++iScalar) { p0->getScalar(iScalar, scalarData[p0->getScalarName(iScalar)][cellId]); }
    }

    ofile << "# vtk DataFile Version 3.0\n";
    ofile << "Surface point created with Palabos and ficsion\n";
    ofile << "ASCII\n";
    ofile << "DATASET POLYDATA\n";
    ofile << "POINTS " << nParticles
          << (sizeof(T)==sizeof(double) ? " double" : " float")
          << "\n";
    for (pluint i = 0; i < nParticles; ++i) {
        Array<T,3> vertex = particles[i]->getPosition();
        vertex *= dx;
        ofile << vertex[0] << " " <<vertex[1] << " " <<vertex[2] << "\n";
    }
    ofile << "POINT_DATA " << nParticles << "\n";
    ofile << "SCALARS ID float 1 \n";
    ofile << "LOOKUP_TABLE default \n";
    for (pluint i = 0; i < nParticles; ++i) {
        ofile << cellIds[i] << "\n";
    }

    for (pluint i = 0; i < scalarNames.size(); ++i) {
        ofile << "SCALARS "<< scalarNames[i] << " double 1\n"
              << "LOOKUP_TABLE default\n";
        for (plint ic=0; ic<(plint)cellIds.size(); ++ic) {
                ofile << scalarData[scalarNames[i]][cellIds[ic]] << "\n";
        }
            ofile << "\n";
    }

    for (pluint i = 0; i < vectorNames.size(); ++i) {
        ofile << "VECTORS "<< vectorNames[i] << " double 1\n"
              << "LOOKUP_TABLE default\n";
        for (plint ic=0; ic<(plint)cellIds.size(); ++ic) {
            Array<T,3> vec = vectorData[vectorNames[i]][cellIds[ic]];
            ofile << vec[0] << " "
                  << vec[1] << " "
                  << vec[2] << "\n";
        }
        ofile << "\n";
    }

    ofile.close();
}


}  // namespace plb

#include "reductionParticle3D.hh"

#endif  // REDUCTION_PARTICLE_3D_H
