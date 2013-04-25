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

#ifndef IMMERSED_WALL_PARTICLE_3D_H
#define IMMERSED_WALL_PARTICLE_3D_H

#include "core/globalDefs.h"
#include "core/array.h"
#include "particles/particle3D.h"
#include "particles/particleIdentifiers3D.h"
#include "atomicBlock/blockLattice3D.h"
#include <vector>

namespace plb {

template<typename T, template<typename U> class Descriptor>
class ImmersedCellParticle3D : public Particle3D<T,Descriptor> {
public:
    ImmersedCellParticle3D();
    ImmersedCellParticle3D( plint tag_, Array<T,3> const& position, plint cellId_ = -1 );
    ImmersedCellParticle3D( plint tag_, Array<T,3> const& position,
                          Array<T,3> const& v_, Array<T,3> const& vHalfTime_,
                            Array<T,3> const& a_, Array<T,3> const& force_, Array<T,3> const& vPrevious_,
                            plint cellId_ = -1);
    virtual void velocityToParticle(TensorField3D<T,3>& velocityField, T scaling=1.) { }
    virtual void rhoBarJtoParticle(NTensorField3D<T>& rhoBarJfield, bool velIsJ, T scaling=1.) { }
    virtual void fluidToParticle(BlockLattice3D<T,Descriptor>& fluid, T scaling=1.) { }
    /// Implements "steps 1 and 2" of the Verlet algorithm: given
    ///   x(t), v(t), and a(t), it computes v(t+1/2) and x(t+1).
    virtual void advance();
    virtual void serialize(HierarchicSerializer& serializer) const;
    virtual void unserialize(HierarchicUnserializer& unserializer);
    virtual int getId() const;
    virtual void reset(Array<T,3> const& position);
    virtual ImmersedCellParticle3D<T,Descriptor>* clone() const;
    /// Return the cellId through a generic interface (vector id=0).
    virtual bool getScalar(plint whichScalar, T& scalar) const;
    /// Return the velocity, acceleration or vHalfTime through a generic interface (vector id=0,1,2).
    virtual bool getVector(plint whichVector, Array<T,3>& vector) const;
    Array<T,3> const& get_v() const { return v; }
    Array<T,3> const& get_vHalfTime() const { return vHalfTime; }
    Array<T,3> const& get_vPrevious() const { return vPrevious; }
    Array<T,3> const& get_a() const { return a; }
    Array<T,3> const& get_force() const { return force; }
    plint const& get_cellId() const { return cellId; }
    Array<T,3>& get_v() { return v; }
    Array<T,3>& get_vHalfTime() { return vHalfTime; }
    Array<T,3>& get_vPrevious() { return vPrevious; }
    Array<T,3>& get_a() { return a; }
    Array<T,3>& get_force() { return force; }
    plint& get_cellId() { return cellId; }
private:
    Array<T,3> v, vHalfTime, a, force, vPrevious;
    static int id;
private:
    Array<T,3> f_wlc, f_bending, f_volume, f_surface, f_shear, f_viscosity;
private:
    plint cellId;
public:
    ImmersedCellParticle3D (
            plint tag_, Array<T,3> const& position,
            Array<T,3> const& v_, Array<T,3> const& vHalfTime_,
            Array<T,3> const& a_, Array<T,3> const& force_,  Array<T,3> const& vPrevious_,
            Array<T,3> const& f_wlc_, Array<T,3> const& f_bending_, Array<T,3> const& f_volume_, Array<T,3> const& f_surface_, Array<T,3> const& f_shear_, Array<T,3> const& f_viscosity_,
            plint cellId_ );

    Array<T,3> const& get_f_wlc() const { return f_wlc; }
    Array<T,3> const& get_f_bending() const { return f_bending; }
    Array<T,3> const& get_f_volume() const { return f_volume; }
    Array<T,3> const& get_f_surface() const { return f_surface; }
    Array<T,3> const& get_f_shear() const { return f_shear; }

    Array<T,3>& get_f_wlc() { return f_wlc; }
    Array<T,3>& get_f_bending() { return f_bending; }
    Array<T,3>& get_f_volume() { return f_volume; }
    Array<T,3>& get_f_surface() { return f_surface; }
    Array<T,3>& get_f_shear() { return f_shear; }
    Array<T,3>& get_f_viscosity() { return f_viscosity; }
};

namespace meta {

template<typename T, template<typename U> class Descriptor>
ParticleRegistration3D<T,Descriptor>& particleRegistration3D();


template< typename T,
          template<typename U> class Descriptor,
          class ImmersedCellParticle >
class ImmersedCellParticleGenerator3D : public ParticleGenerator3D<T,Descriptor>
{
    virtual Particle3D<T,Descriptor>* generate (
            HierarchicUnserializer& unserializer ) const
    {
        // tag, position, scalars, vectors.
        plint tag;
        unserializer.readValue(tag);
        Array<T,3> position;
        unserializer.readValues<T,3>(position);
        Array<T,3> v, vHalfTime, vPrevious, a, force;
        unserializer.readValues<T,3>(v);
        unserializer.readValues<T,3>(vHalfTime);
        unserializer.readValues<T,3>(a);
        unserializer.readValues<T,3>(force);
        unserializer.readValues<T,3>(vPrevious);
        Array<T,3> f_wlc, f_bending, f_volume, f_surface, f_shear, f_viscosity;
        unserializer.readValues<T,3>(f_wlc);
        unserializer.readValues<T,3>(f_bending);
        unserializer.readValues<T,3>(f_volume);
        unserializer.readValues<T,3>(f_surface);
        unserializer.readValues<T,3>(f_shear);
        unserializer.readValues<T,3>(f_viscosity);
        plint cellId;
        unserializer.readValue(cellId);
        return new ImmersedCellParticle(tag, position, v, vHalfTime, a, force, vPrevious,
                f_wlc, f_bending, f_volume, f_surface, f_shear, f_viscosity,
                cellId);
    }
};


template< typename T,
          template<typename U> class Descriptor,
          class ImmersedCellParticle >
int registerImmersedCellParticle3D(std::string name) {
    return particleRegistration3D<T,Descriptor>().announce (
               name, new ImmersedCellParticleGenerator3D<T,Descriptor,ImmersedCellParticle> );
}

}  // namespace meta

}  // namespace plb

#endif  // IMMERSED_WALL_PARTICLE_3D_H

