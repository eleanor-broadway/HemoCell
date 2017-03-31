#ifndef Hemo_CommonCellConstants_H
#define Hemo_CommonCellConstants_H

#include "hemocell_internal.h"
#include "hemoCellField.h"

//Forward declaration:
class HemoCellField;

class CommonCellConstants {
  private:
  CommonCellConstants(HemoCellField & cellField_,
                      vector<Array<plint,3>> triangle_list_,
                      vector<Array<plint,2>> edge_list_,
                      vector<double> edge_length_eq_list_,
                      vector<double> edge_angle_eq_list_,
                      vector<Array<plint,2>> edge_bending_triangles_list_,
                      vector<double> triangle_area_eq_list_,
                      double volume_eq_);
  public: 
  static CommonCellConstants CommonCellConstantsConstructor(HemoCellField &);


  HemoCellField & cellField;
  const vector<Array<plint,3>> triangle_list;
  const vector<Array<plint,2>> edge_list;

  const vector<double> edge_length_eq_list;
  const vector<double> edge_angle_eq_list;
  const vector<Array<plint,2>> edge_bending_triangles_list;
  const vector<double> triangle_area_eq_list;

  const double volume_eq;

};
#endif