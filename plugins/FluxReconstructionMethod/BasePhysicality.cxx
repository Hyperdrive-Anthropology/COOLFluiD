#include "Framework/MethodCommandProvider.hh"
#include "Framework/MeshData.hh"

#include "FluxReconstructionMethod/FluxReconstruction.hh"
#include "FluxReconstructionMethod/BasePhysicality.hh"
#include "FluxReconstructionMethod/FluxReconstructionElementData.hh"

//////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace COOLFluiD::Common;
using namespace COOLFluiD::Framework;
using namespace COOLFluiD::MathTools;
using namespace COOLFluiD::Common;

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

BasePhysicality::BasePhysicality(const std::string& name) :
  FluxReconstructionSolverCom(name),
  socket_posPrev("posPrev"),
  socket_outputPP("outputPP"),
  m_cellBuilder(CFNULL),
  m_cell(),
  m_cellStates(),
  m_solPntsLocalCoords(),
  m_nbrEqs(),
  m_dim(),
  m_nbrSolPnts(),
  m_maxNbrFlxPnts(),
  m_order(),
  m_solPolyValsAtFlxPnts(CFNULL),
  m_cellStatesFlxPnt(),
  m_nbLimits(),
  m_totalNbLimits(),
  m_nbAvLimits(),
  m_totalNbAvLimits()
{
  addConfigOptionsTo(this);
  
  m_showrate= 1;
  setParameter( "ShowRate", &m_showrate );
}

//////////////////////////////////////////////////////////////////////////////

BasePhysicality::~BasePhysicality()
{
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::configure ( Config::ConfigArgs& args )
{
  FluxReconstructionSolverCom::configure(args);
}

//////////////////////////////////////////////////////////////////////////////

std::vector< Common::SafePtr< BaseDataSocketSink > >
BasePhysicality::needsSockets()
{
  std::vector< Common::SafePtr< BaseDataSocketSink > > result;
  result.push_back(&socket_posPrev);
  return result;
}

//////////////////////////////////////////////////////////////////////////////

std::vector< Common::SafePtr< BaseDataSocketSource > >
BasePhysicality::providesSockets()
{
  std::vector< Common::SafePtr< BaseDataSocketSource > > result;
  result.push_back(&socket_outputPP);
  return result;
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::defineConfigOptions(Config::OptionList& options)
{
  options.addConfigOption< CFuint >("ShowRate","Showrate of the positivity preservation information.");
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::execute()
{
  CFTRACEBEGIN;

  // get the elementTypeData
  SafePtr< vector<ElementTypeData> > elemType = MeshDataStack::getActive()->getElementTypeData();

  // get InnerCells TopologicalRegionSet
  SafePtr<TopologicalRegionSet> cells = MeshDataStack::getActive()->getTrs("InnerCells");

  // get the geodata of the geometric entity builder and set the TRS
  StdTrsGeoBuilder::GeoData& geoData = m_cellBuilder->getDataGE();
  geoData.trs = cells;
  
  // number of element types, should be 1 in non-mixed grids
  const CFuint nbrElemTypes = elemType->size();
  
  const CFuint iter = SubSystemStatusStack::getActive()->getNbIter();
  
  // if there is artificial viscosity, reset the positivity preservation values
  const bool hasArtVisc = getMethodData().hasArtificialViscosity();
  
  if (hasArtVisc) 
  {
    DataHandle< CFreal > posPrev = socket_posPrev.getDataHandle();
    
    posPrev = MathTools::MathConsts::CFrealMax();
  }
  
  // variable to store the number of limits done
  m_nbLimits = 0;
  m_totalNbLimits = 0;
  m_nbAvLimits = 0;
  m_totalNbAvLimits = 0;
  
  // loop over all elements to check physicality and if necessary enforce it
  for (CFuint iElemType = 0; iElemType < nbrElemTypes; ++iElemType)
  {
    // get start and end indexes for this type of element
    const CFuint startIdx = (*elemType)[iElemType].getStartIdx();
    const CFuint endIdx   = (*elemType)[iElemType].getEndIdx  ();

    // loop over cells
    for (CFuint elemIdx = startIdx; elemIdx < endIdx; ++elemIdx)
    {
      // build the GeometricEntity
      geoData.idx = elemIdx;
      m_cell = m_cellBuilder->buildGE();

      // get the states in this cell
      m_cellStates = m_cell->getStates();
      
      // extrapolate states to flux points
      computeFlxPntStates(m_cellStatesFlxPnt);

      // check physicality
      if (!checkPhysicality())
      {
	// enforce physicality
	enforcePhysicality();
	
	// add one to the nb of limits done
	++m_nbLimits;
      }

      //release the GeometricEntity
      m_cellBuilder->releaseGE();
    }
  }
  
  const std::string nsp = this->getMethodData().getNamespace();
  
#ifdef CF_HAVE_MPI
    MPI_Comm comm = PE::GetPE().GetCommunicator(nsp);
    PE::GetPE().setBarrier(nsp);
    const CFuint count = 1;
    MPI_Allreduce(&m_nbLimits, &m_totalNbLimits, count, MPI_UNSIGNED, MPI_SUM, comm);
    MPI_Allreduce(&m_nbAvLimits, &m_totalNbAvLimits, count, MPI_UNSIGNED, MPI_SUM, comm);
#endif
    
  if (PE::GetPE().GetRank(nsp) == 0 && iter%m_showrate == 0) 
  {
    // print number of limits
    CFLog(NOTICE, "Number of times physicality enforced: " << m_totalNbLimits << "\n");
    CFLog(NOTICE, "Number of times average limited: " << m_totalNbAvLimits << "\n");
  }

  PE::GetPE().setBarrier(nsp);
  
  CFTRACEEND;
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::computeFlxPntStates(std::vector< RealVector >& statesFlxPnt)
{
  // loop over flx pnts
  for (CFuint iFlxPnt = 0; iFlxPnt < m_maxNbrFlxPnts; ++iFlxPnt)
  {        
    // reset extrapolate state
    statesFlxPnt[iFlxPnt] = 0.0;

    // extrapolate all states to the current flx pnt
    for (CFuint iSol = 0; iSol < m_nbrSolPnts; ++iSol)
    {
      statesFlxPnt[iFlxPnt] += (*m_solPolyValsAtFlxPnts)[iFlxPnt][iSol]*(*((*m_cellStates)[iSol]));
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::setup()
{
  CFAUTOTRACE;

  // get number of equations and dimensions
  m_nbrEqs = PhysicalModelStack::getActive()->getNbEq();
  m_dim    = PhysicalModelStack::getActive()->getDim ();

  // get cell builder
  m_cellBuilder = getMethodData().getStdTrsGeoBuilder();

  // get the local FR data
  vector< FluxReconstructionElementData* >& frLocalData = getMethodData().getFRLocalData();
  const CFuint nbrElemTypes = frLocalData.size();
  cf_assert(nbrElemTypes > 0);
  
  const CFPolyOrder::Type order = frLocalData[0]->getPolyOrder();
  
  m_order = static_cast<CFuint>(order);
  
  // get nbr of sol pnts
  m_nbrSolPnts = frLocalData[0]->getNbrOfSolPnts();

  // get the maximum number of flux points
  m_maxNbrFlxPnts = 0;
  
  // get the elementTypeData
  SafePtr< vector<ElementTypeData> > elemType = MeshDataStack::getActive()->getElementTypeData();
  
  // this is only necessary for mixed grids and is a bit superfluous for now
  for (CFuint iElemType = 0; iElemType < nbrElemTypes; ++iElemType)
  {
    const CFuint nbrFlxPnts = frLocalData[iElemType]->getNbrOfFlxPnts();
    m_maxNbrFlxPnts = m_maxNbrFlxPnts > nbrFlxPnts ? m_maxNbrFlxPnts : nbrFlxPnts;
  }
  
  // get the coefs for extrapolation of the states to the flx pnts
  m_solPolyValsAtFlxPnts = frLocalData[0]->getCoefSolPolyInFlxPnts();
  
  // initialize extrapolated states
  for (CFuint iFlx = 0; iFlx < m_maxNbrFlxPnts; ++iFlx)
  {
    RealVector temp(m_nbrEqs);
    m_cellStatesFlxPnt.push_back(temp);
  }
  
  // get the number of cells in the mesh
  const CFuint nbrCells = (*elemType)[0].getEndIdx();
  
  // get datahandle
  DataHandle< CFreal > posPrev = socket_posPrev.getDataHandle();
  DataHandle< CFreal > output = socket_outputPP.getDataHandle();
  
  const CFuint nbStates = nbrCells*m_nbrSolPnts;

  // resize socket
  output.resize(nbStates);
  //posPrev.resize(nbrCells);
}

//////////////////////////////////////////////////////////////////////////////

void BasePhysicality::unsetup()
{
  CFAUTOTRACE;
}

//////////////////////////////////////////////////////////////////////////////

  } // namespace FluxReconstructionMethod

} // namespace COOLFluiD
