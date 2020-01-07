#ifndef COOLFluiD_FluxReconstructionMethod_BCStateComputer_hh
#define COOLFluiD_FluxReconstructionMethod_BCStateComputer_hh

//////////////////////////////////////////////////////////////////////////////

#include "Framework/BaseMethodStrategyProvider.hh"
#include "FluxReconstructionMethod/FluxReconstructionSolverData.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {
  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

/**
 * This class represents a basic strategy that computes the ghost states corresponding
 * to a boundary condition
 *
 * @author Kris Van den Abeele
 * @author Ray Vandenhoeck
 * 
 */
class BCStateComputer : public FluxReconstructionSolverStrategy {

public:  // types

  typedef Framework::BaseMethodStrategyProvider< FluxReconstructionSolverData,BCStateComputer > PROVIDER;

public:  // methods

  /**
   * Defines the Config Option's of this class
   * @param options a OptionList where to add the Option's
   */
  static void defineConfigOptions(Config::OptionList& options);

  /// Constructor
  BCStateComputer(const std::string& name);

  /// Destructor
  ~BCStateComputer();

  /// Gets the Class name
  static std::string getClassName()
  {
    return "BCStateComputer";
  }

  /// Gets the polymorphic type name
  virtual std::string getPolymorphicTypeName() {return getClassName();}
  
  /// Set up private data and data
  virtual void setup();
  
   //Added from FV for parallelization
  /**
   * Set the preProcesses connectivity between faces belonging to different process
   *
   */
  virtual void preProcess();

  
  /// Unset up private data and data
  virtual void unsetup();

  /// function returning a boolean that is true if the boundary condition requires the spatial coordinates
  bool needsSpatialCoordinates()
  {
    return m_needsSpatCoord;
  }

  /// function returning a boolean that is true if the boundary condition requires the extra variables
  bool needsExtraVariables()
  {
    return m_needsExtraVars;
  }
  
  /// set the current face
  void setFace(Framework::GeometricEntity *const face)
  {
      m_face = face;
  }

  /// adds a trs name
  void addTRSName(const std::string trsName)
  {
    m_trsNames.push_back(trsName);
  }

  /// get trs names
  Common::SafePtr< std::vector< std::string > > getTRSNames()
  {
    return &m_trsNames;
  }

  /// set extra variables in the flux points
  void setExtraVars(std::vector< RealVector* >* extraVars)
  {
    m_extraVars = extraVars;
  }

  /**
   * Sets the ghost states in all the boundary points (depends on the boundary condition type)
   */
  virtual void computeGhostStates(const std::vector< Framework::State* >& intStates,
                                  std::vector< Framework::State* >& ghostStates,
                                  const std::vector< RealVector >& normals,
                                  const std::vector< RealVector >& coords) = 0;

  /**
   * Sets the ghost gradients in all the boundary points (depends on the boundary condition type)
   */
   virtual void computeGhostGradients(const std::vector< std::vector< RealVector* > >& intGrads,
                                      std::vector< std::vector< RealVector* > >& ghostGrads,
                                      const std::vector< RealVector >& normals,
                                      const std::vector< RealVector >& coords) = 0;

protected: // methods

  /**
 * Configures the command.
   */
  void configure ( Config::ConfigArgs& args );

private: // methods

  /**
   * Adds curvature to the boundary faces using the domain model
   */
    void addCurvatureToBndFaces();

protected: // data

  /// boolean telling whether the boundary condition needs the coordinates of the flux points
  bool m_needsSpatCoord;

  /// boolean telling whether the boundary condition needs extra variables in the flux points
  bool m_needsExtraVars;

  /// list of names of TRSs the BC applies to
  std::vector< std::string > m_trsNames;

  /// pointer to extra variables in the boundary points
  std::vector< RealVector* >* m_extraVars;

  /// boolean telling whether to use the domain model to add curvature to the boundary faces
  bool m_useDomainModel;
  
  /// variable for current face
  Framework::GeometricEntity* m_face;

}; // class BCStateComputer

//////////////////////////////////////////////////////////////////////////////

  }  // namespace FluxReconstructionMethod

}  // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif  // COOLFluiD_FluxReconstructionMethod_BCStateComputer_hh

