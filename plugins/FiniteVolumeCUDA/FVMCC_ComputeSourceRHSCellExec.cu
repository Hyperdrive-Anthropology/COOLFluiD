#include "FiniteVolumeCUDA/FVMCC_ComputeSourceRHSCellExec.hh"
#include "Framework/MeshData.hh"
#include "Framework/CellConn.hh"
#include "Config/ConfigOptionPtr.hh"
#include "Framework/CudaDeviceManager.hh"
#include "Common/SelfRegistPtr.hh"
#include "Common/CUDA/CFVec.hh"
#include "Framework/CudaTimer.hh"
#include "FiniteVolume/FluxData.hh"
#include "FiniteVolume/KernelData.hh"
#include "FiniteVolume/CellData.hh"

#include "FiniteVolumeCUDA/FiniteVolumeCUDA.hh"
#include "Framework/MethodCommandProvider.hh"
#include "Framework/VarSetListT.hh"
#include "FiniteVolume/LaxFriedFlux.hh"
#include "FiniteVolume/LeastSquareP1PolyRec2D.hh"
#include "FiniteVolume/LeastSquareP1PolyRec3D.hh"
#include "FiniteVolume/BarthJesp.hh"
#include "FiniteVolume/Venktn2D.hh"
#include "MHD/MHD2DProjectionConsT.hh"
#include "MHD/MHD3DProjectionConsT.hh"
#include "MHD/MHD2DProjectionPrimT.hh"
#include "MHD/MHD3DProjectionPrimT.hh"
#include "MHD/MHDProjectionPrimToConsT.hh"
#include "FiniteVolumeMHD/LaxFriedFluxTanaka.hh"
#include "MHD/MHD2DProjectionVarSet.hh"   
#include "MHD/MHD3DProjectionVarSet.hh"

#include "Maxwell/Maxwell2DProjectionVarSet.hh"
#include "Maxwell/Maxwell2DProjectionConsT.hh"
#include "FiniteVolumeMaxwell/StegerWarmingMaxwellProjection2D.hh"

#include "MultiFluidMHD/MultiFluidMHDVarSet.hh"
#include "MultiFluidMHD/EulerMFMHD2DHalfConsT.hh"
#include "MultiFluidMHD/EulerMFMHD2DHalfRhoiViTiT.hh"
#include "MultiFluidMHD/EulerMFMHD2DHalfRhoiViTiToConsT.hh"
#include "MultiFluidMHD/EulerMFMHD2DHalfConsToRhoiViTiT.hh"
#include "MultiFluidMHD/EulerMFMHD2DConsT.hh"
#include "MultiFluidMHD/EulerMFMHD2DRhoiViTiT.hh"
#include "MultiFluidMHD/EulerMFMHD2DRhoiViTiToConsT.hh"
#include "MultiFluidMHD/EulerMFMHD2DConsToRhoiViTiT.hh"
#include "FiniteVolumeMultiFluidMHD/AUSMPlusUpFluxMultiFluid.hh"
#include "FiniteVolumeMultiFluidMHD/AUSMFluxMultiFluid.hh"
#include "FiniteVolumeMultiFluidMHD/DriftWaves2DHalfTwoFluid.hh"
#include "FiniteVolumeMultiFluidMHD/HartmannSourceTerm.hh"

//////////////////////////////////////////////////////////////////////////////

using namespace COOLFluiD::Framework;
using namespace COOLFluiD::Common;
using namespace COOLFluiD::Config;
using namespace COOLFluiD::Physics::MHD;
using namespace COOLFluiD::Physics::Maxwell;
using namespace COOLFluiD::Physics::MultiFluidMHD;

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Numerics {

    namespace FiniteVolume {

//////////////////////////////////////////////////////////////////////////////


//Provider for AUSMPlusUpFlux with Source
#define FVMCC_MULTIFLUIDMHD_RHS_PROV_AUSMPLUSUP_SOURCE(__dim__,__half__,__svars__,__uvars__,__sourceterm__,__limiter__,__nbBThreads__,__providerName__) \
MethodCommandProvider<FVMCC_ComputeSourceRHSCellExec<AUSMPlusUpFluxMultiFluid<MultiFluidMHDVarSet<Maxwell##__dim__##ProjectionVarSet> >, \
			              VarSetListT<EulerMFMHD##__dim__##__half__##__svars__##T, EulerMFMHD##__dim__##__half__##__uvars__##T>, \
				      __sourceterm__<MultiFluidMHDVarSet<Maxwell##__dim__##ProjectionVarSet> >, \
				      LeastSquareP1PolyRec##__dim__ , __limiter__, __nbBThreads__>, \
		      CellCenterFVMData, FiniteVolumeCUDAModule>	\
fvmcc_RhsMultiFluidMHD##__limiter__##AUSMPlusUp##__dim__##__half__##__svars__##__uvars__##__sourceterm__##__nbBThreads__##Provider(__providerName__);

// 48 block threads (default)
FVMCC_MULTIFLUIDMHD_RHS_PROV_AUSMPLUSUP_SOURCE(2D,Half,Cons,RhoiViTi,DriftWaves2DHalfTwoFluid,BarthJesp,48,"CellBarthAUSMPlusUpEulerMFMHD2DHalfRhoiViTiDriftWavesTwoFluid")
FVMCC_MULTIFLUIDMHD_RHS_PROV_AUSMPLUSUP_SOURCE(2D,Half,Cons,RhoiViTi,DriftWaves2DHalfTwoFluid,Venktn2D,48,"CellVenktnAUSMPlusUpEulerMFMHD2DHalfRhoiViTiHartmann")
#undef FVMCC_MULTIFLUIDMHD_RHS_PROV_AUSMPLUSUP_SOURCE

//////////////////////////////////////////////////////////////////////////////

template <typename PHYS>
HOST_DEVICE inline void setState(CFreal* state, CFreal* statePtr, 
				 CFreal* node, CFreal* nodePtr)
{
  // copy the state node data to shared memory
  for (CFuint i = 0; i < PHYS::DIM; ++i) {node[i] = nodePtr[i];}
  // copy the state data to shared memory
  for (CFuint i = 0; i < PHYS::NBEQS; ++i) {state[i] = statePtr[i];} 
}
      
//////////////////////////////////////////////////////////////////////////////
      
template <typename PHYS>
HOST_DEVICE inline void setFaceNormal(FluxData<PHYS>* fd, CFreal* normal)
{
  CudaEnv::CFVecSlice<CFreal,PHYS::DIM> n(normal);
  const CFreal area = n.norm2();
  fd->setFaceArea(area);
  const CFreal ovArea = 1./area;
  CudaEnv::CFVecSlice<CFreal,PHYS::DIM> un(fd->getUnitNormal());
  for (CFuint i = 0; i < PHYS::DIM; ++i) {
    un[i] = n[i]*ovArea;
  }
}
      
//////////////////////////////////////////////////////////////////////////////

template <typename PHYS, typename PTR>
HOST_DEVICE void setFluxData(const CFuint f, const CFint stype, 
			     const CFuint stateID, const CFuint cellID, 
			     KernelData<CFreal>* kd, FluxData<PHYS>* fd,
			     PTR cellFaces)
{
  fd->setStateID(RIGHT, stateID);
  CFreal* statePtrR = (stype > 0) ? &kd->states[stateID*PHYS::NBEQS] : &kd->ghostStates[stateID*PHYS::NBEQS];  
  CFreal* nodePtrR = (stype > 0) ? &kd->centerNodes[stateID*PHYS::DIM] : &kd->ghostNodes[stateID*PHYS::DIM];  
  setState<PHYS>(fd->getState(RIGHT), statePtrR, fd->getNode(RIGHT), nodePtrR);
  
  fd->setIsBFace(stype < 0);
  fd->setStateID(LEFT, cellID);
  const CFuint faceID = cellFaces[f*kd->nbCells + cellID];
  fd->setIsOutward(kd->isOutward[faceID] == cellID);
  
  CFreal* statePtrL = &kd->states[cellID*PHYS::NBEQS];
  CFreal* nodePtrL = &kd->centerNodes[cellID*PHYS::DIM];
  setState<PHYS>(fd->getState(LEFT), statePtrL, fd->getNode(LEFT), nodePtrL);
  setFaceNormal<PHYS>(fd, &kd->normals[faceID*PHYS::DIM]);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T, CFuint SIZE>
void print(const std::string& name, T* array) 
{
  CFLog(INFO, name << " = \t");
  for (CFuint i = 0; i < SIZE; ++i) {
    CFLog(INFO, array[i] << " ");
  }
  CFLog(INFO, "\n");
}

//////////////////////////////////////////////////////////////////////////////

template <typename MODEL>
HOST_DEVICE void computeFaceCentroid(const CellData::Itr* cell, const CFuint faceIdx, 
				     const CFreal* nodes, CFreal* midFaceCoord)
{  
  CudaEnv::CFVecSlice<CFreal, MODEL::DIM> coord(midFaceCoord);
  coord = 0.;
  const CFuint nbFaceNodes = cell->getNbFaceNodes(faceIdx);
  const CFreal ovNbFaceNodes = 1./(static_cast<CFreal>(nbFaceNodes));
  for (CFuint n = 0; n < nbFaceNodes; ++n) {
    const CFuint cellNodeID = cell->getNodeID(faceIdx, n);
    const CFuint nodeID = cell->getNodeID(faceIdx,n);
    const CFreal* faceNode = &nodes[nodeID*MODEL::DIM];
    for (CFuint d = 0; d < MODEL::DIM; ++d) {
      coord[d] += faceNode[d];
    }
  }
  coord *= ovNbFaceNodes;
}

//////////////////////////////////////////////////////////////////////////////

template <typename PHYS, typename POLYREC>
__global__ void computeGradientsKernel(typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor,
				       const CFuint nbCells,
				       CFreal* states, 
				       CFreal* nodes,
				       CFreal* centerNodes,
				       CFreal* ghostStates,
				       CFreal* ghostNodes,
				       CFreal* uX,
				       CFreal* uY,
				       CFreal* uZ,
				       CFreal* limiter,
				       CFreal* updateCoeff, 
				       CFreal* rhs,
				       CFreal* normals,
				       CFint* isOutward,
				       const CFuint* cellInfo,
				       const CFuint* cellStencil,
				       const CFuint* cellFaces,
				       const CFuint* cellNodes,
				       const CFint*  neighborTypes,
				       const Framework::CellConn* cellConn)
{    
  // each thread takes care of computing the gradient for one single cell
  const int cellID = threadIdx.x + blockIdx.x*blockDim.x;
  
  // __shared__ typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE> s_dcor[32];
  // typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor = &s_dcor[threadIdx.x];
  // dcor->init(gdcor);
  
  if (cellID < nbCells) {    
    KernelData<CFreal> kd (nbCells, states, nodes, centerNodes, ghostStates, ghostNodes, updateCoeff, 
			   rhs, normals, uX, uY, uZ, isOutward);
    
    // compute and store cell gradients at once 
    POLYREC polyRec(dcor);
    CellData cells(nbCells, cellInfo, cellStencil, cellFaces, cellNodes, neighborTypes, cellConn);
    CellData::Itr cell = cells.getItr(cellID);
    polyRec.computeGradients(&states[cellID*PHYS::NBEQS], &centerNodes[cellID*PHYS::DIM], &kd, &cell);
  }
}
      
//////////////////////////////////////////////////////////////////////////////

template <typename PHYS, typename POLYREC, typename LIMITER>
__global__ void computeLimiterKernel(typename LIMITER::BASE::template DeviceConfigOptions<NOTYPE>* dcol,
				     typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor,
				     const CFuint nbCells,
				     CFreal* states, 
				     CFreal* nodes,
				     CFreal* centerNodes,
				     CFreal* ghostStates,
				     CFreal* ghostNodes,
				     CFreal* uX,
				     CFreal* uY,
				     CFreal* uZ,
				     CFreal* limiter,
				     CFreal* updateCoeff, 
				     CFreal* rhs,
				     CFreal* normals,
				     CFint* isOutward,
				     const CFuint* cellInfo,
				     const CFuint* cellStencil,
				     const CFuint* cellFaces,
				     const CFuint* cellNodes,
				     const CFint*  neighborTypes,
				     const Framework::CellConn* cellConn)
{    
  // each thread takes care of computing the gradient for one single cell
  const int cellID = threadIdx.x + blockIdx.x*blockDim.x;
  
  // __shared__ typename LIMITER::BASE::template DeviceConfigOptions<NOTYPE> s_dcol[32];
  // typename LIMITER::BASE::template DeviceConfigOptions<NOTYPE>* dcol = &s_dcol[threadIdx.x];
  // dcol->init(gdcol);
  
  // __shared__ typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE> s_dcor[32];
  // typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor = &s_dcor[threadIdx.x];
  // dcor->init(gdcor);
  
  if (cellID < nbCells) {    
    // compute all cell quadrature points at once (size of this array is overestimated)
    CFreal midFaceCoord[PHYS::DIM*PHYS::DIM*2];
    
    CellData cells(nbCells, cellInfo, cellStencil, cellFaces, cellNodes, neighborTypes, cellConn);
    CellData::Itr cell = cells.getItr(cellID);
    const CFuint nbFacesInCell = cell.getNbFacesInCell();
    for (CFuint f = 0; f < nbFacesInCell; ++f) { 
      computeFaceCentroid<PHYS>(&cell, f, nodes, &midFaceCoord[f*PHYS::DIM]);
    }
    
    // compute cell-based limiter at once
    KernelData<CFreal> kd (nbCells, states, nodes, centerNodes, ghostStates, ghostNodes, updateCoeff, 
			   rhs, normals, uX, uY, uZ, isOutward);
    LIMITER limt(dcol);
    
    if (dcor->currRes > dcor->limitRes && (dcor->limitIter > 0 && dcor->currIter < dcor->limitIter)) {	
      limt.limit(&kd, &cell, &midFaceCoord[0], &limiter[cellID*PHYS::NBEQS]);
    }
    else {
      if (!dcor->freezeLimiter) {
	// historical modification of the limiter
	CudaEnv::CFVec<CFreal,PHYS::NBEQS> tmpLimiter;
	limt.limit(&kd, &cell, &midFaceCoord[0], &tmpLimiter[0]);
	CFuint currID = cellID*PHYS::NBEQS;
	for (CFuint iVar = 0; iVar < PHYS::NBEQS; ++iVar, ++currID) {
	  limiter[currID] = min(tmpLimiter[iVar],limiter[currID]);
	}
      }
    }
  }
}
  
//////////////////////////////////////////////////////////////////////////////
             
template <typename SCHEME, typename POLYREC>
__global__ void computeFluxKernel(typename SCHEME::BASE::template DeviceConfigOptions<NOTYPE>* dcof,
				  typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor,
				  typename SCHEME::MODEL::PTERM::template DeviceConfigOptions<NOTYPE>* dcop,
				  const CFuint nbCells,
				  CFreal* states, 
				  CFreal* nodes,
				  CFreal* centerNodes,
				  CFreal* ghostStates,
				  CFreal* ghostNodes,
				  CFreal* uX,
				  CFreal* uY,
				  CFreal* uZ,
				  CFreal* limiter,
				  CFreal* updateCoeff, 
				  CFreal* rhs,
				  CFreal* normals,
				  CFint* isOutward,
				  const CFuint* cellInfo,
				  const CFuint* cellStencil,
				  const CFuint* cellFaces,
				  const CFuint* cellNodes,
				  const CFint*  neighborTypes,
				  const Framework::CellConn* cellConn)
{    
  // each thread takes care of computing the gradient for one single cell
  const int cellID = threadIdx.x + blockIdx.x*blockDim.x;
  
  // __shared__ typename SCHEME::BASE::template DeviceConfigOptions<NOTYPE> s_dcof[32];
  // typename SCHEME::BASE::template DeviceConfigOptions<NOTYPE>* dcof = &s_dcof[threadIdx.x];
  // dcof->init(gdcof);
  
  // __shared__ typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE> s_dcor[32];
  // typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor = &s_dcor[threadIdx.x];
  // dcor->init(gdcor);
  
  // __shared__ typename SCHEME::MODEL::PTERM::template DeviceConfigOptions<NOTYPE> s_dcop[32];
  // typename SCHEME::MODEL::PTERM::template DeviceConfigOptions<NOTYPE>* dcop = &s_dcop[threadIdx.x];
  // dcop->init(gdcop);
  
  if (cellID < nbCells) {
    // reset the rhs and update coefficients to 0
    CudaEnv::CFVecSlice<CFreal,SCHEME::MODEL::NBEQS> res(&rhs[cellID*SCHEME::MODEL::NBEQS]);
    res = 0.;
    updateCoeff[cellID] = 0.;
    
    KernelData<CFreal> kd (nbCells, states, nodes, centerNodes, ghostStates, ghostNodes, updateCoeff, 
			   rhs, normals, uX, uY, uZ, isOutward);
    
    // compute and store cell gradients at once 
    POLYREC polyRec(dcor);
    SCHEME fluxScheme(dcof);
    CFreal midFaceCoord[SCHEME::MODEL::DIM*SCHEME::MODEL::DIM*2];
    FluxData<typename SCHEME::MODEL> currFd; currFd.initialize();
    typename SCHEME::MODEL pmodel(dcop);
    
    CellData cells(nbCells, cellInfo, cellStencil, cellFaces, cellNodes, neighborTypes, cellConn);
    CellData::Itr cell = cells.getItr(cellID);
    
    // compute the fluxes
    const CFuint nbFacesInCell = cell.getNbActiveFacesInCell();
    for (CFuint f = 0; f < nbFacesInCell; ++f) { 
      const CFint stype = cell.getNeighborType(f);
      
      if (stype != 0) { // skip all partition faces
	// set all flux data for the current face
	const CFuint stateID = cell.getNeighborID(f);
	setFluxData(f, stype, stateID, cellID, &kd, &currFd, cellFaces);
	
	// compute face quadrature points (centroid)
	CFreal* faceCenters = &midFaceCoord[f*SCHEME::MODEL::DIM];
	computeFaceCentroid<typename SCHEME::MODEL>(&cell, f, nodes, faceCenters);
	
	// extrapolate solution on quadrature points on both sides of the face
	polyRec.extrapolateOnFace(&currFd, faceCenters, uX, uY, uZ, limiter);
	
	// compute the convective flux across the face
        fluxScheme.prepareComputation(&currFd, &pmodel);
	fluxScheme(&currFd, &pmodel);
	
	// update the residual
	CudaEnv::CFVecSlice<CFreal,SCHEME::MODEL::NBEQS> ress(currFd.getResidual());
	res -= ress;
	
	// update the update coefficient
	updateCoeff[cellID] += currFd.getUpdateCoeff();
      }
    }
  }
}
 


//////////////////////////////////////////////////////////////////////////////

template <typename SOURCE>
__global__ void computeSource(typename SOURCE::BASE::template DeviceConfigOptions<NOTYPE>* dcos,
				  typename SOURCE::MODEL::PTERM::template DeviceConfigOptions<NOTYPE>* dcop,
				  const CFuint nbCells,
				  CFreal* states, 
                                  CFreal* volumes,
				  CFreal* nodes,
				  CFreal* centerNodes,
				  CFreal* ghostStates,
				  CFreal* ghostNodes,
				  CFreal* uX,
				  CFreal* uY,
				  CFreal* uZ,
				  CFreal* limiter,
				  CFreal* updateCoeff, 
				  CFreal* rhs,
				  CFreal* normals,
				  CFint* isOutward,
				  const CFuint* cellInfo,
				  const CFuint* cellStencil,
				  const CFuint* cellFaces,
				  const CFuint* cellNodes,
				  const CFint*  neighborTypes,
				  const Framework::CellConn* cellConn,
				  CFreal ResFactor, bool IsAxisymmetric)
{
  // each thread takes care of computing the source for one single cell
  const int cellID = threadIdx.x + blockIdx.x*blockDim.x;

  const CFuint nbEqs = SOURCE::MODEL::NBEQS;
  CudaEnv::CFVec<CFreal,SOURCE::MODEL::NBEQS> source;
  source = 0.;

  SOURCE Source(dcos);
  typename SOURCE::MODEL pmodel(dcop);

  CudaEnv::CFVecSlice<CFreal,SOURCE::MODEL::NBEQS> state(&states[cellID*SOURCE::MODEL::NBEQS]);
  Source(&state[0], &pmodel, &source[0]);
      
  CFreal invR = 1.0;
  if (IsAxisymmetric) {     
    //invR /= abs(currCell->getState(0)->getCoordinates()[YY]);  //Not implemented
  }


  for (CFuint iEq = 0; iEq < nbEqs; ++iEq) { 
     rhs[iEq] += ResFactor*source[iEq]*invR;   
  }
}



 
//////////////////////////////////////////////////////////////////////////////

template <typename SCHEME, typename SOURCE, typename POLYREC, typename LIMITER>
void computeFluxSourceCPU(typename SCHEME::BASE::template DeviceConfigOptions<NOTYPE>* dcof,
		    typename POLYREC::BASE::template DeviceConfigOptions<NOTYPE>* dcor,
		    typename LIMITER::BASE::template DeviceConfigOptions<NOTYPE>* dcol,
		    typename SCHEME::MODEL::PTERM::template DeviceConfigOptions<NOTYPE>* dcop,
                    typename SOURCE::BASE::template DeviceConfigOptions<NOTYPE>* dcos,
		    const CFuint nbCells,
		    CFreal* states, 
                    CFreal* volumes,
		    CFreal* nodes,
		    CFreal* centerNodes,
		    CFreal* ghostStates,
		    CFreal* ghostNodes,
		    CFreal* uX,
		    CFreal* uY,
		    CFreal* uZ,
		    CFreal* limiter,
		    CFreal* updateCoeff, 
		    CFreal* rhs,
		    CFreal* normals,
		    CFint* isOutward,
		    const CFuint* cellInfo,
		    const CFuint* cellStencil,
		    const CFuint* cellFaces,
		    const CFuint* cellNodes,
		    const CFint* neighborTypes,
		    const Framework::CellConn* cellConn,
                    CFreal ResFactor, bool IsAxisymmetric)
{ 
  typedef typename SCHEME::MODEL PHYS;
  
  FluxData<PHYS> fd; fd.initialize();
  FluxData<PHYS>* currFd = &fd;
  cf_assert(currFd != CFNULL);
  SCHEME fluxScheme(dcof);
  POLYREC polyRec(dcor);
  LIMITER limt(dcol);
  PHYS pmodel(dcop);
  
  CellData cells(nbCells, cellInfo, cellStencil, cellFaces, cellNodes, neighborTypes, cellConn);
  KernelData<CFreal> kd(nbCells, states, nodes, centerNodes, ghostStates, ghostNodes, updateCoeff, 
			rhs, normals, uX, uY, uZ, isOutward);
  
  CFreal midFaceCoord[PHYS::DIM*PHYS::DIM*2];
  CudaEnv::CFVec<CFreal,PHYS::NBEQS> tmpLimiter;
  
  CudaEnv::CFVec<CFreal,PHYS::NBEQS> source;
  SOURCE Source(dcos);

  // compute the cell-based gradients
  for (CFuint cellID = 0; cellID < nbCells; ++cellID) {
    CellData::Itr cell = cells.getItr(cellID);
    polyRec.computeGradients(&states[cellID*PHYS::NBEQS], &centerNodes[cellID*PHYS::DIM], &kd, &cell);
  }
  
  // compute the cell based limiter
  // for (CFuint cellID = 0; cellID < nbCells; ++cellID) {
  for (CellData::Itr cell = cells.begin(); cell <= cells.end(); ++cell) {
    // compute all cell quadrature points at once (size of this array is overestimated)
    const CFuint nbFacesInCell = cell.getNbFacesInCell();
    for (CFuint f = 0; f < nbFacesInCell; ++f) { 
      computeFaceCentroid<PHYS>(&cell, f, nodes, &midFaceCoord[f*PHYS::DIM]);
    }
    
    const CFuint cellID = cell.getCellID();
    if (dcor->currRes > dcor->limitRes && (dcor->limitIter > 0 && dcor->currIter < dcor->limitIter)) {	
      // compute cell-based limiter
      limt.limit(&kd, &cell, &midFaceCoord[0], &limiter[cellID*PHYS::NBEQS]);
    }
    else {
      if (!dcor->freezeLimiter) {
	// historical modification of the limiter
	limt.limit(&kd, &cell, &midFaceCoord[0], &tmpLimiter[0]);
	CFuint currID = cellID*PHYS::NBEQS;
	for (CFuint iVar = 0; iVar < PHYS::NBEQS; ++iVar, ++currID) {
	  limiter[currID] = min(tmpLimiter[iVar],limiter[currID]);
	}
      }
    }
  }
  
  // compute the fluxes
  for (CellData::Itr cell = cells.begin(); cell <= cells.end(); ++cell) {
    // reset the rhs and update coefficients to 0
    const CFuint cellID = cell.getCellID();
    CudaEnv::CFVecSlice<CFreal,PHYS::NBEQS> res(&rhs[cellID*PHYS::NBEQS]);
    res = 0.;
    updateCoeff[cellID] = 0.;
    
    const CFuint nbFacesInCell = cell.getNbActiveFacesInCell();
    for (CFuint f = 0; f < nbFacesInCell; ++f) { 
      const CFint stype = cell.getNeighborType(f);
      
      if (stype != 0) { // skip all partition faces
	const CFuint stateID =  cell.getNeighborID(f);
	setFluxData(f, stype, stateID, cellID, &kd, currFd, cellFaces);
	
	// compute face quadrature points (centroid)
	CFreal* faceCenters = &midFaceCoord[f*PHYS::DIM];
	computeFaceCentroid<PHYS>(&cell, f, nodes, faceCenters);
	
	// extrapolate solution on quadrature points on both sides of the face
	polyRec.extrapolateOnFace(currFd, faceCenters, uX, uY, uZ, limiter);
        fluxScheme.prepareComputation(currFd, &pmodel);
	fluxScheme(currFd, &pmodel); // compute the convective flux across the face
	
	for (CFuint iEq = 0; iEq < PHYS::NBEQS; ++iEq) {
	  const CFreal value = currFd->getResidual()[iEq];
	  res[iEq] -= value;  // update the residual 
	}
	
	// update the update coefficient
	updateCoeff[cellID] += currFd->getUpdateCoeff();
      }
    }

 
    //Source computation
    source = 0.;
    
 
    CudaEnv::CFVecSlice<CFreal,PHYS::NBEQS> state(&states[cellID*PHYS::NBEQS]);
    Source(&state[0], &pmodel, &source[0]);

    CFreal invR = 1.0;
    if (IsAxisymmetric) {     
      //invR /= abs(currCell->getState(0)->getCoordinates()[YY]);  
    }
    CFreal factor = invR*volumes[cellID]*ResFactor;     

    source *= factor;
    for (CFuint iEq = 0; iEq < PHYS::NBEQS; ++iEq) { 
      res[iEq] += source[iEq];   
    }
    
    /////////////////////////////////////////////


  }
}

//////////////////////////////////////////////////////////////////////////////

template <typename SCHEME, typename PHYSICS, typename SOURCE,typename POLYREC, typename LIMITER, CFuint NB_BLOCK_THREADS>
void FVMCC_ComputeSourceRHSCellExec<SCHEME,PHYSICS,SOURCE,POLYREC,LIMITER,NB_BLOCK_THREADS>::execute()
{
  using namespace COOLFluiD::Framework;
  using namespace COOLFluiD::Common;
  
  CFTRACEBEGIN;
  
  CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() START\n");
  
  this->initializeComputationRHS();
  
  const CFuint nbCells = this->socket_states.getDataHandle().size();
  cf_assert(nbCells > 0);
  DataHandle<CFreal> updateCoeff = this->socket_updateCoeff.getDataHandle();
  DataHandle<CFreal> rhs = this->socket_rhs.getDataHandle();
  DataHandle<CFreal> normals = this->socket_normals.getDataHandle();
  DataHandle<CFint> isOutward = this->socket_isOutward.getDataHandle();  
  
  SafePtr<SCHEME> lf  = this->getMethodData().getFluxSplitter().template d_castTo<SCHEME>();
  SafePtr<POLYREC> pr = this->getMethodData().getPolyReconstructor().template d_castTo<POLYREC>();
  SafePtr<LIMITER> lm = this->getMethodData().getLimiter().template d_castTo<LIMITER>();
  SafePtr<typename PHYSICS::PTERM> phys = PhysicalModelStack::getActive()->getImplementor()->
    getConvectiveTerm().template d_castTo<typename PHYSICS::PTERM>();
  
  typedef typename SCHEME::template DeviceFunc<GPU, PHYSICS> FluxScheme;  
  typedef typename POLYREC::template DeviceFunc<PHYSICS> PolyRec;  
  typedef typename LIMITER::template DeviceFunc<PHYSICS> Limiter;  
  
  //Added for Source
  SelfRegistPtr<SOURCE> ls1  = (*this->getMethodData().getSourceTermComputer())[0].template d_castTo<SOURCE>();  //Only valid if there is only one source term!!
  SafePtr<SOURCE> ls = ls1.getPtr();
  typedef typename SOURCE::template DeviceFunc<GPU, PHYSICS> SourceTerm; 
  
  if (this->m_onGPU) {
    
    CudaEnv::CudaTimer& timer = CudaEnv::CudaTimer::getInstance();
    timer.start();
    
    // copy of data that change at every iteration
    this->socket_states.getDataHandle().getGlobalArray()->put();
    this->socket_volumes.getDataHandle().getLocalArray()->put(); 
    this->m_ghostStates.put();
    
    CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() => CPU-->GPU data transfer took " << timer.elapsed() << " s\n");
    timer.start();
    
    ConfigOptionPtr<POLYREC, NOTYPE, GPU> dcor(pr);
    ConfigOptionPtr<LIMITER, NOTYPE, GPU> dcol(lm);
    ConfigOptionPtr<SCHEME,  NOTYPE, GPU> dcof(lf);
    ConfigOptionPtr<typename PHYSICS::PTERM, NOTYPE, GPU> dcop(phys);
    
    //Added for Source    
    ConfigOptionPtr<SOURCE, NOTYPE, GPU> dcos(ls);
    
    const CFuint blocksPerGrid = CudaEnv::CudaDeviceManager::getInstance().getBlocksPerGrid(nbCells);
    const CFuint nThreads = CudaEnv::CudaDeviceManager::getInstance().getNThreads();
    
    //dim3 blocks(m_nbBlocksPerGridX, m_nbBlocksPerGridY);
    
    //cudaFuncSetCacheConfig("computeGradientsKernel", cudaFuncCachePreferL1);
    
    // compute the cell-based gradients
    computeGradientsKernel<PHYSICS, PolyRec> <<<blocksPerGrid,nThreads>>> 
      (dcor.getPtr(),
       nbCells,
       this->socket_states.getDataHandle().getGlobalArray()->ptrDev(), 
       this->socket_nodes.getDataHandle().getGlobalArray()->ptrDev(),
       this->m_centerNodes.ptrDev(), 
       this->m_ghostStates.ptrDev(),
       this->m_ghostNodes.ptrDev(),
       this->socket_uX.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uY.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uZ.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_limiter.getDataHandle().getLocalArray()->ptrDev(),
       updateCoeff.getLocalArray()->ptrDev(), 
       rhs.getLocalArray()->ptrDev(),
       normals.getLocalArray()->ptrDev(),
       isOutward.getLocalArray()->ptrDev(),
       this->m_cellInfo.ptrDev(),
       this->m_cellStencil.ptrDev(),
       this->m_cellFaces->getPtr()->ptrDev(),
       this->m_cellNodes->getPtr()->ptrDev(),
       this->m_neighborTypes.ptrDev(),
       this->m_cellConn.ptrDev());
    
    CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() => computeGradientsKernel took " << timer.elapsed() << " s\n");
    
    timer.start();
    
    // cudaFuncSetCacheConfig("computeLimiterKernel", cudaFuncCachePreferL1);
    
    // compute the limiter in each cell
    computeLimiterKernel<PHYSICS, PolyRec, Limiter> <<<blocksPerGrid,nThreads>>> 
      (dcol.getPtr(),
       dcor.getPtr(),
       nbCells,
       this->socket_states.getDataHandle().getGlobalArray()->ptrDev(), 
       this->socket_nodes.getDataHandle().getGlobalArray()->ptrDev(),
       this->m_centerNodes.ptrDev(), 
       this->m_ghostStates.ptrDev(),
       this->m_ghostNodes.ptrDev(),
       this->socket_uX.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uY.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uZ.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_limiter.getDataHandle().getLocalArray()->ptrDev(),
       updateCoeff.getLocalArray()->ptrDev(), 
       rhs.getLocalArray()->ptrDev(),
       normals.getLocalArray()->ptrDev(),
       isOutward.getLocalArray()->ptrDev(),
       this->m_cellInfo.ptrDev(),
       this->m_cellStencil.ptrDev(),
       this->m_cellFaces->getPtr()->ptrDev(),
       this->m_cellNodes->getPtr()->ptrDev(),
       this->m_neighborTypes.ptrDev(),
       this->m_cellConn.ptrDev());
    
    CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() => computeLimiterKernel took " << timer.elapsed() << " s\n");
    
    timer.start();
    
    // cudaFuncSetCacheConfig("computeFluxKernel", cudaFuncCachePreferL1);
    
    // compute the convective flux in each cell
    computeFluxKernel<FluxScheme, PolyRec> <<<blocksPerGrid,nThreads>>> 
      (dcof.getPtr(),
       dcor.getPtr(),
       dcop.getPtr(),
       nbCells,
       this->socket_states.getDataHandle().getGlobalArray()->ptrDev(), 
       this->socket_nodes.getDataHandle().getGlobalArray()->ptrDev(),
       this->m_centerNodes.ptrDev(), 
       this->m_ghostStates.ptrDev(),
       this->m_ghostNodes.ptrDev(),
       this->socket_uX.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uY.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uZ.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_limiter.getDataHandle().getLocalArray()->ptrDev(),
       updateCoeff.getLocalArray()->ptrDev(), 
       rhs.getLocalArray()->ptrDev(),
       normals.getLocalArray()->ptrDev(),
       isOutward.getLocalArray()->ptrDev(),
       this->m_cellInfo.ptrDev(),
       this->m_cellStencil.ptrDev(),
       this->m_cellFaces->getPtr()->ptrDev(),
       this->m_cellNodes->getPtr()->ptrDev(),
       this->m_neighborTypes.ptrDev(),
       this->m_cellConn.ptrDev());
    
    CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() => computeFluxKernel took " << timer.elapsed() << " s\n");

    timer.start();
    CFLog(VERBOSE, "FVMCC_ComputeRHS::execute() => before computeSourceTerm()\n");

    bool IsAxisymmetric = this->getMethodData().isAxisymmetric(); //Default = false
    CFreal ResFactor = this->getMethodData().getResFactor(); //Default = 1

    computeSource<SourceTerm> <<<blocksPerGrid,nThreads>>> 
      (dcos.getPtr(),
       dcop.getPtr(),
       nbCells,
       this->socket_states.getDataHandle().getGlobalArray()->ptrDev(), 
       this->socket_volumes.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_nodes.getDataHandle().getGlobalArray()->ptrDev(),
       this->m_centerNodes.ptrDev(), 
       this->m_ghostStates.ptrDev(),
       this->m_ghostNodes.ptrDev(),
       this->socket_uX.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uY.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_uZ.getDataHandle().getLocalArray()->ptrDev(),
       this->socket_limiter.getDataHandle().getLocalArray()->ptrDev(),
       updateCoeff.getLocalArray()->ptrDev(), 
       rhs.getLocalArray()->ptrDev(),
       normals.getLocalArray()->ptrDev(),
       isOutward.getLocalArray()->ptrDev(),
       this->m_cellInfo.ptrDev(),
       this->m_cellStencil.ptrDev(),
       this->m_cellFaces->getPtr()->ptrDev(),
       this->m_cellNodes->getPtr()->ptrDev(),
       this->m_neighborTypes.ptrDev(),
       this->m_cellConn.ptrDev(),
       ResFactor, IsAxisymmetric);

    CFLog(VERBOSE, "FVMCC_ComputeRHS::execute() => computeSourceTerm took " << timer.elapsed() << "\n");
    
    timer.start();
    rhs.getLocalArray()->get();
    updateCoeff.getLocalArray()->get();
    CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() => GPU-->CPU data transfer took " << timer.elapsed() << " s\n");
  }
  else {
    // AL: useful fo debugging
    // for (CFuint i = 0; i <  m_ghostStates.size()/9; ++i) {
    //   std::cout.precision(12); std::cout << "g" << i << " => ";
    //   for (CFuint j = 0; j < 9; ++j) {
    // 	std::cout << m_ghostStates[i*9+j] << " ";
    //   }
    //   std::cout << "\n";
    // }
    // for (CFuint i = 0; i <  socket_states.getDataHandle().size(); ++i) {
    //   std::cout.precision(12); std::cout << i << " => "<< *socket_states.getDataHandle()[i] <<"\n";
    // }
    
    ConfigOptionPtr<SCHEME>  dcof(lf);
    ConfigOptionPtr<POLYREC> dcor(pr);
    ConfigOptionPtr<LIMITER> dcol(lm);
    ConfigOptionPtr<typename PHYSICS::PTERM> dcop(phys);
    ConfigOptionPtr<SOURCE> dcos(ls);

    bool IsAxisymmetric = this->getMethodData().isAxisymmetric(); //Default = false
    CFreal ResFactor = this->getMethodData().getResFactor(); //Default = 1

    computeFluxSourceCPU<FluxScheme, SourceTerm, PolyRec, Limiter>
      (dcof.getPtr(),
       dcor.getPtr(),
       dcol.getPtr(),
       dcop.getPtr(),
       dcos.getPtr(),
       nbCells,
       this->socket_states.getDataHandle().getGlobalArray()->ptr(), 
       this->socket_volumes.getDataHandle().getLocalArray()->ptr(),
       this->socket_nodes.getDataHandle().getGlobalArray()->ptr(),
       this->m_centerNodes.ptr(), 
       this->m_ghostStates.ptr(),
       this->m_ghostNodes.ptr(),
       this->socket_uX.getDataHandle().getLocalArray()->ptr(),
       this->socket_uY.getDataHandle().getLocalArray()->ptr(),
       this->socket_uZ.getDataHandle().getLocalArray()->ptr(),
       this->socket_limiter.getDataHandle().getLocalArray()->ptr(),
       updateCoeff.getLocalArray()->ptr(), 
       rhs.getLocalArray()->ptr(),
       normals.getLocalArray()->ptr(),
       isOutward.getLocalArray()->ptr(),
       this->m_cellInfo.ptr(),
       this->m_cellStencil.ptr(),
       this->m_cellFaces->getPtr()->ptr(),
       this->m_cellNodes->getPtr()->ptr(),
       this->m_neighborTypes.ptr(),
       this->m_cellConn.ptr(),
       ResFactor, IsAxisymmetric);
  }
  
// for (int i = 0; i < updateCoeff.size(); ++i) {
//      std::cout << "updateCoeff[" << i << "] = " << updateCoeff[i]  << std::endl;
//       /* std::cout << "rhs[" << i << "] = ";
//        for (int j = 0; j < 9; ++j) {
//          std::cout << rhs[i*9+j] << " ";
//        }
//        std::cout << std::endl;*/
// } 
//   abort();
  
  this->finalizeComputationRHS();
  
  CFLog(VERBOSE, "FVMCC_ComputeSourceRHSCellExec::execute() END\n");
  
  CFTRACEEND;
}

//////////////////////////////////////////////////////////////////////////////

    } // namespace FiniteVolume
    
  } // namespace Numerics

} // namespace COOLFluiD
