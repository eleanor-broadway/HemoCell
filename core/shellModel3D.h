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

#ifndef SHELL_MODEL_3D_H
#define SHELL_MODEL_3D_H

#include "palabos3D.h"
#include "palabos3D.hh"

namespace plb {

template<typename T>
class ShellModel3D {
public:
    ShellModel3D(T density_);
    virtual ~ShellModel3D() { }
    virtual Array<T,3> computeElasticForce (
           TriangleBoundary3D<T> const& boundary,
            plint iVertex ) =0;
    virtual ShellModel3D<T>* clone() const =0;
    T const& getDensity() const { return density; }
    T& getDensity() { return density; }
private:
    T density;
};

template<typename T>
class SpringModel3D : public ShellModel3D<T>
{
public:
    SpringModel3D(T density_, T k_rest_, T k_stretch_, T k_shear_, T k_bend_);
    virtual Array<T,3> computeElasticForce (
            TriangleBoundary3D<T> const& boundary,
            plint iVertex );
    virtual SpringModel3D<T>* clone() const;
    T const& getRestingStiffness() const { return k_rest; }
    T& getRestingStiffness() { return k_rest; }
    T const& getStretchingStiffness() const { return k_stretch; }
    T& getStretchingStiffness() { return k_stretch; }
    T const& getShearingStiffness() const { return k_shear; }
    T& getShearingStiffness() { return k_shear; }
    T const& getBendingStiffness() const { return k_bend; }
    T& getBendingStiffness() { return k_bend; }
private:
    T k_rest, k_stretch, k_shear, k_bend;
};

template<typename T>
class MembraneBendingModel3D : public ShellModel3D<T>
{
public:
    MembraneBendingModel3D(T density_, T youngModulus_, T poissonRatio_, T thickness_, T k_rest_);
    virtual Array<T,3> computeElasticForce (
            TriangleBoundary3D<T> const& boundary,
            plint iVertex );
    virtual MembraneBendingModel3D<T>* clone() const;
    T const& getYoungModulus() const { return youngModulus; }
    T& getYoungModulus() { return youngModulus; }
    T const& getPoissonRatio() const { return poissonRatio; }
    T& getPoissonRatio() { return poissonRatio; }
    T const& getThickness() const { return thickness; }
    T& getThickness() { return thickness; }
    T const& getRestingStiffness() const { return k_rest; }
    T& getRestingStiffness() { return k_rest; }
private:
    T youngModulus, poissonRatio, thickness, k_rest;
};

namespace shellModelHelper3D {

namespace springModelHelper3D {

template<typename T>
T computePotential(plint iVertex, Array<T,3> const& iPosition,
                   TriangularSurfaceMesh<T> const& dynMesh, 
                   TriangularSurfaceMesh<T> const& eqMesh, 
                   T k_rest, T k_stretch, T k_shear, T k_bend);

template<typename T>
T computeStretchPotential(Array<T,3> const& iPosition, Array<T,3> const& jPosition,
                          Array<T,3> const& iEqPosition, Array<T,3> const& jEqPosition,
                          T k);

template<typename T>
T computeShearPotential(Array<T,3> const& iPosition,
                        Array<T,3> const& jPosition,
                        Array<T,3> const& kPosition,
                        Array<T,3> const& iEqPosition,
                        Array<T,3> const& jEqPosition,
                        Array<T,3> const& kEqPosition,
                        T k);

template<typename T>
T computeBendPotential(Array<T,3> const& iPosition, Array<T,3> const& jPosition,
                       Array<T,3> const& kPosition, Array<T,3> const& lPosition,
                       Array<T,3> const& iEqPosition, Array<T,3> const& jEqPosition,
                       Array<T,3> const& kEqPosition, Array<T,3> const& lEqPosition,
                       T k, T he);

}  // namespace springModelHelper3D

namespace membraneBendingModelHelper3D {

template<typename T>
T computePotential(plint iVertex, Array<T,3> const& iPosition,
                   TriangularSurfaceMesh<T> const& dynMesh,
                   TriangularSurfaceMesh<T> const& eqMesh,
                   T Y, T nu, T h, T k);

template<typename T>
void computeMembraneStrain(Array<T,3> const& iPosition,
                           Array<T,3> const& jPosition,
                           Array<T,3> const& kPosition,
                           Array<T,3> const& iEqPosition,
                           Array<T,3> const& jEqPosition,
                           Array<T,3> const& kEqPosition,
                           Array<Array<T,3>,3>& E);

template<typename T>
void computeBendingStrain(Array<T,3> const* iPositionP, Array<T,3> const* jPositionP,
                          Array<T,3> const* kPositionP,
                          Array<T,3> const* lPositionP, Array<T,3> const* mPositionP,
                          Array<T,3> const* nPositionP,
                          Array<T,3> const* iEqPositionP, Array<T,3> const* jEqPositionP,
                          Array<T,3> const* kEqPositionP,
                          Array<T,3> const* lEqPositionP, Array<T,3> const* mEqPositionP,
                          Array<T,3> const* nEqPositionP,
                          Array<Array<T,3>,3>& E);

template<typename T>
T computeMembranePotentialDensity(Array<Array<T,3>,3> const& E, T Y, T nu, T h);

template<typename T>
T computeBendingPotentialDensity(Array<Array<T,3>,3> const& E, T Y, T nu, T h);

}  // namespace membraneBendingModelHelper3D

template<typename T>
T computeRestPotential(Array<T,3> const& iPosition, Array<T,3> const& iEqPosition, T k);

// template<typename T>
// T computeTriangleArea(Array<T,3> const& iPosition, Array<T,3> const& jPosition,
//                       Array<T,3> const& kPosition); 

}  // namespace shellModelHelper3D

}  // namespace plb

#include "shellModel3D.hh"
#endif  // SHELL_MODEL_3D_H