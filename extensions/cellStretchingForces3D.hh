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

#ifndef CELL_STRETCHING_FORCES_3D_HH
#define CELL_STRETCHING_FORCES_3D_HH

#include "cellStretchingForces3D.h"


namespace plb {


/* ================================================================================ */
/* ******** ApplyStretchingForce3D *********************************** */
/* ================================================================================ */

template<typename T, template<typename U> class Descriptor>
ApplyStretchingForce3D<T,Descriptor>::ApplyStretchingForce3D (
        std::vector<plint> const& outerLeftTags_, std::vector<plint> const& outerRightTags_,
        Array<T,3> const& stretchingForce_, T cellDensity_,
        std::map<plint, Particle3D<T,Descriptor>*> & tagToParticle3D_)
    :  cellDensity(cellDensity_),
      tagToParticle3D(tagToParticle3D_)
{
        particleTags.push_back(outerLeftTags_);
        particleTags.push_back(outerRightTags_);
        forces.push_back(stretchingForce_ * (-1.0));
        forces.push_back(stretchingForce_);
}

template<typename T, template<typename U> class Descriptor>
ApplyStretchingForce3D<T,Descriptor>::ApplyStretchingForce3D (
        std::vector<std::vector<plint> > particleTags_,
        std::vector<Array<T,3> > forces_, T cellDensity_,
        std::map<plint, Particle3D<T,Descriptor>*> & tagToParticle3D_)
    : particleTags(particleTags_),
      forces(forces_),
      cellDensity(cellDensity_),
      tagToParticle3D(tagToParticle3D_) { }


template<typename T, template<typename U> class Descriptor>
ApplyStretchingForce3D<T,Descriptor>::ApplyStretchingForce3D (
        ApplyStretchingForce3D<T,Descriptor> const& rhs)
    : particleTags(rhs.particleTags),
      forces(rhs.forces),
      cellDensity(rhs.cellDensity),
      tagToParticle3D(rhs.tagToParticle3D)
{ }


template<typename T, template<typename U> class Descriptor>
void ApplyStretchingForce3D<T,Descriptor>::processGenericBlocks (
        Box3D domain, std::vector<AtomicBlock3D*> blocks )
{
    PLB_PRECONDITION( blocks.size()==1 );

    for (pluint var = 0; var < particleTags.size(); ++var) {
        std::vector<plint> & pTags = particleTags[var];
        pluint nParts = pTags.size();
        for (pluint iT = 0; iT < nParts; ++iT) {
            plint tag = pTags[iT];
            if (tagToParticle3D.count(tag) > 0) {
                SurfaceParticle3D<T,Descriptor> * particle = dynamic_cast<SurfaceParticle3D<T,Descriptor>*>(tagToParticle3D[tag]);
                particle->get_force() += forces[var] * (1.0/nParts);
            } else pcout << "ImmerseCellParticle3D not found! Something is wrong here!" << std::endl;
        }
    }

}

template<typename T, template<typename U> class Descriptor>
ApplyStretchingForce3D<T,Descriptor>*
    ApplyStretchingForce3D<T,Descriptor>::clone() const
{
    return new ApplyStretchingForce3D<T,Descriptor>(*this);
}

template<typename T, template<typename U> class Descriptor>
void ApplyStretchingForce3D<T,Descriptor>::getModificationPattern(std::vector<bool>& isWritten) const {
    isWritten[0] = true;  // Particle field.
}

template<typename T, template<typename U> class Descriptor>
BlockDomain::DomainT ApplyStretchingForce3D<T,Descriptor>::appliesTo() const {
    return BlockDomain::bulk;
}

template<typename T, template<typename U> class Descriptor>
void ApplyStretchingForce3D<T,Descriptor>::getTypeOfModification (
        std::vector<modif::ModifT>& modified ) const
{
    modified[0] = modif::allVariables; // Particle field.
}


/* ================================================================================ */
/* ******** MeasureCellStretchDeformation3D *********************************** */
/* ================================================================================ */

template<typename T, template<typename U> class Descriptor>
MeasureCellStretchDeformation3D<T,Descriptor>::MeasureCellStretchDeformation3D (
        std::vector<std::vector<plint>*> const& tags_,
        std::vector<T> * deformation_, std::vector<Array<T,3> > * angles_,
        std::map<plint, Particle3D<T,Descriptor>*> & tagToParticle3D_)
    : tags(tags_),
      deformation(deformation_),
      angles(angles_),
      tagToParticle3D(tagToParticle3D_)
{ }


template<typename T, template<typename U> class Descriptor>
MeasureCellStretchDeformation3D<T,Descriptor>::MeasureCellStretchDeformation3D (
        MeasureCellStretchDeformation3D<T,Descriptor> const& rhs)
    : tags(rhs.tags), deformation(rhs.deformation), angles(rhs.angles), tagToParticle3D(rhs.tagToParticle3D)

{ }


template<typename T, template<typename U> class Descriptor>
void MeasureCellStretchDeformation3D<T,Descriptor>::processGenericBlocks (
        Box3D domain, std::vector<AtomicBlock3D*> blocks )
{
    PLB_PRECONDITION( blocks.size()==1 );

    std::vector< Array<T,3> > meanPositions;
    for (pluint i = 0; i < tags.size(); ++i) { // tags:std::vector<std::vector<plint>*> --  Left,Right,  Front,Back
        std::vector<plint>* vec=tags[i];
        meanPositions.push_back(Array<T,3>(0,0,0));
        for (pluint j = 0; j < vec->size(); ++j) {
            plint tag = (*vec)[j];
            if (tagToParticle3D.count(tag) > 0) {
                SurfaceParticle3D<T,Descriptor>* particle =
                        dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (tagToParticle3D[tag]);

                meanPositions[i] += particle->get_pbcPosition();
            }
        }
        meanPositions[i] /= 1.0 * vec->size();
    }

    deformation->clear();
    for (pluint i = 0; i < tags.size()/2; ++i) {
        deformation->push_back(norm(meanPositions[2*i + 1] - meanPositions[2*i]));
    }
    angles->clear();
    for (pluint i = 0; i < tags.size()/2; ++i) {
        Array<T,3> dr = (meanPositions[2*i + 1] - meanPositions[2*i]) * (1.0/(*deformation)[i]);
        Array<T,3> allAngle( acos(dr[0]), acos(dr[1]), acos(dr[2]) );
        angles->push_back( allAngle );
    }
}

template<typename T, template<typename U> class Descriptor>
MeasureCellStretchDeformation3D<T,Descriptor>*
    MeasureCellStretchDeformation3D<T,Descriptor>::clone() const
{
    return new MeasureCellStretchDeformation3D<T,Descriptor>(*this);
}

template<typename T, template<typename U> class Descriptor>
void MeasureCellStretchDeformation3D<T,Descriptor>::getModificationPattern(std::vector<bool>& isWritten) const {
    isWritten[0] = true;  // Particle field.
}

template<typename T, template<typename U> class Descriptor>
BlockDomain::DomainT MeasureCellStretchDeformation3D<T,Descriptor>::appliesTo() const {
    return BlockDomain::bulk;
}

template<typename T, template<typename U> class Descriptor>
void MeasureCellStretchDeformation3D<T,Descriptor>::getTypeOfModification (
        std::vector<modif::ModifT>& modified ) const
{
    modified[0] = modif::allVariables; // Particle field.
}


/* ================================================================================ */
/* ******** FindTagsOfLateralCellParticles3D *********************************** */
/* ================================================================================ */

template<typename T, template<typename U> class Descriptor>
FindTagsOfLateralCellParticles3D<T,Descriptor>::FindTagsOfLateralCellParticles3D
       (plint numParticlesPerSide_, std::vector<plint> * outerLeftTags_,
               std::vector<plint> * outerRightTags_, pluint direction_=0)
    : numParticlesPerSide(numParticlesPerSide_),
      outerLeftTags(outerLeftTags_),
      outerRightTags(outerRightTags_),
      direction(direction_)
{ }


template<typename T, template<typename U> class Descriptor>
FindTagsOfLateralCellParticles3D<T,Descriptor>::FindTagsOfLateralCellParticles3D (
        FindTagsOfLateralCellParticles3D<T,Descriptor> const& rhs)
    : numParticlesPerSide(rhs.numParticlesPerSide),
      outerLeftTags(rhs.outerLeftTags),
      outerRightTags(rhs.outerRightTags),
      direction(rhs.direction)
{ }


template<typename T, template<typename U> class Descriptor>
void FindTagsOfLateralCellParticles3D<T,Descriptor>::processGenericBlocks (
        Box3D domain, std::vector<AtomicBlock3D*> blocks )
{
            PLB_PRECONDITION( blocks.size()==1 );
            ParticleField3D<T,Descriptor>& particleField =
                *dynamic_cast<ParticleField3D<T,Descriptor>*>(blocks[0]);

            std::vector<Particle3D<T,Descriptor>*> found;
            particleField.findParticles(domain, found);
            if (direction == TFL_DIRECTION_X) {
                std::sort(found.begin(), found.end(), compareParticlesInX_PBC<T,Descriptor>);
            } else if (direction == TFL_DIRECTION_Y) {
                std::sort(found.begin(), found.end(), compareParticlesInY_PBC<T,Descriptor>);
            } else if (direction == TFL_DIRECTION_Z) {
                std::sort(found.begin(), found.end(), compareParticlesInZ_PBC<T,Descriptor>);
            } else if (direction == TFL_DISAGGREGATION_UP or direction == TFL_DISAGGREGATION_UP_HALF or direction == TFL_DISAGGREGATION_LEFT) {
                std::sort(found.begin(), found.end(), compareParticlesInY_PBC<T,Descriptor>);
            }
            plint numParticles = found.size();
            for (plint iP = 0; iP < numParticlesPerSide; ++iP) {
                outerLeftTags->push_back(found[iP]->getTag());
                outerRightTags->push_back(found[numParticles - iP - 1]->getTag());
            }
}

template<typename T, template<typename U> class Descriptor>
FindTagsOfLateralCellParticles3D<T,Descriptor>*
FindTagsOfLateralCellParticles3D<T,Descriptor>::clone() const
{
    return new FindTagsOfLateralCellParticles3D<T,Descriptor>(*this);
}

template<typename T, template<typename U> class Descriptor>
void FindTagsOfLateralCellParticles3D<T,Descriptor>::getModificationPattern(std::vector<bool>& isWritten) const {
    isWritten[0] = true;  // Particle field.
}

template<typename T, template<typename U> class Descriptor>
BlockDomain::DomainT FindTagsOfLateralCellParticles3D<T,Descriptor>::appliesTo() const {
    return BlockDomain::bulk;
}

template<typename T, template<typename U> class Descriptor>
void FindTagsOfLateralCellParticles3D<T,Descriptor>::getTypeOfModification (
        std::vector<modif::ModifT>& modified ) const
{
    modified[0] = modif::nothing; // Particle field.
}

/* ================================================================================ */
/* ******** compareParticlesInX *********************************** */
/* ================================================================================ */
template<typename T, template<typename U> class Descriptor>
bool compareParticlesInX (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iX = iParticle->getPosition()[0];
    T jX = jParticle->getPosition()[0];
    return (iX<jX);
}


template<typename T, template<typename U> class Descriptor>
bool compareParticlesInY (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iY = iParticle->getPosition()[1];
    T jY = jParticle->getPosition()[1];
    return (iY<jY);
}


template<typename T, template<typename U> class Descriptor>
bool compareParticlesInZ (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iZ = iParticle->getPosition()[2];
    T jZ = jParticle->getPosition()[2];
    return (iZ<jZ);
}

/* With periodic boundary conditions */

template<typename T, template<typename U> class Descriptor>
bool compareParticlesInX_PBC (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iX = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (iParticle)->get_pbcPosition()[0];
    T jX = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (jParticle)->get_pbcPosition()[0];
    return (iX<jX);
}


template<typename T, template<typename U> class Descriptor>
bool compareParticlesInY_PBC (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iY = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (iParticle)->get_pbcPosition()[1];
    T jY = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (jParticle)->get_pbcPosition()[1];
    return (iY<jY);
}


template<typename T, template<typename U> class Descriptor>
bool compareParticlesInZ_PBC (Particle3D<T,Descriptor>* iParticle, Particle3D<T,Descriptor>* jParticle) {
    T iZ = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (iParticle)->get_pbcPosition()[2];
    T jZ = dynamic_cast<SurfaceParticle3D<T,Descriptor>*> (jParticle)->get_pbcPosition()[2];
    return (iZ<jZ);
}

}  // namespace plb

#endif  // CELL_STRETCHING_FORCES_3D_HH
