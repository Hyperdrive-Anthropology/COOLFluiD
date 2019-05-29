#ifndef COOLFluiD_Numerics_FluctSplit_ScalarDiffusionTerm_hh
#define COOLFluiD_Numerics_FluctSplit_ScalarDiffusionTerm_hh

//////////////////////////////////////////////////////////////////////////////

#include "Framework/DiffusiveVarSet.hh"
#include "ComputeDiffusiveTerm.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Physics {
    namespace LinearAdv {
      class AdvectionDiffusionVarSet;
      class LinearAdvVarSet;
    }
  }
  
  namespace FluctSplit {
    
//////////////////////////////////////////////////////////////////////////////

/**
 * This class computes the diffusive flux corresponding to the Advection diffusion
 * physical model
 *
 * @author Nadege Villedieu
 *
 */
class ScalarDiffusionTerm : public ComputeDiffusiveTerm {
public:

  /**
   * Constructor
   */
  ScalarDiffusionTerm(const std::string& name);

  /**
   * Default destructor
   */
  virtual ~ScalarDiffusionTerm();

  /**
   * Configure the object
   */
  virtual void configure ( Config::ConfigArgs& args );

  /**
   * Set up private data to prepare the simulation
   */
  void setup();

  /**
   * Set the update variable set
   */
  void setDiffusiveVarSet(Common::SafePtr<Framework::DiffusiveVarSet> diffVar);

  /**
   * Compute the diffusive term flux in the current cell
   */
  void computeDiffusiveTerm(Framework::GeometricEntity *const geo,
          std::vector<RealVector>& result,
          const bool updateCoeff);
  /**
    * Set the update variable set
    */
  void setUpdateVarSet(Common::SafePtr<Framework::ConvectiveVarSet>);

  void computePicardDiffJacob(Framework::GeometricEntity *const cell,std::vector<RealMatrix*>& _diffjacob);

protected: // data

  // acquaintance of the diffusive variable set
  Common::SafePtr<Physics::LinearAdv::AdvectionDiffusionVarSet> _diffVar;

  // acquaintance of the update convective var set
  Common::SafePtr<Physics::LinearAdv::LinearAdv2DVarSet> _updateVar;

  // array of cell states
  std::vector<RealVector*> _states;

  // array of values (rho, u, v, T)
  RealMatrix _values;

  // array of gradients
  std::vector<RealVector*> _gradients;

  // array of average values (rho, u, v, T)
  Framework::State* _avValues;

  // unit normal
  RealVector _normal;

  /// temporary residual pointer after transformation
  RealVector * m_phi;

  RealVector normaljState;

}; // end of class AdvectionDiffusionTerm

//////////////////////////////////////////////////////////////////////////////

    } // namespace FluctSplit



} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_Numerics_FluctSplit_AdvectionDiffusionTerm_hh
