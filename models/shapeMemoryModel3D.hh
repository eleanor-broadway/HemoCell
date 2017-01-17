#ifndef SHAPE_MEMORY_MODEL_3D_HH
#define SHAPE_MEMORY_MODEL_3D_HH

#include "shapeMemoryModel3D.h"


namespace plb {

//Looks like some poorly implemented clone function? ...  TODO:FIX
ShapeMemoryModel3D::ShapeMemoryModel3D(ShapeMemoryModel3D const& rhs) :
    ConstitutiveModel<double,DESCRIPTOR>(rhs), meshmetric(meshmetric),
    syncRequirements(rhs.syncRequirements), maxLength(rhs.maxLength), cellRadiusLU(rhs.cellRadiusLU),
    k_rest(rhs.k_rest), k_shear(rhs.k_shear), k_bend(rhs.k_bend), k_stretch(rhs.k_stretch),
    k_inPlane(rhs.k_inPlane), k_elastic(rhs.k_elastic), k_surface(rhs.k_surface), k_volume(rhs.k_volume),
    C_elastic(rhs.C_elastic), eta_m(rhs.eta_m), gamma_T(rhs.gamma_T), gamma_C(rhs.gamma_C),
    eqLength(rhs.eqLength), eqArea(rhs.eqArea), eqAngle(rhs.eqAngle), eqAreaPerTriangle(rhs.eqAreaPerTriangle),
    eqLengthPerEdge(rhs.eqLengthPerEdge), eqAnglePerEdge(rhs.eqAnglePerEdge), eqVolume(rhs.eqVolume),
    eqSurface(rhs.eqSurface), eqTileSpan(rhs.eqTileSpan), persistenceLengthCoarse(rhs.persistenceLengthCoarse),
    eqLengthRatio(rhs.eqLengthRatio), dx(rhs.dx), dt(rhs.dt), dm(rhs.dm), cellNumTriangles(rhs.cellNumTriangles),
    cellNumVertices(rhs.cellNumVertices)
    {}

ShapeMemoryModel3D* ShapeMemoryModel3D::PlateletShapeMemoryModel3D( Config* cfg,
                                        double persistenceLengthFine_, 
                                        double eqLengthRatio_,
                                        double dx_, double dt_, double dm_,
                                        TriangularSurfaceMesh<double> const& meshElement) 
{
  ShapeMemoryModel3D* model = new ShapeMemoryModel3D(cfg,meshElement);
  model->eqLengthRatio = eqLengthRatio_;
  model->dx = dx_;
  model->dt = dt_;
  model->dm = dm_;
  model->k_bend = (*cfg)["cellModel"]["kBend"].read<double>() * 5.0;

  model->Initialize(meshElement);

  return model;
}

ShapeMemoryModel3D* ShapeMemoryModel3D::RBCShapeMemoryModel3D( Config* cfg,
                                        double persistenceLengthFine_, 
                                        double eqLengthRatio_,
                                        double dx_, double dt_, double dm_,
                                        TriangularSurfaceMesh<double> const& meshElement)
{
  ShapeMemoryModel3D* model = new ShapeMemoryModel3D(cfg,meshElement);
  model->eqLengthRatio = eqLengthRatio_;
  model->dx = dx_;
  model->dt = dt_;
  model->dm = dm_;

  model->Initialize(meshElement);

  return model;
}

ShapeMemoryModel3D::ShapeMemoryModel3D(Config* cfg,TriangularSurfaceMesh<double> const& meshElement) 
            : ConstitutiveModel<double,DESCRIPTOR>((*cfg)["cellModel"]["shellDensity"].read<double>()),
            meshmetric(meshElement)
{
  k_rest = (*cfg)["cellModel"]["kRest"].read<double>();
  k_shear = (*cfg)["cellModel"]["kShear"].read<double>();
  k_bend = (*cfg)["cellModel"]["kBend"].read<double>();
  k_stretch =	(*cfg)["cellModel"]["kStretch"].read<double>();
  k_WLC = (*cfg)["cellModel"]["kWLC"].read<double>(); 
  k_elastic =	(*cfg)["cellModel"]["kElastic"].read<double>();
  k_volume = (*cfg)["cellModel"]["kVolume"].read<double>();
  k_surface = (*cfg)["cellModel"]["kSurface"].read<double>();
  eta_m = (*cfg)["cellModel"]["etaM"].read<double>();
}

void ShapeMemoryModel3D::Initialize(TriangularSurfaceMesh<double> const& meshElement) {

    double dNewton = (dm*dx/(dt*dt)) ;
    double kBT = kBT_p / ( dm * dx*dx/(dt*dt) );

    k_WLC *= 1.0;     k_elastic *= 1.0;     k_bend *= 1.0;
    k_volume *= 1.0;     k_surface *= 1.0;     k_shear *= 1.0;
    eta_m /= dNewton*dt/dx;
    //eta_m /= dNewton*dt/dx/dx; 
    k_stretch /= dNewton;    k_rest /= dNewton/dx;
    

    syncRequirements.insert(volumeAndSurfaceReductions);

    cellNumVertices = meshmetric.getNumVertices();
    cellNumTriangles = meshmetric.getNumTriangles();
    eqLength = meshmetric.getMeanLength();
    maxLength = meshmetric.getMaxLength()*eqLengthRatio;
    double eqMeanArea = eqArea = meshmetric.getMeanArea();
//    eqAngle = meshmetric.getMeanAngle();
    eqVolume = meshmetric.getVolume();
    eqSurface = meshmetric.getSurface();
    eqTileSpan = 0.0;

    getCellShapeQuantitiesFromMesh(meshElement, eqAreaPerTriangle, eqLengthPerEdge, eqAnglePerEdge, cellNumTriangles, cellNumVertices);

    persistenceLengthCoarse = persistenceLengthFine/dx * sqrt( (cellNumVertices-2.0) / (23867-2.0)) ;

    eqAngle=0.0;

    typename map<plint,double>::reverse_iterator iter = eqAnglePerEdge.rbegin();
    for (iter = eqAnglePerEdge.rbegin(); iter != eqAnglePerEdge.rend(); ++iter) {
       eqAngle += iter->second;
    }
    eqAngle /= eqAnglePerEdge.size();


    /* Calculate cell Radius */
    cellRadiusLU = meshmetric.getRadius();

    /* Use Mean eqLength for stiffness coefficients of:
     *  Volume, Surface and Shear.
     *  Shear also uses mean eqArea in the calculation.
     *  */
    k_volume *= kBT/pow(eqLength,3);
    k_surface *= kBT/pow(eqLength,2);
    k_shear *= kBT/pow(eqLength,2);
    k_bend *= kBT;
    /* In plane coefficient initialization */
    k_inPlane = k_WLC * kBT /(4.0*persistenceLengthCoarse);

    /* Dissipative term coefficients from FedosovCaswellKarniadakis2010 */
    gamma_T = (eta_m * 12.0/(13.0 * sqrt(3.0)));
    gamma_C = (gamma_T/3.0);

    // pcout << std::endl;
    // pcout << " ============================================= " << std::endl;
    // pcout << " ========  Material model properties ========= " << std::endl;
    // pcout << " ============================================= " << std::endl;
    // pcout << "k_bend: " << k_bend << ",\t eqAngle (degrees): " << eqAngle*180.0/pi << std::endl;
    // pcout << "k_volume: " << k_volume << ",\t eqVolume: " << eqVolume << std::endl;
    // pcout << "k_surface: " << k_surface << ",\t eqSurface: " << eqSurface << std::endl;
    // pcout << "k_shear: " << k_shear << ",\t eqMeanArea: " << eqMeanArea << std::endl;
    // pcout << "eta_m: " << eta_m << ",\t eqLengthRatio: " << eqLengthRatio << std::endl;
    // pcout << "persistenceLengthFine: " << persistenceLengthFine << ",\t persistenceLengthCoarse: " << persistenceLengthCoarse << std::endl;
    // pcout << "gamma_T: " << gamma_T << ", gamma_C: " << gamma_C << std::endl;
    // pcout << "k_rest: " << k_rest << ",\t 0 : " << 0 << std::endl;
    // pcout << "k_stretch: " << k_stretch << ",\t eqTileSpan: " << eqTileSpan << std::endl;
    // pcout << "k_elastic: " << k_elastic << ",\t eqLength: " << eqLength << std::endl;
    // pcout << "* k_bend: " << k_bend/kBT << std::endl;
    // pcout << "* k_volume: " <<  k_volume/(kBT/pow(eqLength,3)) <<  std::endl;
    // pcout << "* k_surface: " << k_surface/(kBT/pow(eqLength,2)) <<  std::endl;
    // pcout << "* k_shear: " << k_shear/(kBT/pow(eqLength,2)) <<  std::endl;
    // pcout << "* eqLength from eqArea: " << sqrt(4*eqMeanArea/sqrt(3.0)) << ",\t eqLength: " << eqLength << std::endl;
    // pcout << "# mu_0 = " << getMembraneShearModulus()*dNewton/dx << std::endl;
    // pcout << "# K = " << getMembraneElasticAreaCompressionModulus()*dNewton/dx << std::endl;
    // pcout << "# YoungsModulus = " << getYoungsModulus()*dNewton/dx << std::endl;
    // pcout << "# Poisson ratio = " << getPoissonRatio() << std::endl;
    // pcout << " ============================================= " << std::endl;

}


plint ShapeMemoryModel3D::getEdgeId(plint iVertex, plint jVertex) {
    iVertex = iVertex % cellNumVertices;
    jVertex = jVertex % cellNumVertices;
    if (iVertex > jVertex){
        return (iVertex*(iVertex - 1))/2 + jVertex;
    } else if (iVertex < jVertex) {
        return (jVertex*(jVertex - 1))/2 + iVertex;
    }
    return -1;
};

void ShapeMemoryModel3D::computeCellForce (Cell3D<double,DESCRIPTOR> * cell) {

#if HEMOCELL_MATERIAL_MODEL == 2
    computeCellForceHighOrder(cell);        // Higher order model
#elif HEMOCELL_MATERIAL_MODEL == 1
    computeCellForceSuresh(cell);           // Suresh/Dao modell improved by Fedosov
#else                                    
    computeCellForceHighOrder(cell);        // Default -> HO model
#endif
};

inline void ShapeMemoryModel3D::computeCellForceSuresh (Cell3D<double,DESCRIPTOR> * cell) {
     /* Some force calculations are according to KrugerThesis, Appendix C */
    double cellVolume = cell->getVolume();
    double cellSurface = cell->getSurface();
    if (not ((cellVolume > 0) and (cellSurface > 0))) {
        cout << "processor: " << cell->getMpiProcessor()
             << ", cellId: " << cell->get_cellId()
             << ", volume: " << cellVolume
             << ", surface: " << cellSurface
             << ", cellNumVertices: " << cellNumVertices
             << endl;
        PLB_PRECONDITION( (cellVolume > 0) and (cellSurface > 0) );
    }
    std::vector<plint> const& triangles = cell->getTriangles();
    std::vector<Array<plint,2> > const& edges = cell->getEdges();
    std::vector<plint > const& vertices = cell->getVertices();
    for (pluint iV = 0; iV < vertices.size(); ++iV) {
        castParticleToICP3D(cell->getParticle3D(vertices[iV]))->resetForces();
    }
    plint iTriangle;
    plint iVertex, jVertex, kVertex, lVertex;

    /* Run through all the edges and calculate:
         x In plane (WLC and repulsive) force
         x Dissipative force
         x Bending force
         o Stretch force
     */
    Array<double,3> force1, force2, force3;

    // Potential only gets computed in debug mode
    double potential;
    for (pluint iE = 0; iE < edges.size(); ++iE) {
        iVertex = edges[iE][0];  jVertex = edges[iE][1];
        plint edgeId = getEdgeId(iVertex, jVertex);
        Array<double,3> const& iX = cell->getVertex(iVertex);
        Array<double,3> const& jX = cell->getVertex(jVertex);
        SurfaceParticle3D<double,DESCRIPTOR>* iParticle = castParticleToICP3D(cell->getParticle3D(iVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* jParticle = castParticleToICP3D(cell->getParticle3D(jVertex));
          /* ------------------------------------*/
         /* In Plane forces (WLC and repulsive) */
        /* ------------------------------------*/
        force1 = computeInPlaneExplicitForce(iX, jX, eqLengthRatio, eqLengthPerEdge[edgeId], k_inPlane, potential);
        iParticle->get_force() += force1;
        jParticle->get_force() -= force1;
#ifdef PLB_DEBUG // Less Calculations
        iParticle->get_E_inPlane() += potential;
        jParticle->get_E_inPlane() += potential;
        iParticle->get_f_wlc() += force1;
        jParticle->get_f_wlc() -= force1;
#endif
          /* ------------------------------------*/
         /*    Dissipative Forces Calculations  */
        /* ------------------------------------*/
        if (gamma_T>0.0) {
            force1 = computeDissipativeForce(iX, jX, iParticle->get_v(), jParticle->get_v(), gamma_T, gamma_C);
            iParticle->get_force() += force1;
            jParticle->get_force() -= force1;
#ifdef PLB_DEBUG // Less Calculations
            iParticle->get_f_viscosity() += force1;
            jParticle->get_f_viscosity() -= force1;
#endif
        }
        /* -------------------------------------------*/
        /*    Stretch (Hookean) Forces Calculations  */
        /* -----------------------------------------*/
        if (k_stretch>0.0) {

        }
          /* ------------------------------------*/
         /*    Bending Forces Calculations      */
        /* ------------------------------------*/
        bool angleFound;

        double edgeAngle = cell->computeSignedAngle(iVertex, jVertex, kVertex, lVertex, angleFound); //edge is iVertex, jVertex
        if (angleFound) {
            Array<double,3> iNormal = cell->computeTriangleNormal(iVertex, jVertex, kVertex);
            Array<double,3> jNormal = cell->computeTriangleNormal(iVertex, jVertex, lVertex);
            double Ai = cell->computeTriangleArea(iVertex, jVertex, kVertex);
            double Aj = cell->computeTriangleArea(iVertex, jVertex, lVertex);
            SurfaceParticle3D<double,DESCRIPTOR>* kParticle = castParticleToICP3D(cell->getParticle3D(kVertex));
            SurfaceParticle3D<double,DESCRIPTOR>* lParticle = castParticleToICP3D(cell->getParticle3D(lVertex));
            Array<double,3> const& kX = cell->getVertex(kVertex);
            Array<double,3> const& lX = cell->getVertex(lVertex);

            /*== Compute bending force for the vertex as part of the main edge ==*/
            #if HEMOCELL_MEMBRANE_BENDING == 1
                Array<double,3> fi, fj;
                fi = computeBendingForce (iX, jX, kX, lX, iNormal, jNormal, eqArea, eqLengthPerEdge[edgeId], eqAnglePerEdge[edgeId], k_bend, fi, fj);
                iParticle->get_force() += fi;
                jParticle->get_force() += fj;
            #else
                // Four-point bending effect
                Array<double,3> fi, fj, fk, fl;
                fi = computeBendingForce (iX, jX, kX, lX, iNormal, jNormal, eqArea, eqLengthPerEdge[edgeId], eqAnglePerEdge[edgeId], k_bend, fi, fj, fk, fl);
                iParticle->get_force() += fi;
                jParticle->get_force() += fj;
                kParticle->get_force() += fk;
                lParticle->get_force() += fl;
            #endif

#ifdef PLB_DEBUG // Less Calculations
            iParticle->get_f_bending() += fi;
            jParticle->get_f_bending() += fj;
            kParticle->get_f_bending() += fk;
            lParticle->get_f_bending() += fl;
            potential = computeBendingPotential (edgeAngle, eqAnglePerEdge[edgeId], k_bend);
            iParticle->get_E_bending() += potential;
            jParticle->get_E_bending() += potential;
            kParticle->get_E_bending() += potential;
            lParticle->get_E_bending() += potential;
#endif
        } else {
            cout << global::mpi().getRank() << " WARNING: angle not found between neighbouring triangles -> surface is not closed and smooth!" << std::endl;
        }
    }

    /* ===================== In case of quasi-rigid object =====================
     *
     * If this is a boundary element (k_rest != 0), get the reference locations
     * of iVertex and calculate and return the force for quasi-rigid objects.
     *          (FengMichaelides2004, J.Comp.Phys. 195(2))
     * // CURRENTLY UNAVAILABLE
     * */

    /* Calculate cell coefficients */
    double volumeCoefficient = k_volume * (cellVolume - eqVolume)/eqVolume;
    double surfaceCoefficient = k_surface * (cellSurface - eqSurface)/eqSurface;
    double eqMeanArea = eqSurface/cellNumTriangles;
    double areaCoefficient = k_shear;//eqMeanArea ;

//    iParticle->get_E_volume() = 0.5*volumeCoefficient*(cellVolume - eqVolume)*1.0/cellNumVertices;
//    iParticle->get_E_area() = 0.5*surfaceCoefficient*(cellSurface - eqSurface)*1.0/cellNumVertices;


    /* Run through all the neighbouring faces of iVertex and calculate:
         x Volume conservation force
         x Surface conservation force
         x Shear force
     */
    Array<double,3> dAdx1, dAdx2, dAdx3, dVdx, tmp(0,0,0);
    std::map<plint, double> trianglesArea;
    std::map<plint, Array<double,3> > trianglesNormal;
    double triangleArea;
    Array<double,3> triangleNormal;

    for (pluint iT = 0; iT < triangles.size(); ++iT) {
        iTriangle = triangles[iT];
        triangleNormal = cell->computeTriangleNormal(iTriangle);
        triangleArea = cell->computeTriangleArea(iTriangle);
        iVertex = cell->getVertexId(iTriangle,0);
        jVertex = cell->getVertexId(iTriangle,1);
        kVertex = cell->getVertexId(iTriangle,2);
        Array<double,3> const& x1 = cell->getVertex(iVertex);
        Array<double,3> const& x2 = cell->getVertex(jVertex);
        Array<double,3> const& x3 = cell->getVertex(kVertex);
        SurfaceParticle3D<double,DESCRIPTOR>* iParticle = castParticleToICP3D(cell->getParticle3D(iVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* jParticle = castParticleToICP3D(cell->getParticle3D(jVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* kParticle = castParticleToICP3D(cell->getParticle3D(kVertex));

        /* Surface conservation forces */
        force1  = computeSurfaceConservationForce(x1, x2, x3, triangleNormal, surfaceCoefficient, dAdx1);
        force2  = computeSurfaceConservationForce(x2, x3, x1, triangleNormal, surfaceCoefficient, dAdx2);
        force3  = computeSurfaceConservationForce(x3, x1, x2, triangleNormal, surfaceCoefficient, dAdx3);
        iParticle->get_force() += force1;
        jParticle->get_force() += force2;
        kParticle->get_force() += force3;

#ifdef PLB_DEBUG // Less Calculations
        iParticle->get_f_surface() += force1;
        jParticle->get_f_surface() += force2;
        kParticle->get_f_surface() += force3;
        potential = 0.5*surfaceCoefficient*(cellSurface - eqSurface)*1.0/cellNumVertices;
        iParticle->get_E_area() += potential;
        jParticle->get_E_area() += potential;
        kParticle->get_E_area() += potential;
#endif
        /* Local area conservation forces */
        force1 = computeLocalAreaConservationForce(dAdx1, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        force2 = computeLocalAreaConservationForce(dAdx2, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        force3 = computeLocalAreaConservationForce(dAdx3, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        iParticle->get_force() += force1;
        jParticle->get_force() += force2;
        kParticle->get_force() += force3;

#ifdef PLB_DEBUG // Less Calculations
        iParticle->get_f_shear() += force1;
        jParticle->get_f_shear() += force2;
        kParticle->get_f_shear() += force3;
        potential = 0.5*areaCoefficient*pow(eqAreaPerTriangle[iTriangle] - triangleArea, 2)/3.0; // 3 vertices on each triangle
        iParticle->get_E_area() += potential;
        jParticle->get_E_area() += potential;
        kParticle->get_E_area() += potential;
#endif

        /* Volume conservation forces */
        force1  = computeVolumeConservationForce(x1, x2, x3, volumeCoefficient);
        force2  = computeVolumeConservationForce(x2, x3, x1, volumeCoefficient);
        force3  = computeVolumeConservationForce(x3, x1, x2, volumeCoefficient);
        iParticle->get_force() += force1;
        jParticle->get_force() += force2;
        kParticle->get_force() += force3;

#ifdef PLB_DEBUG // Less Calculations
        iParticle->get_f_volume() += force1;
        jParticle->get_f_volume() += force2;
        kParticle->get_f_volume() += force3;
        potential = 0.5*volumeCoefficient*(cellVolume - eqVolume)*1.0/cellNumVertices;
        iParticle->get_E_volume() += potential;
        jParticle->get_E_volume() += potential;
        kParticle->get_E_volume() += potential;
#endif

    }

}

inline void ShapeMemoryModel3D::computeCellForceHighOrder (Cell3D<double,DESCRIPTOR> * cell) {
    /* Some force calculations are according to KrugerThesis, Appendix C */
    double cellVolume = cell->getVolume();
    //T cellSurface = cell->getSurface();  // For global surface conservation. Not applicable in HO model
    if (not ((cellVolume > 0) )) {
        cout << "processor: " << cell->getMpiProcessor()
             << ", cellId: " << cell->get_cellId()
             << ", volume: " << cellVolume
             << ", surface: " << cell->getSurface()
             << ", cellNumVertices: " << cellNumVertices
             << endl;
        PLB_PRECONDITION( (cellVolume > 0) );
    }
    std::vector<plint> const& triangles = cell->getTriangles();
    std::vector<Array<plint,2> > const& edges = cell->getEdges();
    std::vector<plint > const& vertices = cell->getVertices();
    for (pluint iV = 0; iV < vertices.size(); ++iV) {
        castParticleToICP3D(cell->getParticle3D(vertices[iV]))->resetForces();
    }

    plint iTriangle;
    plint iVertex, jVertex, kVertex, lVertex;

    Array<double,3> force1, force2, force3;

    for (pluint iE = 0; iE < edges.size(); ++iE) {
        iVertex = edges[iE][0];  jVertex = edges[iE][1];
        plint edgeId = getEdgeId(iVertex, jVertex);
        Array<double,3> const& iX = cell->getVertex(iVertex);
        Array<double,3> const& jX = cell->getVertex(jVertex);
        SurfaceParticle3D<double,DESCRIPTOR>* iParticle = castParticleToICP3D(cell->getParticle3D(iVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* jParticle = castParticleToICP3D(cell->getParticle3D(jVertex));

        /* ------------------------------------*/
        /* In Plane forces (WLC and repulsive) */
        /* ------------------------------------*/
        force1 = computeInPlaneHighOrderForce(iX, jX, eqLengthPerEdge[edgeId], k_inPlane);
        //if(norm(force1) > 1e-3) force1 *= 1e-3/norm(force1);
        iParticle->get_force() += force1;
        jParticle->get_force() -= force1;


        /* ------------------------------------*/
        /*    Dissipative Forces Calculations  */
        /* ------------------------------------*/
        //if (eta_m>0.0) { 
            force1 = computeDissipativeForceHO(iX, jX, iParticle->get_v(), jParticle->get_v(), eta_m);
            iParticle->get_force() += force1;
            jParticle->get_force() -= force1;
        //}

        /* ------------------------------------*/
        /*    Bending Forces Calculations      */
        /* ------------------------------------*/
        bool angleFound;

        double edgeAngle = cell->computeSignedAngle(iVertex, jVertex, kVertex, lVertex, angleFound); //edge is iVertex, jVertex
        if (angleFound) {
            Array<double,3> tri1Norm = cell->computeTriangleNormal(iVertex, jVertex, kVertex);
            Array<double,3> tri2Norm = cell->computeTriangleNormal(iVertex, jVertex, lVertex);
            SurfaceParticle3D<double,DESCRIPTOR>* kParticle = castParticleToICP3D(cell->getParticle3D(kVertex));
            SurfaceParticle3D<double,DESCRIPTOR>* lParticle = castParticleToICP3D(cell->getParticle3D(lVertex));

            //T Ai = cell->computeTriangleArea(iVertex, jVertex, kVertex);
            //T Aj = cell->computeTriangleArea(iVertex, jVertex, lVertex);

            Array<double,3> const& kX = cell->getVertex(kVertex);
            Array<double,3> const& lX = cell->getVertex(lVertex);

            /*== Compute bending force for the vertex as part of the main edge ==*/
            #if HEMOCELL_MEMBRANE_BENDING == 1
/*                Array<T,3> fk, fl;
                fk = computeHighOrderBendingForce (iX, jX, kX, lX, tri1Norm, tri2Norm, eqAnglePerEdge[edgeId], k_bend, fk, fl);
                kParticle->get_force() += fk;
                lParticle->get_force() += fl;
                */
                Array<double,3> fi, fj;
                computeHighOrderBendingForceIn (iX, jX, kX, lX, tri1Norm, tri2Norm, eqAnglePerEdge[edgeId], k_bend, fi, fj);
                iParticle->get_force() += fi;
                jParticle->get_force() += fj;
            #else
                // Four-point bending effect - more stable, less structurally strong, might oscillate on lower lattice resolution
                Array<double,3> fi, fj, fk, fl;
                computeHighOrderBendingForce4p (iX, jX, kX, lX, tri1Norm, tri2Norm, eqAnglePerEdge[edgeId], k_bend, fi, fj, fk, fl);
                iParticle->get_force() += fi;
                jParticle->get_force() += fj;
                kParticle->get_force() += fk;
                lParticle->get_force() += fl;
            #endif

        } else {
            cout << global::mpi().getRank() << " WARNING: angle not found between neighbouring triangles -> surface is not closed and smooth!" << std::endl;
        }
    }

    /* ===================== In case of quasi-rigid object =====================
     *
     * If this is a boundary element (k_rest != 0), get the reference locations
     * of iVertex and calculate and return the force for quasi-rigid objects.
     *          (FengMichaelides2004, J.Comp.Phys. 195(2))
     * // CURRENTLY UNAVAILABLE
     * */

    /* Calculate cell coefficients */
    double dVolume = (cellVolume - eqVolume)/eqVolume;
    double volumeCoefficient = k_volume * dVolume/(0.01 - dVolume*dVolume);
    //T surfaceCoefficient = k_surface * (cellSurface - eqSurface)*1.0/eqSurface;
    //T eqMeanArea = eqSurface/cellNumTriangles;
    double areaCoefficient = k_shear;//eqMeanArea ;

//    iParticle->get_E_volume() = 0.5*volumeCoefficient*(cellVolume - eqVolume)*1.0/cellNumVertices;
//    iParticle->get_E_area() = 0.5*surfaceCoefficient*(cellSurface - eqSurface)*1.0/cellNumVertices;


    /* Run through all the neighbouring faces of iVertex and calculate:
         x Volume conservation force
         x Surface conservation force
         x Shear force
     */
    //Array<T,3> dAdx1, dAdx2, dAdx3, dVdx, tmp(0,0,0);
    //std::map<plint, T> trianglesArea;
    //std::map<plint, Array<T,3> > trianglesNormal;
    double triangleArea;
    Array<double,3> triangleNormal;

    for (pluint iT = 0; iT < triangles.size(); ++iT) {
        iTriangle = triangles[iT];
        triangleNormal = cell->computeTriangleNormal(iTriangle);
        triangleArea = cell->computeTriangleArea(iTriangle);
        iVertex = cell->getVertexId(iTriangle,0);
        jVertex = cell->getVertexId(iTriangle,1);
        kVertex = cell->getVertexId(iTriangle,2);
        Array<double,3> const& x1 = cell->getVertex(iVertex);
        Array<double,3> const& x2 = cell->getVertex(jVertex);
        Array<double,3> const& x3 = cell->getVertex(kVertex);
        SurfaceParticle3D<double,DESCRIPTOR>* iParticle = castParticleToICP3D(cell->getParticle3D(iVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* jParticle = castParticleToICP3D(cell->getParticle3D(jVertex));
        SurfaceParticle3D<double,DESCRIPTOR>* kParticle = castParticleToICP3D(cell->getParticle3D(kVertex));


        /* Local area conservation forces */
        force1 = computeHighOrderLocalAreaConservationForce(x1, x2, x3, triangleNormal, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        force2 = computeHighOrderLocalAreaConservationForce(x2, x3, x1, triangleNormal, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        force3 = computeHighOrderLocalAreaConservationForce(x3, x1, x2, triangleNormal, triangleArea, eqAreaPerTriangle[iTriangle], areaCoefficient);
        //if(norm(force1) > 1e-3) force1 *= 1e-3/norm(force1);
        //if(norm(force2) > 1e-3) force2 *= 1e-3/norm(force2);
        //if(norm(force3) > 1e-3) force3 *= 1e-3/norm(force3);
        iParticle->get_force() += force1;
        jParticle->get_force() += force2;
        kParticle->get_force() += force3;


        /* Volume conservation forces */
        force1  = computeVolumeConservationForce(x1, x2, x3, volumeCoefficient);
        force2  = computeVolumeConservationForce(x2, x3, x1, volumeCoefficient);
        force3  = computeVolumeConservationForce(x3, x1, x2, volumeCoefficient);
        iParticle->get_force() += force1;
        jParticle->get_force() += force2;
        kParticle->get_force() += force3;

#ifdef PLB_DEBUG // Less Calculations
        iParticle->get_f_volume() += force1;
    jParticle->get_f_volume() += force2;
    kParticle->get_f_volume() += force3;
    potential = 0.5*volumeCoefficient*(cellVolume - eqVolume)*1.0/cellNumVertices;
    iParticle->get_E_volume() += potential;
    jParticle->get_E_volume() += potential;
    kParticle->get_E_volume() += potential;
#endif

    }

    /* ======================== Viscosity =======================
        Highly experimental and theoretically wrong as it is. 
        Stay tuned for a better solution!
    */

    // for (pluint iV = 0; iV < vertices.size(); ++iV) {
    //     castParticleToICP3D(cell->getParticle3D(vertices[iV]))->get_force()*=0.98; // 2% damping
    // }
}

//TODO this is the-most-ugly-way of implementing clone, improve
ShapeMemoryModel3D* ShapeMemoryModel3D::clone() const {
    return new ShapeMemoryModel3D(*this);
}


template<typename T>
void getCellShapeQuantitiesFromMesh(TriangularSurfaceMesh<T> const& dynMesh,
                            vector<T> & eqAreaPerTriangle, map<plint,T> & eqLengthPerEdge, map<plint,T> & eqAnglePerEdge,
                            plint cellNumTriangles, plint cellNumPartsPerCell) {
    eqLengthPerEdge.clear(); eqAreaPerTriangle.clear(); eqAnglePerEdge.clear();
    for (plint iVertex = 0; iVertex < cellNumPartsPerCell; ++iVertex) {
        std::vector<plint> neighborVertexIds = dynMesh.getNeighborVertexIds(iVertex);
        for (pluint jV = 0; jV < neighborVertexIds.size(); jV++) {
            plint jVertex = neighborVertexIds[jV];
            if (iVertex > jVertex){
                plint edgeId = (iVertex*(iVertex - 1))/2 + jVertex;
                eqLengthPerEdge[edgeId] = dynMesh.computeEdgeLength(iVertex, jVertex);
                eqAnglePerEdge[edgeId] = calculateSignedAngle(dynMesh, iVertex, jVertex);
            }
        }
    }
    eqAreaPerTriangle.resize(cellNumTriangles);
    for (plint iTriangle = 0; iTriangle < cellNumTriangles; ++iTriangle) {
        eqAreaPerTriangle[iTriangle] = dynMesh.computeTriangleArea(iTriangle);
    }
}

}  // namespace plb

#endif  // SHAPE_MEMORY_MODEL_3D_HH
