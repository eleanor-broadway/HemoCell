/*
   DISCLAIMER OF WARRANTY
   The author makes no warranties, express or implied, that this program
   is free of error, or consistent with any particular standard of
   merchantability, or that it will meet your requirements for any
   particular application. It should not be relied on for solving a problem
   whose incorrect solution could result in injury to a person or loss
   of property. If you do use the program in such a manner, it is at your own
   risk. The author disclaims all liability for direct or consequential
   damages resulting from your use of the program.
*/
//---------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <limits>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "diagonalize.hpp"
/*
    This C++ program tests the formulas for computing the eigenvalues
    and eigenvectorDiags from a real symmetric matrixDiag in 3 dimensions.
    The formulas are taken from the paper on arxiv:
    A Method for Fast Diagonalization of a 2x2 or 3x3 Real Symmetric matrixDiag
    Author: M.J. Kronenburg.
    The program works as follows:
    1. Generate a large set of 3 lambdas and 3 angles
    2. Generate the corresponding matrixDiag
    3. Use the formulas to compute the lambdas and angles from the matrixDiag
    4. Generate from these the new matrixDiag
    5. Compute and output the max. relative difference between the two matrices.
    When the formulas are correct, this max. relative difference should be
    very small, because of rounding errors not larger than about 10^-7.
    Multithreading may be used to speed up the testing computations.
    This program should compile with Borland C++ and Visual C++.
*/

#define sqr( x ) (( x )*( x ))

using namespace std;

#ifndef PI__
#define PI__
const double pi = 4.*atan(1.);
#endif  // PI__

const int outprecision = 17;
const int outwidth = 26;

// implementation of 3-vectorDiag

vectorDiag_exception::vectorDiag_exception( const string & arg )
{ error_description += "vectorDiag error:\n";
  error_description += arg;
}

vectorDiag_exception::~vectorDiag_exception() throw()
{}

const char * vectorDiag_exception::what() const throw()
{ return error_description.c_str();
}

vectorDiag::vectorDiag()
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] = 0.0;
}

vectorDiag::vectorDiag( const vectorDiag & arg )
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] = arg.the_data[ i ];
}

vectorDiag::~vectorDiag()
{}

double & vectorDiag::operator[]( unsigned int pos )
{ if( pos == 0 || pos > 3 )
    throw vectorDiag_exception( "operator[]: index out of bounds." );
  return the_data[ pos-1 ];
}

const double & vectorDiag::operator[]( unsigned int pos ) const
{ if( pos == 0 || pos > 3 )
    throw vectorDiag_exception( "operator[]: index out of bounds." );
  return the_data[ pos-1 ];
}

vectorDiag & vectorDiag::set_zero()
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] = 0.0;
  return *this;
}

vectorDiag & vectorDiag::operator=( const vectorDiag & arg )
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] = arg.the_data[ i ];
  return *this;
}

vectorDiag & vectorDiag::operator+=( const vectorDiag & arg )
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] += arg.the_data[ i ];
  return *this;
}

vectorDiag & vectorDiag::operator-=( const vectorDiag & arg )
{ for( unsigned int i=0; i<3; i++ )
    the_data[ i ] -= arg.the_data[ i ];
  return *this;
}

istream & operator>>( istream & stream, vectorDiag & arg )
{ for( unsigned int i=1; i<=3; i++ )
    stream >> arg[i];
  return stream;
}

ostream & operator<<( ostream & stream, const vectorDiag & arg )
{ for( unsigned int i=1; i<=3; i++ )
    stream << scientific << setw( outwidth ) << setprecision( outprecision )
      << arg[i];
  stream << endl;
  return stream;
}

// implementation of 3x3 matrixDiag

matrixDiag_exception::matrixDiag_exception( const string & arg )
{ error_description += "matrixDiag error:\n";
  error_description += arg;
}

matrixDiag_exception::~matrixDiag_exception() throw()
{}

const char * matrixDiag_exception::what() const throw()
{ return error_description.c_str();
}

matrixDiag::matrixDiag()
{ for( unsigned int i=0; i<9; i++ )
    the_data[ i ] = 0.0;
}

matrixDiag::matrixDiag( const matrixDiag & arg )
{ for( unsigned int i=0; i<9; i++ )
    the_data[ i ] = arg.the_data[ i ];
}

matrixDiag::~matrixDiag()
{}

matrixDiag::array_ref::array_ref( matrixDiag * ref, unsigned int pos )
 : the_ref( ref ), the_pos( pos )
{}

matrixDiag::array_ref::~array_ref()
{}

double & matrixDiag::array_ref::operator[]( unsigned int pos )
{ if( pos == 0 || pos > 3 )
    throw matrixDiag_exception( "operator[]: row index out of bounds." );
  return the_ref->the_data[ 3 * the_pos + pos - 1 ];
}

const double & matrixDiag::array_ref::operator[]( unsigned int pos ) const
{ if( pos == 0 || pos > 3 )
    throw matrixDiag_exception( "operator[]: row index out of bounds." );
  return the_ref->the_data[ 3 * the_pos + pos - 1 ];
}

matrixDiag::array_ref matrixDiag::operator[]( unsigned int pos )
{ if( pos == 0 || pos > 3 )
    throw matrixDiag_exception( "operator[]: column index out of bounds." );
  return array_ref( this, pos - 1 );
}

const matrixDiag::array_ref matrixDiag::operator[]( unsigned int pos ) const
{ if( pos == 0 || pos > 3 )
    throw matrixDiag_exception( "operator[]: column index out of bounds." );
  return array_ref( (matrixDiag *)this, pos - 1 );
}

double matrixDiag::norm() const
{ double sum = 0.0;
  for( int i=0; i<9; i++ )
    sum += sqr( the_data[i] );
  return sqrt( sum );
}

matrixDiag & matrixDiag::set_zero()
{ for( unsigned int i=0; i<9; i++ )
      the_data[ i ] = 0.0;
  return *this;
}

matrixDiag & matrixDiag::transpose()
{ double temp;
  for( unsigned int i=1; i<3; ++i )
      for( unsigned int j=0; j<i; ++j )
      { temp = the_data[ 3 * i + j ];
        the_data[ 3 * i + j ] = the_data[ 3 * j + i ];
        the_data[ 3 * j + i ] = temp;
      };
  return *this;
}

matrixDiag & matrixDiag::set_rotation( unsigned int axis, double phi )
{ if( axis == 0 || axis > 3 )
    throw matrixDiag_exception( "set_rotation: axis number out of bounds." );
  set_zero();
  switch( axis )
  { case 1:
      the_data[0] = 1.0;
      the_data[4] = the_data[8] = cos( phi );
      the_data[5] = the_data[7] = sin( phi );
      the_data[7] = -the_data[7];
      break;
    case 2:
      the_data[4] = 1.0;
      the_data[0] = the_data[8] = cos( phi );
      the_data[2] = the_data[6] = sin( phi );
      the_data[2] = -the_data[2];
      break;
    case 3:
      the_data[8] = 1.0;
      the_data[0] = the_data[4] = cos( phi );
      the_data[1] = the_data[3] = sin( phi );
      the_data[3] = -the_data[3];
      break;
  };
  return *this;
}

matrixDiag & matrixDiag::set_diagonal( const vectorDiag & arg )
{ set_zero();
  the_data[0] = arg.the_data[0];
  the_data[4] = arg.the_data[1];
  the_data[8] = arg.the_data[2];
  return *this;
}

matrixDiag & matrixDiag::operator=( const matrixDiag & arg )
{ for( unsigned int i=0; i<9; i++ )
    the_data[i] = arg.the_data[i];
  return *this;
}

matrixDiag & matrixDiag::operator+=( const matrixDiag & arg )
{ for( unsigned int i=0; i<9; i++ )
    the_data[i] += arg.the_data[i];
  return *this;
}

matrixDiag & matrixDiag::operator-=( const matrixDiag & arg )
{ for( unsigned int i=0; i<9; i++ )
    the_data[i] -= arg.the_data[i];
  return *this;
}

matrixDiag & matrixDiag::operator*=( const matrixDiag & arg )
{ double temp[ 3 ];
  double result;
  double * that_row;
  for( unsigned int j=0; j<3; j++ )
  { for( unsigned int i=0; i<3; i++ )
      temp[ i ] = the_data[ 3 * i + j ];
    for( unsigned int i=0; i<3; i++ )
    { result = 0.0;
      that_row = (double *)arg.the_data + 3 * i;
      for( unsigned int k=0; k<3; k++ )
        result += temp[ k ] * that_row[ k ];
      the_data[ 3 * i + j ] = result;
    };
  };
  return *this;
}

istream & operator>>( istream & stream, matrixDiag & arg )
{ for( unsigned int i=1; i<=3; i++ )
    for( unsigned int j=1; j<=3; j++ )
      stream >> arg[i][j];
  return stream;
}

ostream & operator<<( ostream & stream, matrixDiag & arg )
{ for( unsigned int i=1; i<=3; i++ )
  { for( unsigned int j=1; j<=3; j++ )
      stream << scientific << setw( outwidth ) << setprecision( outprecision )
        << arg[i][j];
    stream << endl;
  };
  return stream;
}

// implementation of computing the lambdas and angles

// dlambda_limit, below which two lambdas are relatively equal
double dlambda_limit = 1.0E-3;
double iszero_limit = 1.0E-20;

// Some globals to record global statistics
int n_all_lambdas_equal = 0, n_two_lambdas_equal = 0,
    n_all_lambdas_different = 0;

// The functions trunc_sqrt and trunc_acos prevent a domain error
// because of rounding off errors:

double trunc_sqrt( double x )
{ return( x <= 0.0 ? 0.0 : sqrt( x ) );
}

double trunc_acos( double x )
{ if( x >= 1.0 )
    return 0.0;
  if( x <= -1.0 )
    return pi;
  return acos( x );
}

double sign( double x )
{ return( x < 0.0 ? -1.0 : 1.0 );
}

double angle( double x, double y )
{ if( x == 0.0 )
    return( y == 0.0 ? 0.0 : 0.5 * pi * sign( y ) );
  return( x < 0.0 ? atan( y / x ) + pi * sign( y )
                  : atan( y / x ) );
}

// Compute the relative difference between two matrices:

double difference_matrixDiag( const matrixDiag & arga, const matrixDiag & argb )
{ matrixDiag dif( arga );
  double norma, normb;
  dif -= argb;
  norma = arga.norm();
  normb = argb.norm();
  norma = 0.5 * ( norma + normb );
  if( norma < iszero_limit )
    return 0.0;
  return dif.norm() / norma;
}

// Generate the symmetric matrixDiag A from lambdas and angles:

void compute_matrixDiag( matrixDiag & res, const vectorDiag & lambdas,
                     const vectorDiag & angles )
{ matrixDiag rot, trans, diag;
  res.set_rotation( 1, angles[1] );
  rot.set_rotation( 2, angles[2] );
  res *= rot;
  rot.set_rotation( 3, angles[3] );
  res *= rot;
  trans = res;
  trans.transpose();
  rot.set_diagonal( lambdas );
  res *= rot;
  res *= trans;
}

// C-interface:
// 3x3 matrixDiag is represented as an array with indices:
// 0 1 2
// 3 4 5
// 6 7 8

// Solve the lambdas from the matrixDiag:

void solve_lambdas( double res[3], double mat[9] )
{ double p, q, b, t, delta;
  b = mat[0] + mat[4] + mat[8];
  t = sqr( mat[1] ) + sqr( mat[2] ) + sqr( mat[5] );
  p = 0.5 * ( sqr( mat[0] - mat[4] ) + sqr( mat[0] - mat[8] )
      + sqr( mat[4] - mat[8] ) );
  p += 3.0 * t;
  q = 18.0 * ( mat[0] * mat[4] * mat[8] + 3.0 * mat[1] * mat[2] * mat[5] );
  q += 2.0 * ( mat[0] * sqr( mat[0] ) + mat[4] * sqr( mat[4] ) +
               mat[8] * sqr( mat[8] ) );
  q += 9.0 * b * t;
  q -= 3.0 * ( mat[0] + mat[4] ) * ( mat[0] + mat[8] ) *
             ( mat[4] + mat[8] );
  q -= 27.0 * ( mat[0] * sqr( mat[5] ) + mat[4] * sqr( mat[2] ) +
                mat[8] * sqr( mat[1] ) );
  if( p < iszero_limit )
    res[0] = res[1] = res[2] = b / 3.0;
  else
  { delta = trunc_acos( 0.5 * q / sqrt( p * sqr( p ) ) );
    p = 2.0 * sqrt( p );
// Changing the order in result yields different angles but identical matrixDiag
    res[0] = ( b + p * cos( delta / 3.0 ) ) / 3.0;
    res[1] = ( b + p * cos( ( delta + 2.0 * pi ) / 3.0 ) ) / 3.0;
    res[2] = ( b + p * cos( ( delta - 2.0 * pi ) / 3.0 ) ) / 3.0;
  };
}

// Determine which type of solution is needed:
//  0: all lambdas equal
//  1: two lambdas equal
//  2: all lambdas different

int solve_type( double lambdas[3] )
{ int i1=0, i2=0, isum = 0;
  double t, lambdasum = 0.0;
  for( int i=0; i<3; i++ )
    lambdasum += sqr( lambdas[i] );
  lambdasum = sqrt( lambdasum );
  for( int i=0; i<2; i++ )
   for( int j=i+1; j<3; j++ )
   { t = fabs( lambdas[i] - lambdas[j] );
     if( lambdasum > iszero_limit )
       t /= lambdasum;
     if( t < dlambda_limit )
     { isum++;
       i1 = i;
       i2 = j;
     };
   };
  if( isum == 0 )
    return 2;
  if( isum >= 2 )
    return 0;
  t = 0.5 * ( lambdas[i1] + lambdas[i2] );
  lambdas[2] = lambdas[3-i1-i2];
  lambdas[0] = lambdas[1] = t;
  return 1;
}

// Solve the angles from the matrixDiag and the solved lambdas:
//  solve_angles_0: all lambdas equal
//  solve_angles_1: two lambdas equal
//  solve_angles_2: all lambdas different

void solve_angles_0( double res[3], double mat[9], double lambdas[3] )
{ res[0] = 0.0;
  res[1] = 0.0;
  res[2] = 0.0;
}

void solve_angles_1( double res[3], double mat[9], double lambdas[3] )
{ double phi1a, phi1b, phi2, absdif, delta = 1.0E10;
  double g12, g21, t1, t2;
  t1 = lambdas[0] - lambdas[2];
  t2 = mat[0] - lambdas[2];
  phi2 = trunc_acos( trunc_sqrt( t2 / t1 ) ); // + pi for symmetry
  g12 = 0.5 * t1 * sin( 2.0 * phi2 );
  g21 = t2;
  t1 = angle( mat[4] - mat[8], -2.0 * mat[5] );
  t2 = sin( t1 );
  t1 = cos( t1 );
  phi1b = 0.5 * angle( g21 * t1, -g21 * t2 );
  t1 = angle( mat[1], -1.0 * mat[2] );
  t2 = sin( t1 );
  t1 = cos( t1 );
  bool big = sqr( mat[4] - mat[8] ) + sqr( 2.0 * mat[5] )
            > sqr( mat[1] ) + sqr( mat[2] );
  for( int i=0; i<2; i++ )
  { phi1a = angle( g12 * t2, g12 * t1 );
    absdif = fabs( phi1a - phi1b );
    if( absdif < delta )
    { delta = absdif;
      res[0] = big ? phi1b : phi1a;
      res[1] = phi2;
    };
    phi2 = -phi2;
    g12 = -g12;
  };
  res[2] = 0.0;
}

void solve_angles_2( double res[3], double mat[9], double lambdas[3] )
{ double phi1a, phi1b, phi2, phi3, v, w, absdif, delta = 1.0E10;
  double g11, g12, g21, g22, t1, t2, t3, t4;
  t1 = lambdas[0] - lambdas[1];
  t2 = lambdas[1] - lambdas[2];
  t3 = lambdas[2] - lambdas[0];
  t4 = mat[0] - lambdas[2];
  v = sqr( mat[1] ) + sqr( mat[2] );
  v += t4 * ( mat[0] + t3 - lambdas[1] );
  v /= t2 * t3;
  if( fabs( v ) < iszero_limit ) w = 1.0;
  else w = ( t4 - t2 * v ) / ( t1 * v );
  phi2 = trunc_acos( trunc_sqrt( v ) ); // + pi for symmetry
  phi3 = trunc_acos( trunc_sqrt( w ) ); // + pi for symmetry
  g11 = 0.5 * t1 * cos( phi2 ) * sin( 2.0 * phi3 );
  g12 = 0.5 * ( t1 * w + t2 ) * sin( 2.0 * phi2 );
  g21 = t1 * ( 1.0 + ( v - 2.0 ) * w ) + t2 * v;
  g22 = t1 * sin( phi2 ) * sin( 2.0 * phi3 );
  t1 = angle( mat[1], -1.0 * mat[2] );
  t3 = angle( mat[4] - mat[8], -2.0 * mat[5] );
  t2 = sin( t1 );
  t1 = cos( t1 );
  t4 = sin( t3 );
  t3 = cos( t3 );
  bool big = sqr( mat[4] - mat[8] ) + sqr( 2.0 * mat[5] )
            > sqr( mat[1] ) + sqr( mat[2] );
  for( int i=0; i<4; i++ )
  { phi1a = angle( g11 * t1 + g12 * t2, -g11 * t2 + g12 * t1 );
    phi1b = 0.5 * angle( g21 * t3 + g22 * t4, -g21 * t4 + g22 * t3 );
    absdif = fabs( phi1a - phi1b );
    if( absdif < delta )
    { delta = absdif;
      res[0] = big ? phi1b : phi1a;
      res[1] = phi2;
      res[2] = phi3;
    };
    phi3 = -phi3;
    g11 = -g11;
    g22 = -g22;
    if( i == 1 )
    { phi2 = -phi2;
      g12 = -g12;
      g22 = -g22;
    };
  };
}

matrixDiag & matrixDiag::sym_eigen( vectorDiag & lambdas, vectorDiag & angles )
{ solve_lambdas( lambdas.the_data, the_data );
  switch( solve_type( lambdas.the_data ) )
  { case 0:
      solve_angles_0( angles.the_data, the_data, lambdas.the_data );
      n_all_lambdas_equal++;
      break;
    case 1:
      solve_angles_1( angles.the_data, the_data, lambdas.the_data );
      n_two_lambdas_equal++;
      break;
    case 2:
      solve_angles_2( angles.the_data, the_data, lambdas.the_data );
      n_all_lambdas_different++;
      break;
  };
  return *this;
}

double compute_maxdif(
  double minlambda1, double maxlambda1, unsigned int nlambda1,
  double minlambda2, double maxlambda2, unsigned int nlambda2,
  double minlambda3, double maxlambda3, unsigned int nlambda3,
  double minangle1, double maxangle1, unsigned int nangle1,
  double minangle2, double maxangle2, unsigned int nangle2,
  double minangle3, double maxangle3, unsigned int nangle3,
  unsigned int & count )
{ vectorDiag lambdas, angles;
  matrixDiag input_matrixDiag, solved_matrixDiag;
  double temp, res = 0.0;
  for( unsigned int i1=0; i1<nlambda1; i1++ )
  for( unsigned int i2=0; i2<nlambda2; i2++ )
  for( unsigned int i3=0; i3<nlambda3; i3++ )
  for( unsigned int j1=0; j1<nangle1; j1++ )
  for( unsigned int j2=0; j2<nangle2; j2++ )
  for( unsigned int j3=0; j3<nangle3; j3++ )
  { lambdas[1] = minlambda1 + ( maxlambda1 - minlambda1 ) * i1 / nlambda1;
    lambdas[2] = minlambda2 + ( maxlambda2 - minlambda2 ) * i2 / nlambda2;
    lambdas[3] = minlambda3 + ( maxlambda3 - minlambda3 ) * i3 / nlambda3;
    angles[1] = minangle1 + ( maxangle1 - minangle1 ) * j1 / nangle1;
    angles[2] = minangle2 + ( maxangle2 - minangle2 ) * j2 / nangle2;
    angles[3] = minangle3 + ( maxangle3 - minangle3 ) * j3 / nangle3;
    compute_matrixDiag( input_matrixDiag, lambdas, angles );
    input_matrixDiag.sym_eigen( lambdas, angles );
    compute_matrixDiag( solved_matrixDiag, lambdas, angles );
    ++count;
    temp = difference_matrixDiag( input_matrixDiag, solved_matrixDiag );
    if( temp > res )
      res = temp;
  };
  return res;
}

/* === Matrix diagonalization. ===
 * inputTensor [3x3 matrix, vector(9)]
 * returnLambdas [vector(3) with the diagonal of the diagonalized matrix]
 * returnAnglesRad [vector(3) with the Euler angles of the rotation matrix]
 * difference [double with the difference between the input and the solved matrix]
*/
template<typename T>
void getLambdasAndAngles(vector<T> inputTensor,
                         vector<T> & returnLambdas, vector<T> & returnAnglesRad,
                         double & difference)
{
    matrixDiag input_matrixDiag, solved_matrixDiag;
    vectorDiag lambdas, angles;

    returnLambdas.clear();
    returnAnglesRad.clear();

    int k=0;
    for( int i=1; i<=3; i++ ) {
        for( int j=1; j<=3; j++ ) {
            input_matrixDiag[i][j] = inputTensor[k++];
        }
    }

    input_matrixDiag.sym_eigen( lambdas, angles );
    compute_matrixDiag( solved_matrixDiag, lambdas, angles );
    for( int i=1; i<=3; i++ ) {
        returnAnglesRad.push_back(angles[i]);
        returnLambdas.push_back(lambdas[i]);
    }
    difference = difference_matrixDiag( input_matrixDiag, solved_matrixDiag );
}


//int main(int argc, char * argv[])
//{  int testtype=0;
//   double minlambda, maxlambda;
//   int nlambda, nangle;
//   matrixDiag input_matrixDiag, solved_matrixDiag;
//   vectorDiag lambdas, angles;
//   unsigned int ncount = 0;
//
//   vector<double> inputTensor(9), lambda, angle;
//   inputTensor[0] = 0.75406;
//   inputTensor[1] = -0.134899;
//   inputTensor[2] = 2.52013e-09;
//   inputTensor[3] = -0.134899;
//   inputTensor[4] = 0.598293;
//   inputTensor[5] = 1.03239e-08;
//   inputTensor[6] = 2.52013e-09;
//   inputTensor[7] = 1.03239e-08;
//   inputTensor[8] = 0.52041;
//
//   getLambdasAndAngles(inputTensor, lambda, angle);
//   cout << "\n==========\n " ;
//
//   for( int i=0; i<3; i++ )
//       cout << lambda[i] << " " ;
//   cout << endl;
//   for( int i=0; i<3; i++ )
//       cout << angle[i]*180/pi << " ";
//   cout << endl;
//
//
//
//   cout << "Give test type:" << endl;
//   cout << "1: one specific real symmetric matrixDiag" << endl;
//   cout << "2: one specific set of lambdas and angles" << endl;
//   cout << "3: range of lambdas and angles" << endl;
//   // cin >> testtype;
//   try
//   { switch( testtype )
//     { case 1:
//         cout << "Give any real symmetric 3x3 matrixDiag:" << endl;
//         cin >> input_matrixDiag;
//         input_matrixDiag.sym_eigen( lambdas, angles );
//         compute_matrixDiag( solved_matrixDiag, lambdas, angles );
//         cout << "Solved matrixDiag:" << endl;
//         cout << solved_matrixDiag;
//         cout << "Lambdas:" << endl;
//         cout << lambdas;
//         for( int i=1; i<=3; i++ )
//           angles[i] *= 180.0 / pi;
//         cout << "Anti-Clockwise Euler Angles (degrees):" << endl;
//         cout << angles;
//         cout << "matrixDiag relative difference: "
//           << scientific << setw( outwidth ) << setprecision( outprecision )
//           << difference_matrixDiag( input_matrixDiag, solved_matrixDiag ) << endl;
//         break;
//       case 2:
//         cout << "Give the three lambdas:" << endl;
//         cin >> lambdas;
//         cout << "Give the three angles in degrees:" << endl;
//         cin >> angles;
//         for( int i=1; i<=3; i++ )
//          angles[i] *= pi / 180.0;
//         compute_matrixDiag( input_matrixDiag, lambdas, angles );
//         cout << "Input matrixDiag:" << endl;
//         cout << input_matrixDiag;
//         input_matrixDiag.sym_eigen( lambdas, angles );
//         compute_matrixDiag( solved_matrixDiag, lambdas, angles );
//         cout << "Solved matrixDiag:" << endl;
//         cout << solved_matrixDiag;
//         cout << "Lambdas: " << endl;
//         cout << lambdas;
//         for( int i=1; i<=3; i++ )
//           angles[i] *= 180.0 / pi;
//         cout << "Anti-Clockwise Euler Angles (degrees): " << endl;
//         cout << angles;
//         cout << "matrixDiag relative difference: "
//           << scientific << setw( outwidth ) << setprecision( outprecision )
//           << difference_matrixDiag( input_matrixDiag, solved_matrixDiag );
//         break;
//       case 3:
//         cout << "Give minimum and maximum lambda:" << endl;
//         cin >> minlambda >> maxlambda;
//         cout << "Give #lambdas:" << endl;
//         cin >> nlambda;
//         cout << "Give #angles:" << endl;
//         cin >> nangle;
//         time_t time1, time2;
//         time1 = time( NULL );
//         double minangle = 0.0, maxangle = 2.0 * pi;
//         double res = compute_maxdif(
//                       minlambda, maxlambda, nlambda,
//                       minlambda, maxlambda, nlambda,
//                       minlambda, maxlambda, nlambda,
//                       minangle, maxangle, nangle,
//                       minangle, maxangle, nangle,
//                       minangle, maxangle, nangle,
//                       ncount );
//	 time2 = time( NULL );
//         cout << "Result: "
//           << scientific << setw( outwidth ) << setprecision( outprecision )
//           << res << endl;
//	 cout << "Time: " << difftime( time2, time1 ) << " secs." << endl;
//         cout << "Statistics:" << endl;
//         cout << "# all lambdas equal    : " << n_all_lambdas_equal << endl;
//         cout << "# two lambdas equal    : " << n_two_lambdas_equal << endl;
//         cout << "# all lambdas different: " << n_all_lambdas_different << endl;
//	 cout << "# total                : " << ncount << endl;
//         break;
//      };
//   } catch( exception & e )
//   { cout << "Error: " << endl;
//     cout << e.what() << endl;
//   };
//	return 0;
//}
