#ifndef COOLFluiD_FluxReconstructionMethod_FluxData_hh
#define COOLFluiD_FluxReconstructionMethod_FluxData_hh

//////////////////////////////////////////////////////////////////////////////

#include "Common/COOLFluiD.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

    namespace FluxReconstructionMethod {
            
//////////////////////////////////////////////////////////////////////////////

template <typename PHYS>      
class FluxData {
public:
  /// initialize the flux data
  HOST_DEVICE void initialize() 
  {
    for (CFuint j = 0; j < 4; j++) {for (CFuint i = 0; i < PHYS::NBEQS; ++i) {m_states[j][i] = 0.;}}
    for (CFuint j = 0; j < 2; j++) {for (CFuint i = 0; i < PHYS::NBEQS; ++i) {m_rstates[j][i] = 0.;}}
    for (CFuint j = 0; j < 2; j++) {for (CFuint i = 0; i < PHYS::DIM; ++i) {m_nodes[j][i] = 0.;}}
    for (CFuint j = 0; j < 2; j++) {for (CFuint i = 0; i < PHYS::DIM; ++i) {m_rnodes[j][i] = 0.;}}
    for (CFuint i = 0; i < 4; ++i) {for (CFuint j = 0; j < PHYS::DIM; ++j) {for (CFuint k = 0; k < PHYS::NBEQS; ++k) {m_flux[i][j][k] = 0.;}}}
    for (CFuint i = 0; i < 4; ++i) {for (CFuint j = 0; j < PHYS::DIM*PHYS::DIM; ++j) {m_unitNormal[i][j] = 0.;}}
    m_updateCoeff = 0.; m_faceArea = 0.; 
    m_stateID[0] = 0; m_stateID[1] = 0; 
    m_nbSolPnts = 0;
    m_isBFace = false; m_isOutward = false; m_isPerturb = false; 
  }
    
  /// get the left or right state
  HOST_DEVICE CFreal* getState(const CFuint iState) {return &m_states[iState][0];}
    
  /// get the left or right reconstructed state
  HOST_DEVICE CFreal* getRstate(const CFuint iState) {return &m_rstates[iState][0];}
    
  /// get the left or right state node
  HOST_DEVICE CFreal* getNode(const CFuint i) {return &m_nodes[i][0];}
  
  /// get the left or right reconstructed node
  HOST_DEVICE CFreal* getRnode(const CFuint i) {return &m_rnodes[i][0];}
  
  /// get the flux array
  HOST_DEVICE CFreal* getFlux(const CFuint iSol, const CFuint iDim) {return &m_flux[iSol][iDim][0];}
  
  /// get the face normal scaled with jacobian
  HOST_DEVICE CFreal* getScaledNormal(const CFuint iSol) {return &m_unitNormal[iSol][0];}
  
  /// get the face area
  HOST_DEVICE CFreal getFaceArea() const {return m_faceArea;}
  
  /// set the face area
  HOST_DEVICE void setFaceArea(const CFreal area) {m_faceArea = area;}
  
  /// get the boundary flag
  HOST_DEVICE bool isBFace() const {return m_isBFace;}
  
  /// set the boundary flag
  HOST_DEVICE void setIsBFace(const bool flag) {m_isBFace = flag;}
  
  /// get the flag telling if the normal is outward
  HOST_DEVICE bool isOutward() const {return m_isOutward;}
  
  /// set the flag telling if the normal is outward
  HOST_DEVICE void setIsOutward(const bool flag) {m_isOutward = flag;}
  
  /// get the flag telling if jacobian perturbation is applied
  HOST_DEVICE bool isPerturb() const {return m_isPerturb;}
  
  /// set the flag telling if jacobian perturbation is applied
  HOST_DEVICE void setIsPerturb(bool flag) {m_isPerturb = flag;}
  
  /// get the state IDs
  HOST_DEVICE CFuint getStateID(const CFuint iState) const {return m_stateID[iState];}
  
  /// get number of solution points
  HOST_DEVICE CFuint getNbSolPnts() const {return m_nbSolPnts;}
  
  /// set number of solution points
  HOST_DEVICE void setNbSolPnts(const CFuint nbSolPnts) {m_nbSolPnts = nbSolPnts;}
  
  /// set the state IDs
  HOST_DEVICE void setStateID(const CFuint iState, const CFuint sID) {m_stateID[iState] = sID;}
  
  /// get the update coefficient
  HOST_DEVICE CFreal getUpdateCoeff() {return m_updateCoeff;}
  
  /// set the update coefficient
  HOST_DEVICE void setUpdateCoeff(const CFreal coeff) {m_updateCoeff = coeff;}
    
private:
  
  /// left and right states
  CFreal m_states[2][PHYS::NBEQS];
   
  /// left and right reconstructed states
  CFreal m_rstates[2][PHYS::NBEQS];
    
  /// left and right state nodes
  CFreal m_nodes[2][PHYS::DIM];
  
  /// left and right reconstructed nodes
  CFreal m_rnodes[2][PHYS::DIM];
  
  /// residual
  CFreal m_flux[4][PHYS::DIM][PHYS::NBEQS];
  
  /// unit normal
  CFreal m_unitNormal[4][PHYS::DIM*PHYS::DIM];
  
  /// left and right update coefficient
  CFreal m_updateCoeff;
  
  /// face length/area
  CFreal m_faceArea;
  
  /// state IDs
  CFuint m_stateID[2];
  
  /// number of solution points
  CFuint m_nbSolPnts;
  
  /// flag telling if this is a boundary face
  bool m_isBFace;
  
  /// flag telling if the normal is outward
  bool m_isOutward;
  
  /// flag telling if jacobian perturbation has to be applied
  bool m_isPerturb;
  
};
      
//////////////////////////////////////////////////////////////////////////////

    } // namespace FluxReconstructionMethod

} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOLFluiD_Numerics_FluxReconstructionMethod_FluxData_hh


