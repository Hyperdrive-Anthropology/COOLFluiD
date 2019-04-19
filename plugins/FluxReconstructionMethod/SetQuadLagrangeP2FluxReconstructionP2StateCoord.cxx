#include "Framework/State.hh"
#include "Environment/ObjectProvider.hh"

#include "FluxReconstructionMethod/FluxReconstruction.hh"
#include "FluxReconstructionMethod/SetQuadLagrangeP2FluxReconstructionP2StateCoord.hh"

//////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

Environment::ObjectProvider<SetQuadLagrangeP2FluxReconstructionP2StateCoord,
               Framework::SetElementStateCoord,
               FluxReconstructionModule>
SetQuadLagrangeP2FluxReconstructionP2StateCoord("QuadLagrangeP2FluxReconstructionP2");

//////////////////////////////////////////////////////////////////////////////

void SetQuadLagrangeP2FluxReconstructionP2StateCoord::operator() (const vector<Framework::Node*>& nodes,
                                                          vector<Framework::State*>& states)
{
  cf_assert(states.size() == 9);
  cf_assert(nodes.size() == 9);

//  states[0]->setSpaceCoordinates(nodes[0]);
//  states[6]->setSpaceCoordinates(nodes[1]);
//  states[8]->setSpaceCoordinates(nodes[2]);
//  states[2]->setSpaceCoordinates(nodes[3]);
//
//  states[3]->setSpaceCoordinates(nodes[4]);
//  states[7]->setSpaceCoordinates(nodes[5]);
//  states[5]->setSpaceCoordinates(nodes[6]);
//  states[1]->setSpaceCoordinates(nodes[7]);
//
//  states[4]->setSpaceCoordinates(nodes[8]);
}

//////////////////////////////////////////////////////////////////////////////

void SetQuadLagrangeP2FluxReconstructionP2StateCoord::update(const vector<Framework::Node*>& nodes,
                                                     vector<Framework::State*>& states)
{
}

//////////////////////////////////////////////////////////////////////////////

  } // namespace ShapeFunctions

} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////
