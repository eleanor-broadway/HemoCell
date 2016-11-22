#ifndef SURFACE_PARTICLE_3D_HH
#define SURFACE_PARTICLE_3D_HH

#include "surfaceParticle3D.h"
#include <limits>       // std::numeric_limits



namespace plb {

/* *************** class ImmersedCellParticle3D ************************************ */


template<typename T, template<typename U> class Descriptor>
SurfaceParticle3D<T,Descriptor>::SurfaceParticle3D(SurfaceParticle3D<T,Descriptor> const& rhs)
    : Particle3D<T,Descriptor>(rhs.getTag(), rhs.getPosition()), v(rhs.v), pbcPosition(rhs.pbcPosition), a(rhs.a),
      force(rhs.force), vPrevious(rhs.vPrevious), processor(rhs.processor), cellId(rhs.cellId), vertexId(rhs.vertexId),
      bondTypeSaturation(rhs.bondTypeSaturation), dt(rhs.dt)
{
}


template<typename T, template<typename U> class Descriptor>
SurfaceParticle3D<T,Descriptor>::SurfaceParticle3D()
    : Particle3D<T,Descriptor>(),
      v(T(),T(),T()),
      pbcPosition(this->getPosition()),
      a(T(),T(),T()), force(T(),T(),T()), vPrevious(T(),T(),T()),
#ifdef PLB_DEBUG // Less Calculations
      f_wlc(T(),T(),T()), f_bending(T(),T(),T()), f_volume(T(),T(),T()),
      f_surface(T(),T(),T()), f_shear(T(),T(),T()), f_viscosity(T(),T(),T()),
      f_repulsive(T(),T(),T()),
      stress(T(),T(),T()),
      E_other(T()),
      E_inPlane(T()), E_bending(T()), E_area(T()),  E_volume(T()),
      E_repulsive(T()),
#endif
      processor(getMpiProcessor()), cellId(this->getTag()), vertexId(0),
      bondTypeSaturation()
{ }

template<typename T, template<typename U> class Descriptor>
SurfaceParticle3D<T,Descriptor>::SurfaceParticle3D (Array<T,3> const& position, plint cellId_, plint vertexId_, T dt_ )
    : Particle3D<T,Descriptor>(cellId_, position),
      v(T(),T(),T()),
      pbcPosition(position),
      a(T(),T(),T()),
      dt(dt_),
      force(T(),T(),T()),
      vPrevious(T(),T(),T()),
#ifdef PLB_DEBUG // Less Calculations
      f_wlc(T(),T(),T()), f_bending(T(),T(),T()), f_volume(T(),T(),T()), f_surface(T(),T(),T()), f_shear(T(),T(),T()),
      f_viscosity(T(),T(),T()), f_repulsive(T(),T(),T()),
      stress(T(),T(),T()),
      E_other(T()),
      E_inPlane(T()), E_bending(T()), E_area(T()),  E_volume(T()), E_repulsive(T()),
#endif
      processor(getMpiProcessor()), cellId(cellId_), vertexId(vertexId_), bondTypeSaturation()
{ }

template<typename T, template<typename U> class Descriptor>
SurfaceParticle3D<T,Descriptor>::SurfaceParticle3D (
        Array<T,3> const& position,
        Array<T,3> const& v_, Array<T,3> const& pbcPosition_,
        Array<T,3> const& a_, Array<T,3> const& force_,  Array<T,3> const& vPrevious_, plint cellId_, plint vertexId_, T dt_)
    : Particle3D<T,Descriptor>(cellId_, position),
      v(v_),
      pbcPosition(pbcPosition_),
      a(a_),
      force(force_),
      dt(dt_),
      vPrevious(vPrevious_),
#ifdef PLB_DEBUG // Less Calculations
      f_wlc(T(),T(),T()), f_bending(T(),T(),T()), f_volume(T(),T(),T()), f_surface(T(),T(),T()), f_shear(T(),T(),T()), f_viscosity(T(),T(),T()),
      f_repulsive(T(),T(),T()),
      stress(T(),T(),T()),
      E_other(T()),
      E_inPlane(T()), E_bending(T()), E_area(T()),  E_volume(T()), E_repulsive(T()),
#endif
      processor(getMpiProcessor()), cellId(cellId_), vertexId(vertexId_), bondTypeSaturation()
{ }


template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::advance() {
    // No fluid interaction
    //    v += force;
    //    this->getPosition() += v + 0.5*force;
    // Velocity Verlet
    //    pbcPosition = v + (T)0.5*force;
    //    this->getPosition() += pbcPosition;
    // Adams-Bashforth update scheme
    //    this->getPosition() += 1.5*v - 0.5*vPrevious;
    //    vPrevious = v;

    /* scheme:
     *  1: Euler
     *  2: Adams-Bashforth
     */
    #if HEMOCELL_MATERIAL_INTEGRATION == 1
            Array<T,3> dx = v * dt;
        	this->getPosition() += dx;         
        	pbcPosition += dx;                 

    #elif HEMOCELL_MATERIAL_INTEGRATION == 2
            Array<T,3> dx = (1.5*v - 0.5*vPrevious)*dt;
        	this->getPosition() +=  dx;
        	pbcPosition += dx;
        	
        	vPrevious = v;  // Store velocity
    #endif

        // Reset current velocity
        // v.resetToZero();

        // Get the current processor (TODO: why here?)
        processor = this->getMpiProcessor();
}

template<typename T, template<typename U> class Descriptor>
int SurfaceParticle3D<T,Descriptor>::getId() const {
    return id;
}

template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::reset(Array<T,3> const& position_, Array<T,3> const& velocity_, bool allVariables) {
        Particle3D<T,Descriptor>::reset(position_);
        if (allVariables) {
            pbcPosition = position_;
        }

        v = velocity_;
        vPrevious = velocity_;
        vProgressed = velocity_;

        a.resetToZero();
        force.resetToZero();
#ifdef PLB_DEBUG // Less Calculations
        f_wlc.resetToZero();
        f_bending.resetToZero();
        f_volume.resetToZero();
        f_surface.resetToZero();
        f_shear.resetToZero();
        f_viscosity.resetToZero();
        f_repulsive.resetToZero();
        stress.resetToZero();

        E_other = T();
        E_inPlane = T();
        E_bending = T();
        E_area = T();
        E_volume = T();
        E_repulsive = T();
#endif

        processor = this->getMpiProcessor();
}


template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::resetForces() {
        a.resetToZero();
        force.resetToZero();
#ifdef PLB_DEBUG // Less Calculations
        f_wlc.resetToZero();
        f_bending.resetToZero();
        f_volume.resetToZero();
        f_surface.resetToZero();
        f_shear.resetToZero();
        f_viscosity.resetToZero();
        f_repulsive.resetToZero();
        stress.resetToZero();
        E_other = T();
        E_inPlane = T();
        E_bending = T();
        E_area = T();
        E_volume = T();
        E_repulsive = T();
#endif

        processor = this->getMpiProcessor();
}


template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::reset(Array<T,3> const& position_)
{
        reset(position_, Array<T,3>(0.,0.,0.));
}

template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::serialize(HierarchicSerializer& serializer) const
{
    Particle3D<T,Descriptor>::serialize(serializer);
    serializer.addValues<T,3>(v);
    serializer.addValues<T,3>(pbcPosition);
    serializer.addValues<T,3>(vProgressed);
    serializer.addValues<T,3>(force);
    serializer.addValues<T,3>(vPrevious);
    serializer.addValue<plint>(processor);
    serializer.addValue<plint>(cellId);
    serializer.addValue<plint>(vertexId);
    serializer.addValue<T>(dt);
//    if (trombosit::useTrombosit) {     }
    serializeMap<plint, T>(serializer, bondTypeSaturation);
}

template<typename T, template<typename U> class Descriptor>
void SurfaceParticle3D<T,Descriptor>::unserialize(HierarchicUnserializer& unserializer)
{
    Particle3D<T,Descriptor>::unserialize(unserializer);
    unserializer.readValues<T,3>(v);
    unserializer.readValues<T,3>(pbcPosition);
    unserializer.readValues<T,3>(vProgressed);
    unserializer.readValues<T,3>(force);
    unserializer.readValues<T,3>(vPrevious);
    unserializer.readValue<plint>(processor);
    unserializer.readValue<plint>(cellId);
    unserializer.readValue<plint>(vertexId);
    unserializer.readValue<T>(dt);
    //    if (trombosit::useTrombosit) {     }
    bondTypeSaturation = unserializeMap<plint, T>(unserializer);

}


template<typename T, template<typename U> class Descriptor>
SurfaceParticle3D<T,Descriptor>* SurfaceParticle3D<T,Descriptor>::clone() const {
    return new SurfaceParticle3D<T,Descriptor>(*this);
}


template<typename T, template<typename U> class Descriptor>
bool SurfaceParticle3D<T,Descriptor>::getVector(plint whichVector, Array<T,3>& vector) const {
    if (whichVector==0) {
        vector = get_pbcPosition();
        return true;
    } else if (whichVector==1) {
        vector = get_v();
        return true;
    } else if (whichVector==2) {
        vector = get_force();
        return true;
#ifdef PLB_DEBUG // Less Calculations
    } else if (whichVector==3) {
        vector = get_a();
        return true;
    } else if (whichVector==4) {
        vector = get_v();
        return true;
    } else if (whichVector==5) {
        vector = get_f_wlc();
        return true;
    } else if (whichVector==6) {
        vector = get_f_bending();
        return true;
    } else if (whichVector==7) {
        vector = get_f_volume();
        return true;
    } else if (whichVector==8) {
        vector = get_f_surface();
        return true;
    } else if (whichVector==9) {
        vector = get_f_shear();
        return true;
    } else if (whichVector==10) {
        vector = get_f_viscosity();
        return true;
    } else if (whichVector==11) {
        vector = get_f_repulsive();
        return true;
    } else if (whichVector==12) {
        vector = get_stress();
        return true;
#endif
    }
    return Particle3D<T,Descriptor>::getVector(whichVector, vector);
}

template<typename T, template<typename U> class Descriptor>
std::string SurfaceParticle3D<T,Descriptor>::getVectorName(plint whichVector) const {
    if (whichVector==0) {
        return "pbcPosition";
    } else if (whichVector==1) {
        return "velocity";
    } else if (whichVector==2) {
        return "force";
#ifdef PLB_DEBUG // Less Calculations
    } else if (whichVector==3) {
        return "acceleration";
    } else if (whichVector==4) {
        return "v";
    } else if (whichVector==5) {
        return "f_wlc";
    } else if (whichVector==6) {
        return "f_bending";
    } else if (whichVector==7) {
        return "f_volume";
    } else if (whichVector==8) {
        return "f_surface";
    } else if (whichVector==9) {
        return "f_shear";
    } else if (whichVector==10) {
        return "f_viscosity";
    } else if (whichVector==11) {
        return "f_repulsive";
    } else if (whichVector==12) {
        return "stress";
#endif
    }
    return "empty";
}

template<typename T, template<typename U> class Descriptor>
plint SurfaceParticle3D<T,Descriptor>::getVectorsNumber() const {
#ifdef PLB_DEBUG // Less Calculations
        return 13;
#else
        return 3;
#endif
}

/* Same for scalars */
template<typename T, template<typename U> class Descriptor>
bool SurfaceParticle3D<T,Descriptor>::getScalar(plint whichScalar, T& scalar) const {
    if (whichScalar==0) {
    	scalar = T(this->getVertexId());
        return true;
    } else if (whichScalar==1) {
        scalar = T(get_cellId());
        return true;
    } else if (whichScalar==2) {
        scalar = T(get_processor());
        return true;
#ifdef PLB_DEBUG // Less Calculations
    } else if (whichScalar==3) {
        scalar = get_E_total();
        return true;
    } else if (whichScalar==4) {
        scalar = T(get_E_inPlane());
        return true;
    } else if (whichScalar==5) {
        scalar = T(get_E_bending());
        return true;
    } else if (whichScalar==6) {
        scalar = T(get_E_area());
        return true;
    } else if (whichScalar==7) {
        scalar = T(get_E_volume());
        return true;
    } else if (whichScalar==8) {
        scalar = T(get_E_repulsive());
        return true;
    } else if (whichScalar==9) {
        scalar = T(get_E_other());
        return true;
#endif
    }
    return Particle3D<T,Descriptor>::getScalar(whichScalar, scalar);
}


template<typename T, template<typename U> class Descriptor>
std::string SurfaceParticle3D<T,Descriptor>::getScalarName(plint whichScalar) const {
    if (whichScalar==0) {
        return "VertexId";
    } else if (whichScalar==1) {
        return "cellId";
    } else if (whichScalar==2) {
        return "processor";
#ifdef PLB_DEBUG // Less Calculations
    } else if (whichScalar==3) {
        return "E_total";
    } else if (whichScalar==4) {
        return "E_inPlane";
    } else if (whichScalar==5) {
        return "E_bending";
    } else if (whichScalar==6) {
        return "E_area";
    } else if (whichScalar==7) {
        return "E_volume";
    } else if (whichScalar==8) {
        return "E_repulsive";
    } else if (whichScalar==9) {
        return "E_other";
#endif
    }
    return "empty";
}


template<typename T, template<typename U> class Descriptor>
plint SurfaceParticle3D<T,Descriptor>::getScalarsNumber() const {
#ifdef PLB_DEBUG // Less Calculations
        return 10;
#else
        return 3;
#endif
}

}  // namespace plb

#endif  // SURFACE_PARTICLE_3D_HH
