// Copyright (C) 2012 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef COOLFluiD_Physics_LinearAdv_LinearAdv2DVarSet_hh
#define COOLFluiD_Physics_LinearAdv_LinearAdv2DVarSet_hh

//////////////////////////////////////////////////////////////////////////////

#include "Framework/ConvectiveVarSet.hh"
#include "LinearAdvTerm.hh"
#include "LinearAdv/LinearAdv.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Physics {
namespace LinearAdv {

//////////////////////////////////////////////////////////////////////////////

/// This class represents a LinearAdv physical model 2D for conservative
/// variables
/// @author Andrea Lani
class LinearAdv_API LinearAdv2DVarSet : public Framework::ConvectiveVarSet {

public: // classes

  /// Constructor
  /// @see LinearAdv2D
  LinearAdv2DVarSet(Common::SafePtr<Framework::BaseTerm> term) :
    Framework::ConvectiveVarSet(term),
    _model(term.d_castTo<LinearAdvTerm>())
  {
  }

  /// Default destructor
  virtual ~LinearAdv2DVarSet()
  {
  }

  /// Set up the private data and give the maximum size of states physical
  /// data to store
  virtual void setup()
  {
    Framework::ConvectiveVarSet::setup();
  }

  /// Gets the block separator for this variable set
  CFuint getBlockSeparator() const
  {
    return 1;
  }

  /// Set the jacobians
  virtual void computeJacobians() = 0;

  /// Set the scalar part of the jacobian
  virtual void computeScalarJacobian(const RealVector& normal,
                              RealVector& jacob) = 0;

  /// Set the matrix of the right eigenvectors and the matrix of the eigenvalues
  virtual void computeEigenValuesVectors(RealMatrix& rightEv,
                                     RealMatrix& leftEv,
                                     RealVector& eValues,
                                     const RealVector& normal) = 0;

  /// Get the model
  Common::SafePtr<LinearAdvTerm> getModel() const
  {
    return _model;
  }
  
  /// Set the IDs corresponding to the velocity components in a State
  virtual void setStateVelocityIDs (std::vector<CFuint>& velIDs)
  {
    //velIDs.resize(1); velIDs[XX] = 1;
  }
  
protected:

  /// Compute the convective flux
  virtual void computeFlux(const RealVector& pdata, const RealVector& normals);

  /// Compute the convective flux
  virtual void computeStateFlux(const RealVector& pdata);
 /**
   * Get the maximum eigen value
   */
  virtual CFreal getMaxEigenValue(const RealVector& pdata,
               const RealVector& normal);

  /**
   * Get the maximum absolute eigenvalue
   */
  virtual CFreal getMaxAbsEigenValue(const RealVector& pdata,
            const RealVector& normal);

  /**
   * Set the vector of the eigenValues
   */
  virtual void computeEigenValues(const RealVector& pdata,
               const RealVector& normal, RealVector& result);


private:

  /// acquaintance of the model
  Common::SafePtr<LinearAdvTerm> _model;

}; // end of class LinearAdv2DVarSet

//////////////////////////////////////////////////////////////////////////////

} // namespace LinearAdv
  }
} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_Physics_LinearAdv_LinearAdv2DVarSet_hh
