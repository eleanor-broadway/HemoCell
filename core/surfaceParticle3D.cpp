#ifndef SURFACE_PARTICLE_3D_HH
#define SURFACE_PARTICLE_3D_HH

#include "surfaceParticle3D.h"
#include <limits>       // std::numeric_limits



/* *************** class ImmersedCellParticle3D ************************************ */


SurfaceParticle3D::SurfaceParticle3D(){
  force_volume = &force; //These pointers are only changed for nice outputs
  force_area = &force; //These pointers are only changed for nice outputs
  force_inplane = &force; //These pointers are only changed for nice outputs
  force_bending = &force; //These pointers are only changed for nice outputs
}
SurfaceParticle3D::SurfaceParticle3D (Array<double,3> const& position, plint cellId_, plint vertexId_,pluint celltype_)
    : Particle3D<double,DESCRIPTOR>(-1, position), // The cellId initializor does nothing
      v(),
      force(),
      vPrevious(),
      cellId(cellId_), 
      vertexId(vertexId_),
      celltype(celltype_),
      rank(getMpiProcessor())
{
  force_volume = &force; //These pointers are only changed for nice outputs
  force_area = &force; //These pointers are only changed for nice outputs
  force_inplane = &force; //These pointers are only changed for nice outputs
  force_bending = &force; //These pointers are only changed for nice outputs
}


void SurfaceParticle3D::advance(){

    /* scheme:
     *  1: Euler
     *  2: Adams-Bashforth
     */
    #if HEMOCELL_MATERIAL_INTEGRATION == 1
          this->getPosition() += v;         

    #elif HEMOCELL_MATERIAL_INTEGRATION == 2
            Array<double,3> dxyz = (1.5*v - 0.5*vPrevious);
        	this->getPosition() +=  dxyz;
        	//pbcPosition += dxyz;
        	
        	vPrevious = v;  // Store velocity
    #endif
    v = {0.0,0.0,0.0};
}


void SurfaceParticle3D::reset(Array<double,3> const& position_, Array<double,3> const& velocity_) {
        Particle3D<double,DESCRIPTOR>::reset(position_);
        v = velocity_;
        vPrevious = velocity_;
        resetForces();
        rank = this->getMpiProcessor();
}


void SurfaceParticle3D::resetForces() {
        force.resetToZero();
}


void SurfaceParticle3D::reset(Array<double,3> const& position_)
{
        reset(position_,Array<double,3>(0,0,0));
}

void SurfaceParticle3D::serialize(HierarchicSerializer& serializer) const
{
    Particle3D<double,DESCRIPTOR>::serialize(serializer);
    serializer.addValues<double,3>(v);
    serializer.addValues<double,3>(force);
    serializer.addValues<double,3>(vPrevious);
    serializer.addValue<int>(rank);
    serializer.addValue<plint>(cellId);
    serializer.addValue<plint>(vertexId);
    serializer.addValue<pluint>(celltype);
}

void SurfaceParticle3D::unserialize(HierarchicUnserializer& unserializer)
{
    Particle3D<double,DESCRIPTOR>::unserialize(unserializer);
    unserializer.readValues<double,3>(v);
    unserializer.readValues<double,3>(force);
    unserializer.readValues<double,3>(vPrevious);
    unserializer.readValue<int>(rank);
    unserializer.readValue<plint>(cellId);
    unserializer.readValue<plint>(vertexId);
    unserializer.readValue<pluint>(celltype);
    force_volume = &force; //These pointers are only changed for nice outputs
    force_area = &force; //These pointers are only changed for nice outputs
    force_inplane = &force; //These pointers are only changed for nice outputs
    force_bending = &force; //These pointers are only changed for nice outputs
}


SurfaceParticle3D* SurfaceParticle3D::clone() const {
    SurfaceParticle3D* sparticle = new SurfaceParticle3D(*this);
    sparticle->force_volume = &sparticle->force;
    sparticle->force_bending = &sparticle->force;
    sparticle->force_inplane = &sparticle->force;
    sparticle->force_area = &sparticle->force;
    return sparticle;
}

void SurfaceParticle3D::velocityToParticle(TensorField3D<double,3>& velocityField, double scaling) { }
void SurfaceParticle3D::velocityToParticle(NTensorField3D<double>& velocityField, double scaling) { }
void SurfaceParticle3D::rhoBarJtoParticle(NTensorField3D<double>& rhoBarJfield, bool velIsJ, double scaling) { }
void SurfaceParticle3D::fluidToParticle(BlockLattice3D<double,DESCRIPTOR>& fluid, double scaling) { }
   int SurfaceParticle3D::getId() const {return id;}

Array<double,3> const& SurfaceParticle3D::get_v() const { return v; }
Array<double,3>& SurfaceParticle3D::get_v() {return v;}
Array<double,3>& SurfaceParticle3D::get_force() {return force;}



Array<double,3> const& SurfaceParticle3D::getVelocity() const { return get_v(); }
Array<double,3> const& SurfaceParticle3D::get_vPrevious() const { return vPrevious; }
Array<double,3> const& SurfaceParticle3D::get_force() const { return force; }
plint const& SurfaceParticle3D::get_cellId() const { return cellId; }
pluint const& SurfaceParticle3D::get_celltype() const { return celltype; }
plint const& SurfaceParticle3D::getVertexId() const { return vertexId; }
int SurfaceParticle3D::getMpiProcessor() { MPI_Comm_rank(MPI_COMM_WORLD, &rank); return rank;}

int SurfaceParticle3D::id = meta::registerGenericParticle3D<double,DESCRIPTOR,SurfaceParticle3D>("SurfaceParticle3D");



#endif  // SURFACE_PARTICLE_3D_HH