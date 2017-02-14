#ifndef HEMOCELL_PARTICLE_FIELD_CPP
#define HEMOCELL_PARTICLE_FIELD_CPP

#include "hemoCellParticleField3D.h"

/* *************** class HemoParticleDataTransfer3D ************************ */

template<typename T, template<typename U> class Descriptor>
HemoParticleDataTransfer3D<T,Descriptor>::HemoParticleDataTransfer3D (
        HemoParticleField3D<T,Descriptor>& particleField_)
    : particleField(particleField_)
{ }

template<typename T, template<typename U> class Descriptor>
plint HemoParticleDataTransfer3D<T,Descriptor>::staticCellSize() const {
    return 0;  // Particle containers have only dynamic data.
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleDataTransfer3D<T,Descriptor>::send (
        Box3D domain, std::vector<char>& buffer, modif::ModifT kind ) const
{
    buffer.clear();
    // Particles, by definition, are dynamic data, and they need to
    //   be reconstructed in any case. Therefore, the send procedure
    //   is run whenever kind is one of the dynamic types.
    if ( (kind==modif::dynamicVariables) ||
         (kind==modif::allVariables) ||
         (kind==modif::dataStructure) )
    {
        std::vector<Particle3D<T,Descriptor>*> foundParticles;
        particleField.findParticles(domain, foundParticles);
        for (pluint iParticle=0; iParticle<foundParticles.size(); ++iParticle) {
            // The serialize function automatically reallocates memory for buffer.
            serialize(*foundParticles[iParticle], buffer);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleDataTransfer3D<T,Descriptor>::receive (
        Box3D domain, std::vector<char> const& buffer, modif::ModifT kind )
{
    PLB_PRECONDITION(contained(domain, particleField.getBoundingBox()));
    // Clear the existing data before introducing the new data.
    particleField.removeParticles(domain);
    // Particles, by definition, are dynamic data, and they need to
    //   be reconstructed in any case. Therefore, the receive procedure
    //   is run whenever kind is one of the dynamic types.
    if ( (kind==modif::dynamicVariables) ||
         (kind==modif::allVariables) ||
         (kind==modif::dataStructure) )
    {
        pluint posInBuffer = 0;
        while (posInBuffer < buffer.size()) {
            // 1. Generate dynamics object, and unserialize dynamic data.
            HierarchicUnserializer unserializer(buffer, posInBuffer);
            Particle3D<T,Descriptor>* newParticle =
                meta::particleRegistration3D<T,Descriptor>().generate(unserializer);
            posInBuffer = unserializer.getCurrentPos();
            particleField.addParticle(domain, newParticle);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleDataTransfer3D<T,Descriptor>::receive (
        Box3D domain, std::vector<char> const& buffer, modif::ModifT kind, Dot3D absoluteOffset )
{
    if (absoluteOffset.x == 0 && absoluteOffset.y == 0 && absoluteOffset.z == 0) {
        receive(domain, buffer, kind);
        return;
    }
    PLB_PRECONDITION(contained(domain, particleField.getBoundingBox()));
    Array<T,3> realAbsoluteOffset((T)absoluteOffset.x, (T)absoluteOffset.y, (T)absoluteOffset.z);
    // Clear the existing data before introducing the new data.
    particleField.removeParticles(domain);
    // Particles, by definition, are dynamic data, and they need to
    //   be reconstructed in any case. Therefore, the receive procedure
    //   is run whenever kind is one of the dynamic types.
    if ( (kind==modif::dynamicVariables) ||
         (kind==modif::allVariables) ||
         (kind==modif::dataStructure) )
    {
        pluint posInBuffer = 0;
        while (posInBuffer < buffer.size()) {
            // 1. Generate dynamics object, and unserialize dynamic data.
            HierarchicUnserializer unserializer(buffer, posInBuffer);
            Particle3D<T,Descriptor>* newParticle =
                meta::particleRegistration3D<T,Descriptor>().generate(unserializer);
            posInBuffer = unserializer.getCurrentPos();
            newParticle -> getPosition() += realAbsoluteOffset;

            particleField.addParticle(domain, newParticle);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleDataTransfer3D<T,Descriptor>::attribute (
        Box3D toDomain, plint deltaX, plint deltaY, plint deltaZ,
        AtomicBlock3D const& from, modif::ModifT kind )
{
    Box3D fromDomain(toDomain.shift(deltaX,deltaY,deltaZ));
    std::vector<char> buffer;
    HemoParticleField3D<T,Descriptor> const& fromParticleField =
        dynamic_cast<HemoParticleField3D<T,Descriptor>const &>(from);
    fromParticleField.getDataTransfer().send(fromDomain, buffer, kind);
    receive(toDomain, buffer, kind);
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleDataTransfer3D<T,Descriptor>::attribute (
        Box3D toDomain, plint deltaX, plint deltaY, plint deltaZ,
        AtomicBlock3D const& from, modif::ModifT kind, Dot3D absoluteOffset )
{
    Box3D fromDomain(toDomain.shift(deltaX,deltaY,deltaZ));
    std::vector<char> buffer;
    HemoParticleField3D<T,Descriptor> const& fromParticleField =
        dynamic_cast<HemoParticleField3D<T,Descriptor>const &>(from);
    fromParticleField.getDataTransfer().send(fromDomain, buffer, kind);
    receive(toDomain, buffer, kind, absoluteOffset);
}


/* *************** class HemoParticleField3D ********************** */

template<typename T, template<typename U> class Descriptor>
HemoParticleField3D<T,Descriptor>::HemoParticleField3D(plint nx, plint ny, plint nz)
    : ParticleField3D<T,Descriptor>(nx,ny,nz),
      dataTransfer(*this)
{ }

template<typename T, template<typename U> class Descriptor>
HemoParticleField3D<T,Descriptor>::~HemoParticleField3D()
{
    for (pluint i=0; i<particles.size(); ++i) {
        delete particles[i];
    }
}

template<typename T, template<typename U> class Descriptor>
HemoParticleField3D<T,Descriptor>::HemoParticleField3D(HemoParticleField3D const& rhs)
    : ParticleField3D<T,Descriptor>(rhs),
      dataTransfer(*this)
{
    for (pluint i=0; i<rhs.particles.size(); ++i) {
        particles.push_back(rhs.particles[i]->clone());
    }
}

template<typename T, template<typename U> class Descriptor>
HemoParticleField3D<T,Descriptor>& 
    HemoParticleField3D<T,Descriptor>::operator=(HemoParticleField3D<T,Descriptor> const& rhs)
{
    HemoParticleField3D<T,Descriptor>(rhs).swap(*this);
    return *this;
}

template<typename T, template<typename U> class Descriptor>
HemoParticleField3D<T,Descriptor>*
    HemoParticleField3D<T,Descriptor>::clone() const
{
    return new HemoParticleField3D<T,Descriptor>(*this);
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::swap(HemoParticleField3D<T,Descriptor>& rhs) {
    ParticleField3D<T,Descriptor>::swap(rhs);
    particles.swap(rhs.particles);
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::addParticle(Box3D domain, Particle3D<T,Descriptor>* particle) {
    Box3D finalDomain;
    Array<T,3> pos; 
    pos = particle->getPosition();
    if( intersect(domain, this->getBoundingBox(), finalDomain) &&
        this->isContained(pos[0],pos[1],pos[2], finalDomain) )
    {
        particles.push_back(particle);
    }
    else {
        delete particle;
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::removeParticles(Box3D domain) {
    std::vector<Particle3D<T,Descriptor>*> remainingParticles;
    Box3D finalDomain;
    Array<T,3> pos; 
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
        for (pluint i=0; i<particles.size(); ++i) {
            pos = particles[i]->getPosition();
            if (this->isContained(pos[0],pos[1],pos[2],finalDomain)) {
                delete particles[i];
            }
            else {
                remainingParticles.push_back(particles[i]);
            }
        }
    }
    remainingParticles.swap(particles);
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::removeParticles(Box3D domain, plint tag) {
    std::vector<Particle3D<T,Descriptor>*> remainingParticles;
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
        Array<T,3> pos; 
        for (pluint i=0; i<particles.size(); ++i) {
            pos = particles[i]->getPosition();
            if (this->isContained(pos[0],pos[1],pos[2],finalDomain) &&
                particles[i]->getTag() == tag )
            {
                delete particles[i];
            }
            else {
                remainingParticles.push_back(particles[i]);
            }
        }
    }
    remainingParticles.swap(particles);
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::findParticles (
        Box3D domain, std::vector<Particle3D<T,Descriptor>*>& found ) 
{
    found.clear();
    PLB_ASSERT( contained(domain, this->getBoundingBox()) );
    Array<T,3> pos; 
    for (pluint i=0; i<particles.size(); ++i) {
        pos = particles[i]->getPosition();
        if (this->isContained(pos[0],pos[1],pos[2],domain)) {
            found.push_back(particles[i]);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::findParticles (
        Box3D domain, std::vector<Particle3D<T,Descriptor> const*>& found ) const
{
    found.clear();
    PLB_ASSERT( contained(domain, this->getBoundingBox()) );
    Array<T,3> pos; 
        
    for (pluint i=0; i<particles.size(); ++i) {
        pos = particles[i]->getPosition();
        if (this->isContained(pos[0],pos[1],pos[2],domain)) {
            found.push_back(particles[i]);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::velocityToParticleCoupling (
        Box3D domain, TensorField3D<T,3>& velocityField, T scaling )
{
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
      Array<T,3> pos;
        for (pluint i=0; i<particles.size(); ++i) {
	  pos = particles[i]->getPosition();
	  if (this->isContained(pos[0],pos[1],pos[2],finalDomain)) {
                particles[i]->velocityToParticle(velocityField, scaling);
            }
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::velocityToParticleCoupling (
        Box3D domain, NTensorField3D<T>& velocityField, T scaling )
{
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
        for (pluint i=0; i<particles.size(); ++i) {
            if (this->isContained(particles[i]->getPosition(),finalDomain)) {
                particles[i]->velocityToParticle(velocityField, scaling);
            }
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::rhoBarJtoParticleCoupling (
        Box3D domain, NTensorField3D<T>& rhoBarJfield, bool velIsJ, T scaling )
{
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
      Array<T,3> pos;
        for (pluint i=0; i<particles.size(); ++i) {
	  pos = particles[i]->getPosition();
	  if (this->isContained(pos[0],pos[1],pos[2],finalDomain)) {
                particles[i]->rhoBarJtoParticle(rhoBarJfield, velIsJ, scaling);
            }
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::fluidToParticleCoupling (
        Box3D domain, BlockLattice3D<T,Descriptor>& lattice, T scaling )
{
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
        Array<T,3> pos;
        for (pluint i=0; i<particles.size(); ++i) {
            pos = particles[i]->getPosition();
            if (this->isContained(pos[0],pos[1],pos[2],finalDomain)) {
                particles[i]->fluidToParticle(lattice, scaling);
            }
        }
    }
}

template<typename T, template<typename U> class Descriptor>
void HemoParticleField3D<T,Descriptor>::advanceParticles(Box3D domain, T cutOffValue) {
    std::vector<Particle3D<T,Descriptor>*> remainingParticles;
    Box3D finalDomain;
    if( intersect(domain, this->getBoundingBox(), finalDomain) )
    {
        Array<T,3> pos; 
        for (pluint i=0; i<particles.size(); ++i) {
            Particle3D<T,Descriptor>* particle = particles[i];
            pos = particle->getPosition();
            if (this->isContained(pos[0],pos[1],pos[2],finalDomain)) {
                Array<T,3> oldPos( particle->getPosition() );
                particle->advance();
                pos = particle->getPosition();
                if ( (cutOffValue>=T() && normSqr(oldPos-particle->getPosition())<cutOffValue) ||
                     (!this->isContained(pos[0],pos[1],pos[2],this->getBoundingBox()))  )
                {
                    delete particle;
                }
                else {
                    remainingParticles.push_back(particle);
                }
            }
        }
    }
    particles.swap(remainingParticles);
}

template<typename T, template<typename U> class Descriptor>
HemoParticleDataTransfer3D<T,Descriptor>& HemoParticleField3D<T,Descriptor>::getDataTransfer() {
    return dataTransfer;
}

template<typename T, template<typename U> class Descriptor>
HemoParticleDataTransfer3D<T,Descriptor> const& HemoParticleField3D<T,Descriptor>::getDataTransfer() const {
    return dataTransfer;
}

template<typename T, template<typename U> class Descriptor>
std::string HemoParticleField3D<T,Descriptor>::getBlockName() {
    return std::string("HemoParticleField3D");
}

template<typename T, template<typename U> class Descriptor>
std::string HemoParticleField3D<T,Descriptor>::basicType() {
    return std::string(NativeType<T>::getName());
}

template<typename T, template<typename U> class Descriptor>
std::string HemoParticleField3D<T,Descriptor>::descriptorType() {
    return std::string(Descriptor<T>::name);
}


#endif