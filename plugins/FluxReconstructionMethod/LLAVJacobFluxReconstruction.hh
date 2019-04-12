// Copyright (C) 2012 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef COOLFluiD_FluxReconstructionMethod_LLAVJacobFluxReconstruction_hh
#define COOLFluiD_FluxReconstructionMethod_LLAVJacobFluxReconstruction_hh

//////////////////////////////////////////////////////////////////////////////

#include "Framework/BaseMethodStrategyProvider.hh"

#include "FluxReconstructionMethod/FluxReconstructionSolverData.hh"

#include "FluxReconstructionMethod/DiffRHSJacobFluxReconstruction.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {
  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

/// Command to add Localized Laplacian Artificial Viscosity near discontinuities for implicit schemes
/// @author Ray Vandenhoeck
    
class LLAVJacobFluxReconstruction : public DiffRHSJacobFluxReconstruction {

public: // functions

  /// Constructor
  explicit LLAVJacobFluxReconstruction(const std::string& name);

  /// Destructor
  virtual ~LLAVJacobFluxReconstruction() {}

  /// Execute processing actions
  void execute();
  
  /**
   * Defines the Config Option's of this class
   * @param options a OptionList where to add the Option's
   */
  static void defineConfigOptions(Config::OptionList& options);
  
  /**
   * Configures the command.
   */
  virtual void configure ( Config::ConfigArgs& args );
  
  /**
   * Set up private data and data of the aggregated classes
   * in this command before processing phase
   */
  virtual void setup();

  /**
   * Unsetup private data
   */
  virtual void unsetup();
  
  /// Returns the DataSocket's that this command provides as sources
  /// @return a vector of SafePtr with the DataSockets
  virtual std::vector< Common::SafePtr< Framework::BaseDataSocketSource > >
    providesSockets();
    
protected: //functions
  
  /**
   * compute the wave speed updates for this face
   * @pre reconstructFluxPntsStates(), reconstructFaceAvgState(),
   *      setFaceTermData() and set the geometrical data of the face
   */
  virtual void computeWaveSpeedUpdates(std::vector< CFreal >& waveSpeedUpd);

  /// compute the interface flux
  void computeInterfaceFlxCorrection();
  
  /// compute the divergence of the discontinuous flux (-divFD+divhFD)
  void computeDivDiscontFlx(std::vector< RealVector >& residuals);
  
  /// compute the divergence of the discontinuous flux (-divFD+divhFD) of a neighbor cell
  void computeDivDiscontFlxNeighb(RealVector& residuals, const CFuint side);
  
  /**
   * Set the data for the current face necessary to calculate FI
   */
  void setFaceData(CFuint faceID);
  
  /**
   * Set the data for the current cell necessary to calculate the residual update
   */
  void setCellData(const CFuint side);
  
  /**
   * Compute the projected states on order P-1
   */
  void computeProjStates(std::vector< RealVector >& projStates);
  
  /**
   * Compute the projected states on order P-1 on a side of the current face
   */
  void computeProjStates(std::vector< RealVector >& projStates, const CFuint side);
  
  /**
   * Compute the artificial viscosity
   */
  virtual void computeEpsilon();
  
  /**
   * Compute the reference artificial viscosity
   */
  virtual void computeEpsilon0();
  
  /**
   * Compute the reference artificial viscosity
   */
  virtual void computeEpsilon0(const CFuint side);
  
  /**
   * Compute the smoothness indicator
   */
  virtual void computeSmoothness();
  
  /**
   * Compute the smoothness indicator
   */
  virtual void computeSmoothness(const CFuint side);
  
  /**
   * Store the computed artificial viscosity
   */
  virtual void storeEpsilon();
  
  /**
   * compute the artificial diffusive flux
   */
  virtual void computeFlux(const RealVector& values, const std::vector< RealVector* >& gradients, const RealVector& normal, const CFreal& radius, RealVector& flux);
  
  /**
   * command to compute the peclet number based on user input
   */
  virtual CFreal computePeclet(); 
  
  /**
   * recompute the cell gradients from the current cell solutions,
   * after perturbation
   */
  virtual void computePerturbedGradientsAnalytical(const CFuint side);

protected: //data
  
  /// update variable set
  Common::SafePtr< Framework::ConvectiveVarSet > m_updateVarSet;
  
  /// order of the FR method
  CFuint m_order;
  
  /// transformation matrices to order P-1
  RealMatrix m_transformationMatrix;
  
  /// states projected on P-1
  std::vector< RealVector > m_statesPMinOne;
  
  /// artificial Viscosity
  CFreal m_epsilon;
  
  /// artificial Viscosity in the sol pnts
  std::vector< std::vector< CFreal > > m_solEpsilons;
  
  /// artificial Viscosity
  std::vector< std::vector< CFreal > > m_epsilonLR;
  
  /// reference artificial Viscosity
  CFreal m_epsilon0;
  
  /// reference smoothness
  CFreal m_s0;
  
  /// smoothness
  CFreal m_s;
  
  /// controlling parameter kappa
  CFreal m_kappa;
  
  /// peclet number
  CFreal m_peclet;
  
  /// damping coefficient of recalculation of epsilon
  CFreal m_dampingCoeff;
  
  /// average artificial viscosities in the nodes
  RealVector m_nodeEpsilons;
  
  /// number of neighbors for each node
  RealVector m_nbNodeNeighbors;
  
  /// average artificial viscosities in the elements
  RealVector m_cellEpsilons;
  
  /// vector containing pointers to the nodes in a cell
  std::vector< Framework::Node*  >* m_cellNodes;
  
  /// number of corner nodes for current element type
  CFuint m_nbrCornerNodes;
  
  /// vector containing pointers to the nodes in a face
  std::vector< Framework::Node*  >* m_faceNodes;
  
  /// flag telling whether to compute the number of node neighbors
  bool m_flagComputeNbNghb;
  
  /// polynomial coefficients for reconstruction of the artificial viscosity at the flx pnts
  std::vector< std::vector< CFreal > > m_nodePolyValsAtFlxPnts;
  
  /// polynomial coefficients for reconstruction of the artificial viscosity at the sol pnts
  std::vector< std::vector< CFreal > > m_nodePolyValsAtSolPnts;
  
  /// cell node connectivity table
  Common::SafePtr< Framework::MeshData::ConnTable > m_cellNodesConn;
  
  /// current element index
  CFuint m_elemIdx;
  
  /// variable for faces
  const std::vector< Framework::GeometricEntity* >* m_facesCell;
  
  /// bool telling whether the jacobian is being computed
  bool m_jacob;
  
  /// residual after which the limiter is frozen
  CFreal m_freezeLimiterRes;
  
  /// iteration after which the limiter is frozen
  CFuint m_freezeLimiterIter;
  
  /// boolean telling whether to use max artificial viscosity wrt previous iteration
  bool m_useMax;
  
  /// total artificial viscosity added
  CFreal m_totalEps;
  
  /// total artificial viscosity added over all CPUs
  CFreal m_totalEpsGlobal;
  
  /// maximum smoothness in domain
  CFreal m_Smax;
  
  /// maximum smoothness in domain over all CPUs
  CFreal m_SmaxGlobal;
  
  /// boolean telling whether to add the contribution of the artificial flux to the update coefficients
  bool m_addUpdCoeff;
  
  /// value at which point positivity preservation is added
  CFreal m_minValue;
  
  /// index of the monitored variable for positivity preservation
  CFuint m_monitoredVar;
  
  /// maximum factor applied to viscosity for positivity preservation
  CFreal m_viscFactor;
  
  /// subcell resolution
  CFreal m_subcellRes;
  
  /// index of the monitored physical variable for positivity preservation
  CFuint m_monitoredPhysVar;
  
  /// extra vector to store the unit normal vectors in the flx pnts
  std::vector< RealVector > m_unitNormalFlxPnts2;
  
  /// storage for the artificial viscosity
  Framework::DataSocketSource<CFreal> socket_artVisc;
  
  /// storage for the monitored phys var
  Framework::DataSocketSource<CFreal> socket_monPhysVar;
  
  /// storage for the smoothness
  Framework::DataSocketSource<CFreal> socket_smoothness;
  
  /// backup for epsilon
  CFreal m_epsBackUp;
  
  private:

  /// Physical data temporary vector
  RealVector m_pData;
  
}; // class Solve

//////////////////////////////////////////////////////////////////////////////

  }  // namespace FluxReconstructionMethod
}  // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_FluxReconstructionMethod_LLAVJacobFluxReconstruction_hh

