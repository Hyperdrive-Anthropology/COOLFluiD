// Copyright (C) 2022 KU Leuven CmPA, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef COOLFluiD_FluxReconstructionMethod_DiffRHSFluxReconstructionMFMHD_hh
#define COOLFluiD_FluxReconstructionMethod_DiffRHSFluxReconstructionMFMHD_hh

//////////////////////////////////////////////////////////////////////////////

#include "FluxReconstructionMethod/DiffRHSFluxReconstruction.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {
  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

/**
 * This class represents a command that computes the contribution of the 
 * diffusive tems to the RHS for a Flux Recosntruction scheme for MFMHD
 * 
 * @author Alexander Papen
 * @author Ray Vandenhoeck
 */
class DiffRHSFluxReconstructionMFMHD : public DiffRHSFluxReconstruction {

public: // functions

  /// Constructor
  explicit DiffRHSFluxReconstructionMFMHD(const std::string& name);

  /// Destructor
  virtual ~DiffRHSFluxReconstructionMFMHD() {}

protected: //functions

  /**
   * compute the wave speed updates for this face
   * @pre reconstructFluxPntsStates(), reconstructFaceAvgState(),
   *      setFaceTermData() and set the geometrical data of the face
   */
  void computeWaveSpeedUpdates(std::vector< CFreal >& waveSpeedUpd);
  
  /// prepare the computation of the diffusive flux
  void prepareFluxComputation();
    
}; // class Solve

//////////////////////////////////////////////////////////////////////////////

  }  // namespace FluxReconstructionMethod
}  // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_FluxReconstructionMethod_DiffRHSFluxReconstructionMFMHD_hh

