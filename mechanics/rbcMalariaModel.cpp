#include "rbcMalariaModel.h"
//TODO Make all inner hemo::Array variables constant as well


RbcMalariaModel::RbcMalariaModel(Config & modelCfg_, HemoCellField & cellField_) : CellMechanics(cellField_, modelCfg_),
                  cellField(cellField_),
                  k_volume( RbcMalariaModel::calculate_kVolume(modelCfg_,*cellField_.meshmetric) ),
                  k_area( RbcMalariaModel::calculate_kArea(modelCfg_,*cellField_.meshmetric) ), 
                  k_link( RbcMalariaModel::calculate_kLink(modelCfg_,*cellField_.meshmetric) ), 
                  k_bend( RbcMalariaModel::calculate_kBend(modelCfg_,*cellField_.meshmetric) ),
                  k_inner_link( RbcMalariaModel::calculate_kInnerLink(modelCfg_,*cellField_.meshmetric) ),
                  eta_m( RbcMalariaModel::calculate_etaM(modelCfg_) )
    {};

void RbcMalariaModel::ParticleMechanics(map<int,vector<HemoCellParticle *>> & particles_per_cell, const map<int,bool> & lpc, size_t ctype) {

  for (const auto & pair : lpc) { //For all cells with at least one lsp in the local domain.
    const int & cid = pair.first;
    vector<HemoCellParticle*> & cell = particles_per_cell[cid];
    if (cell.size() == 0) continue;
    if (cell[0]->sv.celltype != ctype) continue; //only execute on correct particle

    //Calculate Cell Values that need all particles (but do it most efficient
    //tailored to this class)
    T volume = 0.0;
    int triangle_n = 0;
    vector<T> triangle_areas;
    triangle_areas.reserve(cellConstants.triangle_list.size());
    vector<hemo::Array<T,3>> triangle_normals;
    triangle_normals.reserve(cellConstants.triangle_list.size());

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
      volume += (-v210+v120+v201-v021-v102+v012); // the factor of 1/6 moved to after the summation -> saves a few flops
      
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

//Volume force loop
    for (const hemo::Array<plint,3> & triangle : cellConstants.triangle_list) {
      // Scale volume force with local face area
      const hemo::Array<T, 3> local_volume_force = (volume_force*triangle_normals[triangle_n])*(triangle_areas[triangle_n]/cellConstants.area_mean_eq);
      *cell[triangle[0]]->force_volume += local_volume_force;
      *cell[triangle[1]]->force_volume += local_volume_force;
      *cell[triangle[2]]->force_volume += local_volume_force;

      triangle_n++;
    }

//Per-vertex bending force loop
    for (long unsigned int i = 0 ; i < cell.size() ; i++) {
      hemo::Array<T,3> vertexes_sum = {0.,0.,0.};

      for(unsigned int j = 0; j < cellConstants.vertex_n_vertexes[i]; j++) {
        vertexes_sum += cell[cellConstants.vertex_vertexes[i][j]]->sv.position;
      }
      const hemo::Array<T,3> vertexes_middle = vertexes_sum/cellConstants.vertex_n_vertexes[i];

      const hemo::Array<T,3> dev_vect = vertexes_middle - cell[i]->sv.position;
      
      
      // Get the local surface normal
      hemo::Array<T,3> patch_normal = {0.,0.,0.};
      for(unsigned int j = 0; j < cellConstants.vertex_n_vertexes[i]-1; j++) {
        hemo::Array<T,3> triangle_normal = crossProduct(cell[cellConstants.vertex_vertexes[i][j]]->sv.position - cell[i]->sv.position, 
                                                             cell[cellConstants.vertex_vertexes[i][j+1]]->sv.position - cell[i]->sv.position);
        triangle_normal /= norm(triangle_normal);  
        patch_normal += triangle_normal;                                                   
      }
      hemo::Array<T,3> triangle_normal = crossProduct(cell[cellConstants.vertex_vertexes[i][cellConstants.vertex_n_vertexes[i]-1]]->sv.position - cell[i]->sv.position, 
                                                           cell[cellConstants.vertex_vertexes[i][0]]->sv.position - cell[i]->sv.position);
      triangle_normal /= norm(triangle_normal);
      patch_normal += triangle_normal;
 
      patch_normal /= norm(patch_normal);
              
      const T ndev = dot(patch_normal, dev_vect); // distance along patch normal

#ifdef PRECALCULATED_ANGLES
      const T dDev = (ndev - cellConstants.surface_patch_center_dist_eq_list[i] ) / cellConstants.edge_mean_eq; // Non-dimensional
#else 
      const T dDev = ndev / cellConstants.edge_mean_eq; // Non-dimensional
#endif

      //TODO scale bending force
      const hemo::Array<T,3> bending_force = k_bend * ( dDev + dDev/std::fabs(0.055-dDev*dDev)) * patch_normal; // tau_b comes from the angle limit w. eq.lat.tri. assumptiln
      
      //Apply bending force
      *cell[i]->force_bending += bending_force;
    }

    // Per-edge calculations
    int edge_n=0;
    for (const hemo::Array<plint,2> & edge : cellConstants.edge_list) {
      const hemo::Array<T,3> & p0 = cell[edge[0]]->sv.position;
      const hemo::Array<T,3> & p1 = cell[edge[1]]->sv.position;

      // Link force
      const hemo::Array<T,3> edge_vec = p1-p0;
      const T edge_length = norm(edge_vec);
      const hemo::Array<T,3> edge_uv = edge_vec/edge_length;
      const T edge_frac = (edge_length - /*cellConstants.edge_mean_eq*/ cellConstants.edge_length_eq_list[edge_n])
                               / /*cellConstants.edge_mean_eq*/ cellConstants.edge_length_eq_list[edge_n];

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

      const T edge_force_scalar = k_inner_link * 5.0 * edge_frac; // Keep the linear part only for stability  
      
      const hemo::Array<T,3> force = edge_uv*edge_force_scalar;
      *cell[edge[0]]->force_inner_link += force;
      *cell[edge[1]]->force_inner_link -= force;
      inner_edge_n++;
    }
  } 
};

void RbcMalariaModel::statistics() {
    pcout << "(Cell-mechanics model) Malaria model parameters for " << cellField.name << " cellfield" << std::endl; 
    pcout << "\t k_link:   " << k_link << std::endl; 
    pcout << "\t k_area:   " << k_area << std::endl; 
    pcout << "\t k_bend: : " << k_bend << std::endl; 
    pcout << "\t k_inner_link:   " << k_inner_link << std::endl; 
    pcout << "\t k_volume: " << k_volume << std::endl;
    pcout << "\t eta_m:    " << eta_m << std::endl;
};


// Provide methods to calculate and scale to coefficients from here

T RbcMalariaModel::calculate_kInnerLink(Config & cfg, MeshMetrics<T> & meshmetric){
  T kInnerLink = cfg["MaterialModel"]["kInnerLink"].read<T>();
  T persistenceLengthFine = 7.5e-9; // In meters -> this is a biological value
  //TODO: It should scale with the number of surface points!
  T plc = persistenceLengthFine/param::dx;
  kInnerLink *= param::kBT_lbm/plc;
  return kInnerLink;
};

