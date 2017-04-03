#include "highOrderForces.h"

HighOrderForces::HighOrderForces(HemoCellField & cellField_, double k_volume_, double k_area_, double k_inPlane_, double k_bend_) : CellMechanics(),
                  cellConstants(CommonCellConstants::CommonCellConstantsConstructor(cellField_)),
                  cellField(cellField_), k_volume(k_volume_), k_area(k_area_), k_inPlane(k_inPlane_), k_bend(k_bend_)
  { };

void HighOrderForces::ParticleMechanics(map<int,vector<SurfaceParticle3D *>> particles_per_cell, map<int,bool> lpc, pluint ctype) {

  for (const auto & pair : lpc) { //For all cells with at least one lsp in the local domain.
    const int & cid = pair.first;
    vector<SurfaceParticle3D*> & cell = particles_per_cell[cid];
    if (cell[0]->get_celltype() != ctype) continue; //only execute on correct particles

    //Calculate Cell Values that need all particles (but do it most efficient
    //tailered to this class)
    double volume = 0.0;
    int triangle_n = 0;
    vector<double> triangle_areas;
    vector<Array<double,3>> triangle_normals;


    for (const Array<plint,3> & triangle : cellConstants.triangle_list) {
      const Array<double,3> & v0 = cell[triangle[0]]->getPosition();
      const Array<double,3> & v1 = cell[triangle[1]]->getPosition();
      const Array<double,3> & v2 = cell[triangle[2]]->getPosition();
      
      //Volume
      const double v210 = v2[0]*v1[1]*v0[2];
      const double v120 = v1[0]*v2[1]*v0[2];
      const double v201 = v2[0]*v0[1]*v1[2];
      const double v021 = v0[0]*v2[1]*v1[2];
      const double v102 = v1[0]*v0[1]*v2[2];
      const double v012 = v0[0]*v1[1]*v2[2];
      volume += (1.0/6.0)*(-v210+v120+v201-v021-v102+v012);
      
      //Area
      //With herons formula, rewritten for speed
      const double l1 = (v0[0]-v1[0])*(v0[0]-v1[0]) +
                        (v0[1]-v1[1])*(v0[1]-v1[1]) +
                        (v0[2]-v1[2])*(v0[2]-v1[2]);
      const double l2 = (v2[0]-v1[0])*(v2[0]-v1[0]) +
                        (v2[1]-v1[1])*(v2[1]-v1[1]) +
                        (v2[2]-v1[2])*(v2[2]-v1[2]);
      const double l3 = (v0[0]-v2[0])*(v0[0]-v2[0]) +
                        (v0[1]-v2[1])*(v0[1]-v2[1]) +
                        (v0[2]-v2[2])*(v0[2]-v2[2]);
      const double area = sqrt((2*l1*l2 + 2*l2*l3 + 2*l1*l3 - l1*l1 - l2*l2 - l3*l3)/16.0);

      const double areaRatio = (area-cellConstants.triangle_area_eq_list[triangle_n])
                               /cellConstants.triangle_area_eq_list[triangle_n];
      //Area Force per vertex calculation
      //Unit vector pointing from the area
      const Array<double,3> av0 = {v0[0] - (v1[0]-v2[0])*0.5,v0[1] - (v1[1]-v2[1])*0.5,v0[2] - (v1[2]-v2[2])*0.5};
      const Array<double,3> av1 = {v1[0] - (v0[0]-v2[0])*0.5,v1[1] - (v0[1]-v2[1])*0.5,v1[2] - (v0[2]-v2[2])*0.5};
      const Array<double,3> av2 = {v2[0] - (v1[0]-v0[0])*0.5,v2[1] - (v1[1]-v0[1])*0.5,v2[2] - (v1[2]-v0[2])*0.5};
      //length of vector
      const double avl0 = sqrt(av0[0]*av0[0]+av0[1]*av0[1]+av0[2]*av0[2]);
      const double avl1 = sqrt(av1[0]*av1[0]+av1[1]*av1[1]+av1[2]*av1[2]);
      const double avl2 = sqrt(av2[0]*av2[0]+av2[1]*av2[1]+av2[2]*av2[2]);
      //unit vector
      const Array<double,3> avu0 = av0/avl0;
      const Array<double,3> avu1 = av1/avl1;
      const Array<double,3> avu2 = av2/avl2;
      //area force magnitude
      const double afm = -k_area *(areaRatio+areaRatio/(0.04-areaRatio*areaRatio));
      //push back area force
      *cell[triangle[0]]->force_area += afm*avu0;
      *cell[triangle[1]]->force_area += afm*avu1;
      *cell[triangle[2]]->force_area += afm*avu2;


      //Calculate triangle normal while we're busy with this
      Array<double,3> t_normal;
      crossProduct(v1-v0,v2-v0,t_normal); //crossproduct with correct reference point //tODO, swap arg 1 and 2 maybe
      //set normal to unit length
      const double t_normal_l = sqrt(t_normal[0]*t_normal[0]+t_normal[1]*t_normal[1]+t_normal[2]*t_normal[2]);
      t_normal = t_normal/t_normal_l;


      //Store values necessary later
      triangle_areas.push_back(area);
      triangle_normals.push_back(t_normal);

      triangle_n++;
    }

    //Volume
    const double volume_frac = (volume-cellConstants.volume_eq)/cellConstants.volume_eq;
    const double volume_force = k_volume * volume_frac/(0.01-volume_frac*volume_frac);

    triangle_n = 0;

    for (const Array<plint,3> & triangle : cellConstants.triangle_list) {
      //TODO volume force per area
      *cell[triangle[0]]->force_volume += volume_force*1.0/6.0*triangle_normals[triangle_n];
      *cell[triangle[1]]->force_volume += volume_force*1.0/6.0*triangle_normals[triangle_n];
      *cell[triangle[2]]->force_volume += volume_force*1.0/6.0*triangle_normals[triangle_n];

      triangle_n++;
    }


    //Edges
    int edge_n=0;
    for (const Array<plint,2> & edge : cellConstants.edge_list) {
      const Array<double,3> & v0 = cell[edge[0]]->getPosition();
      const Array<double,3> & v1 = cell[edge[1]]->getPosition();

      //In Plane force
      const Array<double,3> edge_v = v1-v0;
      const double edge_length = sqrt(edge_v[0]*edge_v[0]+edge_v[1]*edge_v[1]+edge_v[2]*edge_v[2]);
      const Array<double,3> edge_uv = edge_v/edge_length;
      const double edge_frac = (edge_length-cellConstants.edge_length_eq_list[edge_n])/cellConstants.edge_length_eq_list[edge_n];
      
      if (edge_frac > 0) {
        const double edge_force_scalar = k_inPlane * ( edge_frac + edge_frac/(0.64-edge_frac*edge_frac));   // allows at max. 80% stretch
        const Array<double,3> force = edge_uv*edge_force_scalar;
        *cell[edge[0]]->force_inplane += force;
        *cell[edge[1]]->force_inplane -= force;
      } else{
         // less stiff compression resistance -> let compression be dominated
         // by area conservation force
        const double edge_force_scalar = k_inPlane * edge_frac * edge_frac * edge_frac;
        const Array<double,3> force = edge_uv*edge_force_scalar;
        *cell[edge[0]]->force_inplane += force;
        *cell[edge[1]]->force_inplane -= force;
      }

      //TODO dissapative forces
      //TODO Bending Force
      
      // calculate triangle normals, this should be in a function

      const plint b0 = cellConstants.edge_bending_triangles_list[edge_n][0];
      const plint b1 = cellConstants.edge_bending_triangles_list[edge_n][1];

      const Array<double,3> b00 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,0)]->getPosition();
      const Array<double,3> b01 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,1)]->getPosition();
      const Array<double,3> b02 = particles_per_cell[cid][cellField.meshElement.getVertexId(b0,2)]->getPosition();
      
      const Array<double,3> b10 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,0)]->getPosition();
      const Array<double,3> b11 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,1)]->getPosition();
      const Array<double,3> b12 = particles_per_cell[cid][cellField.meshElement.getVertexId(b1,2)]->getPosition();

      const Array<double,3> V1 = plb::computeTriangleNormal(b00,b01,b02, false);
      const Array<double,3> V2 = plb::computeTriangleNormal(b10,b11,b12, false);

      //TODO this can be precalculated
      Array<double,3> x2 = {0.0,0.0,0.0};
      for (pluint id = 0 ; id < 3 ; id ++ ) {
        const plint kVertex = cellField.meshElement.getVertexId(b0,id);
        if (kVertex != edge[0] && kVertex != edge[1]) {
          x2 = cell[kVertex]->getPosition();
          break;
        }
      }

      //calculate angle
      double angle = angleBetweenVectors(V1, V2);
      const plint sign = dot(x2-v0, V2) >= 0 ? 1 : -1;
      if (sign <= 0) {
        angle = 2 * PI - angle;
      }

      //calculate resulting bending force //todo go to 4 point bending force
      const double angle_frac = cellConstants.edge_angle_eq_list[edge_n] - angle;

      const double force_magnitude = - k_bend * (angle_frac + angle_frac / ( 0.62 - (angle_frac * angle_frac)));

      //TODO bending force differs with area
      const Array<double,3> bending_force = force_magnitude*(V1 + V2)*0.5;
      *cell[edge[0]]->force_bending += bending_force;
      *cell[edge[1]]->force_bending += bending_force;

      edge_n++;
    }


  } 
};

void HighOrderForces::statistics() {
    pcout << "High Order forces for " << cellField.name << " cellfield" << std::endl;
    pcout << "k_volume: " << k_volume << std::endl; 
    pcout << "k_area:   " << k_area << std::endl; 
    pcout << "k_inPlane:" << k_inPlane << std::endl; 
    pcout << "k_bend: : " << k_bend << std::endl; 
};

