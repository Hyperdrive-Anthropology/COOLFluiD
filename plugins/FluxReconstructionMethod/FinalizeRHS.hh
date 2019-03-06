#ifndef COOLFluiD_FluxReconstructionMethod_FinalizeRHS_hh
#define COOLFluiD_FluxReconstructionMethod_FinalizeRHS_hh

//////////////////////////////////////////////////////////////////////////////

#include "Framework/BaseMethodStrategyProvider.hh"
#include "FluxReconstructionMethod/FluxReconstructionSolverData.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {
  
  namespace MathTools { class MatrixInverter; }


  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

/**
 * This class is used to compute the transformed residual
 *
 * @author Ray Vandenhoeck
 */
class FinalizeRHS : public FluxReconstructionSolverCom {

public:  // methods

  /**
   * Defines the Config Option's of this class
   * @param options a OptionList where to add the Option's
   */
  static void defineConfigOptions(Config::OptionList& options);

  /// Constructor
  FinalizeRHS(const std::string& name);

  /// Destructor
  ~FinalizeRHS();

  /// Gets the Class name
  static std::string getClassName()
  {
    return "FinalizeRHS";
  }
  
  /// Returns the DataSocket's that this command needs as sinks
  /// @return a vector of SafePtr with the DataSockets
  std::vector< Common::SafePtr< Framework::BaseDataSocketSink > >
    needsSockets();

  /// Set up private data and data
  void setup();
  
  /// Unset up private data and data
  void unsetup();
  
  /**
   * Finalize computation RHS
   */
  void execute();
  
private: // helper functions
  
  /// Compute the transformation matrix dP/dU numerically
  RealMatrix& computeNumericalTransMatrix(Framework::State& state);
  
  /// Compute the transformation matrix dP/dU analytically
  const RealMatrix& computeAnalyticalTransMatrix(const Framework::State& state) 
  {
    m_solutionToUpdateMatTrans->setMatrix(state);
    return *m_solutionToUpdateMatTrans->getMatrix();
  }


protected: // data
  
  /// socket for state's
  Framework::DataSocketSink<Framework::State*, Framework::GLOBAL> socket_states;
  
  /// storage of the rhs
  Framework::DataSocketSink<CFreal> socket_rhs;
  
  /// variable for physical data
  RealVector m_solPhysData;
  
  // dummy jacobian
  RealMatrix m_jacobDummy;
  
  // inverse of the dummy jacobian
  RealMatrix m_invJacobDummy;
  
  /// Vector transformer from update to solution variables
  Common::SafePtr<Framework::VarSetTransformer> m_updateToSolutionVecTrans;
  
  /// Matrix transformer from solution to update variables
  /// starting from update variables
  Common::SafePtr<Framework::VarSetMatrixTransformer> m_solutionToUpdateMatTrans;
  
  /// vector to store the a state
  RealVector m_state;
  
  /// vector to store the finite difference deriv for state transformation
  RealVector m_dState;

  /// temporary data for holding the matrix inverter
  std::auto_ptr<MathTools::MatrixInverter>  m_inverter;
  
  /// pointer to the numerical Jacobian computer
  Common::SafePtr<Framework::NumericalJacobian> m_numJacob;
  
  /// vector to temporarily store residuals
  RealVector m_tempRes;
  
  /// vector to temporarily store residuals
  RealVector m_tempRes2;
  
  /// flag telling if to use analytical transformation matrix
  bool m_useAnalyticalMatrix;
  
  /// flag telling if variable transformation is needed
  bool m_doVarTrans;

}; // class FinalizeRHS

//////////////////////////////////////////////////////////////////////////////

  }  // namespace FluxReconstructionMethod

}  // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif  // COOLFluiD_Numerics_FluxReconstructionMethod_FinalizeRHS_hh

