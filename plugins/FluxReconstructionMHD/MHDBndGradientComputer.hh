// Copyright (C) 2012 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef COOLFluiD_FluxReconstructionMethod_MHDBndGradientComputer_hh
#define COOLFluiD_FluxReconstructionMethod_MHDBndGradientComputer_hh

//////////////////////////////////////////////////////////////////////////////

#include "FluxReconstructionMethod/ConvBndCorrectionsRHSFluxReconstruction.hh"
#include "MHD/MHDProjectionDiffVarSet.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {
  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

/**
 * Daughterclass of ConvBndCorrectionsRHSFluxReconstruction, needed to 
 * calculate the bnd gradients for MHD
 * 
 * @author Ray Vandenhoeck
 */
class MHDBndGradientComputer : public ConvBndCorrectionsRHSFluxReconstruction {

public: // functions

  /// Constructor
  explicit MHDBndGradientComputer(const std::string& name);

  /// Destructor
  virtual ~MHDBndGradientComputer() {}
  
  /**
   * Set up private data and data of the aggregated classes
   * in this command before processing phase
   */
  virtual void setup();

protected: //functions
  
  /**
   * Compute the correction part of the corrected gradient for the bnd face
   */
  virtual void computeGradientBndFaceCorrections();
  
protected: //data
  
  /// diffusive variable set
  Common::SafePtr< Physics::MHD::MHDProjectionDiffVarSet > m_diffusiveVarSet;
  
  /// Vector transformer from update to solution variables
  Common::SafePtr<Framework::VarSetTransformer> m_updateToSolutionVecTrans;
  
  /// matrix to store the state terms needed for the gradients (p, u, v, T)
  RealMatrix m_tempGradTerm;
  
  /// matrix to store the ghost state terms needed for the gradients (p, u, v, T)
  RealMatrix m_tempGradTermGhost;
  
  /// element states within an element in the correct format
  std::vector< RealVector* > m_tempStates;
  
  /// element ghost states in the correct format
  std::vector< RealVector* > m_tempStatesGhost;
    
}; // class Solve

//////////////////////////////////////////////////////////////////////////////

  }  // namespace FluxReconstructionMethod
}  // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_FluxReconstructionMethod_MHDBndGradientComputer_hh

