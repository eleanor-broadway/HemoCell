/*
This file is part of the HemoCell library

HemoCell is developed and maintained by the Computational Science Lab 
in the University of Amsterdam. Any questions or remarks regarding this library 
can be sent to: info@hemocell.eu

When using the HemoCell library in scientific work please cite the
corresponding paper: https://doi.org/10.3389/fphys.2017.00563

The HemoCell library is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "pltSimpleModel.h"
#include "logfile.h"
#include "octree.h"
#include "mollerTrumbore.h"

#include "palabos3D.h"
#include "palabos3D.hh"


namespace hemo {
PltSimpleModel::PltSimpleModel(Config & modelCfg_, HemoCellField & cellField_) : CellMechanics(cellField_, modelCfg_), 
                  cellField(cellField_),
                  k_volume( PltSimpleModel::calculate_kVolume(modelCfg_,*cellField_.meshmetric) ),
                  k_area( PltSimpleModel::calculate_kArea(modelCfg_,*cellField_.meshmetric) ), 
                  k_link( PltSimpleModel::calculate_kLink(modelCfg_,*cellField_.meshmetric) ), 
                  k_bend( PltSimpleModel::calculate_kBend(modelCfg_,*cellField_.meshmetric) ),
                  eta_m( PltSimpleModel::calculate_etaM(modelCfg_))
  { };

void PltSimpleModel::ParticleMechanics(map<int,vector<HemoCellParticle *>> & particles_per_cell, const map<int,bool> & lpc, pluint ctype) {
  for (const auto & pair : lpc) { //For all cells with at least one lsp in the local domain.
    const int & cid = pair.first;
    vector<HemoCellParticle*> & cell = particles_per_cell[cid];
    if (cell.size() == 0) continue;
    if (cell[0]->sv.celltype != ctype) continue; //only execute on correct particle

    //Calculate Cell Values that need all particles (but do it efficiently,
    //tailored to this class)
    T volume = 0.0;
    int triangle_n = 0;
    vector<T> triangle_areas;
    vector<hemo::Array<T,3>> triangle_normals; 

    // Per-triangle calculations
    for (const hemo::Array<plint,3> & triangle : cellConstants.triangle_list) {
      const hemo::Array<T,3> & v0 = cell[triangle[0]]->sv.position;
      const hemo::Array<T,3> & v1 = cell[triangle[1]]->sv.position;
      const hemo::Array<T,3> & v2 = cell[triangle[2]]->sv.position;
      
      //Volume
      const T v210 = v2[0]*v1[1]*v0[2];
      const T v120 = v1[0]*v2[1]*v0[2];
      const T v201 = v2[0]*v0[1]*v1[2];
      const T v021 = v0[0]*v2[1]*v1[2];
      const T v102 = v1[0]*v0[1]*v2[2];
      const T v012 = v0[0]*v1[1]*v2[2];
      volume += (-v210+v120+v201-v021-v102+v012);
      
      //Area
      T area; 
      hemo::Array<T,3> t_normal;
      computeTriangleAreaAndUnitNormal(v0, v1, v2, area, t_normal);
      
      const T areaRatio = (area - /*cellConstants.area_mean_eq*/ cellConstants.triangle_area_eq_list[triangle_n])
                               / /*cellConstants.area_mean_eq*/ cellConstants.triangle_area_eq_list[triangle_n];      
       
      //area force magnitude
      const T afm = k_area * (areaRatio+areaRatio/std::fabs(0.09-areaRatio*areaRatio));

      hemo::Array<T,3> centroid;
      centroid[0] = (v0[0]+v1[0]+v2[0])/3.0;
      centroid[1] = (v0[1]+v1[1]+v2[1])/3.0;
      centroid[2] = (v0[2]+v1[2]+v2[2])/3.0;
      hemo::Array<T,3> av0 = centroid - v0;
      hemo::Array<T,3> av1 = centroid - v1;
      hemo::Array<T,3> av2 = centroid - v2;

      *cell[triangle[0]]->force_area += afm*av0;
      *cell[triangle[1]]->force_area += afm*av1;
      *cell[triangle[2]]->force_area += afm*av2;

      //Store values necessary later
      triangle_areas.push_back(area);
      triangle_normals.push_back(t_normal);

      triangle_n++;
    }

    volume *= (1.0/6.0);

    //Volume
    const T volume_frac = (volume-cellConstants.volume_eq)/cellConstants.volume_eq;
    const T volume_force = -k_volume * volume_frac/std::fabs(0.01-volume_frac*volume_frac);

    triangle_n = 0;

    for (const hemo::Array<plint,3> & triangle : cellConstants.triangle_list) {
      //Fixed volume force per area
      const hemo::Array<T, 3> local_volume_force = (volume_force*triangle_normals[triangle_n])*(triangle_areas[triangle_n]/cellConstants.area_mean_eq);
      *cell[triangle[0]]->force_volume += local_volume_force;
      *cell[triangle[1]]->force_volume += local_volume_force;
      *cell[triangle[2]]->force_volume += local_volume_force;

      triangle_n++;
    }


    // Per-edge calculations
    int edge_n=0;
    for (const hemo::Array<plint,2> & edge : cellConstants.edge_list) {
      const hemo::Array<T,3> & v0 = cell[edge[0]]->sv.position;
      const hemo::Array<T,3> & v1 = cell[edge[1]]->sv.position;

      // Link force
      const hemo::Array<T,3> edge_v = v1-v0;
      const T edge_length = sqrt(edge_v[0]*edge_v[0]+edge_v[1]*edge_v[1]+edge_v[2]*edge_v[2]);
      const hemo::Array<T,3> edge_uv = edge_v/edge_length;
      const T edge_frac = (edge_length-cellConstants.edge_length_eq_list[edge_n])/cellConstants.edge_length_eq_list[edge_n];

      const T edge_force_scalar = k_link * ( edge_frac + edge_frac/std::fabs(9.0-edge_frac*edge_frac));   // allows at max. 300% stretch
      
      const hemo::Array<T,3> force = edge_uv*edge_force_scalar;
      *cell[edge[0]]->force_link += force;
      *cell[edge[1]]->force_link -= force;

      // Membrane viscosity of bilipid layer
      // F = eta * (dv/l) * l. 
      const hemo::Array<T,3> rel_vel = cell[edge[1]]->sv.v - cell[edge[0]]->sv.v;
      const hemo::Array<T,3> rel_vel_projection = dot(rel_vel, edge_uv) * edge_uv;
      hemo::Array<T,3> Fvisc_memb = eta_m * rel_vel_projection;

      // Limit membrane viscosity
      const T Fvisc_memb_mag = norm(Fvisc_memb);
      if (Fvisc_memb_mag > FORCE_LIMIT / 4.0) {
        Fvisc_memb *= (FORCE_LIMIT / 4.0) / Fvisc_memb_mag;
      }

      *cell[edge[0]]->force_visc += Fvisc_memb;
      *cell[edge[1]]->force_visc -= Fvisc_memb; 


      const plint b0 = cellConstants.edge_bending_triangles_list[edge_n][0];
      const plint b1 = cellConstants.edge_bending_triangles_list[edge_n][1];

      const hemo::Array<T,3> b00 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,0)]->sv.position;
      const hemo::Array<T,3> b01 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,1)]->sv.position;
      const hemo::Array<T,3> b02 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,2)]->sv.position;
      
      const hemo::Array<T,3> b10 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,0)]->sv.position;
      const hemo::Array<T,3> b11 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,1)]->sv.position;
      const hemo::Array<T,3> b12 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,2)]->sv.position;

      const hemo::Array<T,3> V1 = computeTriangleNormal(b00,b01,b02, false);
      const hemo::Array<T,3> V2 = computeTriangleNormal(b10,b11,b12, false);

      T angle = getAngleBetweenFaces(V1, V2, edge_uv);
      
      //calculate resulting bending force
      const T angle_frac = angle - cellConstants.edge_angle_eq_list[edge_n];

      const T force_magnitude = k_bend * (angle_frac + angle_frac / std::fabs(2.467 - angle_frac * angle_frac) ); // tau_b = pi/2
      
      //TODO Make bending force differ with area!
      const hemo::Array<T,3> bending_force = force_magnitude*(V1 + V2)*0.5;
      *cell[edge[0]]->force_bending += bending_force;
      *cell[edge[1]]->force_bending += bending_force;
      *cell[cellConstants.edge_bending_triangles_outer_points[edge_n][0]]->force_bending -= bending_force;
      *cell[cellConstants.edge_bending_triangles_outer_points[edge_n][1]]->force_bending -= bending_force;

      edge_n++;
    }

    // Per-inner-edge caluclations
    int inner_edge_n=0;
    for (const hemo::Array<plint,2> & edge : cellConstants.inner_edge_list) {
      const hemo::Array<T,3> & v0 = cell[edge[0]]->sv.position;
      const hemo::Array<T,3> & v1 = cell[edge[1]]->sv.position;

      // Link force
      const hemo::Array<T,3> edge_v = v1-v0;
      const T edge_length = sqrt(edge_v[0]*edge_v[0]+edge_v[1]*edge_v[1]+edge_v[2]*edge_v[2]);
      const hemo::Array<T,3> edge_uv = edge_v/edge_length;
      const T edge_frac = (edge_length-cellConstants.inner_edge_length_eq_list[inner_edge_n])/cellConstants.inner_edge_length_eq_list[inner_edge_n];

      const T edge_force_scalar = k_link * 5.0 * edge_frac; // Keep the linear part only for stability  
      
      const hemo::Array<T,3> force = edge_uv*edge_force_scalar;
      *cell[edge[0]]->force_inner_link += force;
      *cell[edge[1]]->force_inner_link -= force;
      inner_edge_n++;
    }

  } 
}

#ifdef SOLIDIFY_MECHANICS
void PltSimpleModel::solidifyMechanics(const std::map<int,std::vector<int>>& ppc,std::vector<HemoCellParticle>& particles,plb::BlockLattice3D<T,DESCRIPTOR> * fluid,plb::BlockLattice3D<T,CEPAC_DESCRIPTOR> * CEPAC, pluint ctype) {
  hemo::Array<T,3> * pos;
  Dot3D const& location_CEPAC = CEPAC->getLocation();
  double threshold = cfg["MaterialModel"]["solidifyThreshold"].read<double>();

  //For all cells
  for (auto & pair : ppc) {
    bool broken = false;
    const std::vector<int> & cell = pair.second;
    //For all particles of cell
    for (const int & particle : cell ) {
      //Skip non-complete and non-platelets
      if (particle == -1) { broken = true; break; }
      if (particles[particle].sv.celltype != ctype) { broken = true; break; }
    }
    if (broken) { continue; }
    
    bool solidify = false;
    // Complete and Correct Type, do solidify mechanics:
    for (const int & particle : cell) {
      //Firstly check if any particle should be solidified
      if (particles[particle].sv.solidify) {
        solidify = true;
          break;
      }
    }

    // If it was tagged last round, solidify it now
    if (solidify) {
      hemo::Array<T,6> bbox;
      hemo::Array<T,3>  * position = &particles[cell[0]].sv.position;
      bbox[0] = bbox[1] = (*position)[0];
      bbox[2] = bbox[3] = (*position)[1];
      bbox[4] = bbox[5] = (*position)[2];

      for (const int pid : cell ) {

        position = &particles[pid].sv.position;

        bbox[0] = bbox[0] > (*position)[0] ? (*position)[0] : bbox[0];
        bbox[1] = bbox[1] < (*position)[0] ? (*position)[0] : bbox[1];
        bbox[2] = bbox[2] > (*position)[1] ? (*position)[1] : bbox[2];
        bbox[3] = bbox[3] < (*position)[1] ? (*position)[1] : bbox[3];
        bbox[4] = bbox[4] > (*position)[2] ? (*position)[2] : bbox[4];
        bbox[5] = bbox[5] < (*position)[2] ? (*position)[2] : bbox[5];
      }
      bbox[1] += 0.1;
      bbox[3] += 0.1;
      bbox[5] += 0.1;
      hemo::OctreeStructCell octCell(1, 1, 30, bbox,
                                  cellConstants.triangle_list,
                                  &particles, cell);
      const double EPSILON = 0.0000001;  // Constant to compare
    
      // Any all negative vector pointing outside is fine as ray
      hemo::Array<double, 3> rayVector = {-100,-100,-100};

      //Adjust bbox to fit local atomic block
      bbox[0] = bbox[0] < fluid->getLocation().x ? fluid->getLocation().x : bbox[0];
      bbox[1] = bbox[1] > fluid->getLocation().x + fluid->getNx()-1 ? fluid->getLocation().x + fluid->getNx()-1: bbox[1];
      bbox[2] = bbox[2] < fluid->getLocation().y ? fluid->getLocation().y : bbox[2];
      bbox[3] = bbox[3] > fluid->getLocation().y + fluid->getNy()-1 ? fluid->getLocation().y + fluid->getNy()-1: bbox[3];
      bbox[4] = bbox[4] < fluid->getLocation().z ? fluid->getLocation().z : bbox[4];
      bbox[5] = bbox[5] > fluid->getLocation().z + fluid->getNz()-1 ? fluid->getLocation().z + fluid->getNz()-1: bbox[5];

      // Create a triple for-loop to go over all lattice points in the bounding box of a cell
      for (int x = (int)bbox[0]; x <= (int)bbox[1]+0.5; x++) { 
        for (int y = (int)bbox[2]; y <= (int)bbox[3]+0.5; y++) {
          for (int z = (int)bbox[4]; z <= (int)bbox[5]+0.5; z++) {
            int crossedCounter = 0; // How many triangles are crossed

            hemo::Array<plint, 3> latticeSite = {x, y, z};
            vector<hemo::Array<plint,3>> triangles_list;
            octCell.findCrossings(latticeSite,triangles_list);

            for (hemo::Array<plint, 3> triangle : triangles_list) {
              // Muller-trumbore intersection algorithm 
              const hemo::Array<double,3> & v0 = particles[cell[triangle[0]]].sv.position;
              const hemo::Array<double,3> & v1 = particles[cell[triangle[1]]].sv.position;
              const hemo::Array<double,3> & v2 = particles[cell[triangle[2]]].sv.position;

              crossedCounter += hemo::MollerTrumbore(v0, v1, v2, rayVector, latticeSite, EPSILON);
            }

            // Count even-odd crossings
            bool inside = crossedCounter % 2 == 0 ? false : true;

            if (inside) {
              int x_l = x-fluid->getLocation().x;
              int y_l = y-fluid->getLocation().y;
              int z_l = z-fluid->getLocation().z;
              if (!fluid->get(x_l,y_l,z_l).getDynamics().isBoundary()) {
                defineDynamics(*fluid,x_l,y_l,z_l,new BounceBack<T,DESCRIPTOR>());
              }
            }
          }
        }
      } 
      for (const int & particle : cell) {
        particles[particle].tag = 1; //tag for removal
      }
    } else {
      //Otherwise, see if we have to tag for solidify
      for (const int & particle : cell) {
        pos = &particles[particle].sv.position;
        int x = pos->operator[](0)-location_CEPAC.x+0.5;
        int y = pos->operator[](1)-location_CEPAC.y+0.5;
        int z = pos->operator[](2)-location_CEPAC.z+0.5;
        if ((x >= 0) && (x < CEPAC->getNx()) &&
            (y >= 0) && (y < CEPAC->getNy()) &&
            (z >= 0) && (z < CEPAC->getNz()) ) {
          if (CEPAC->get(x,y,z).computeDensity() > threshold) {
            particles[particle].sv.solidify = true;
          }
        }
      }
    }
  }
    
  //Check CEPAC density
  
  //If high, solidify all nodes
  
  //Convert Platelet to non-active (change mechanical model to PLT_NO_ACTIVE)
}
#endif

void PltSimpleModel::statistics() {
    hlog << "(Cell-mechanics model) Reduced-model parameters for " << cellField.name << " cellfield" << std::endl;
    hlog << "\t k_link:   " << k_link << std::endl; 
    hlog << "\t k_bend: : " << k_bend << std::endl; 
    hlog << "\t k_volume: " << k_volume << std::endl; 
    hlog << "\t eta_m:      " << eta_m << std::endl;
};
}