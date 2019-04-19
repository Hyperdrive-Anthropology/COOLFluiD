#include "Framework/State.hh"
#include "Environment/ObjectProvider.hh"

#include "FluxReconstructionMethod/FluxReconstruction.hh"
#include "FluxReconstructionMethod/SetQuadLagrangeP2FluxReconstructionP1StateCoord.hh"

//////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace FluxReconstructionMethod {

//////////////////////////////////////////////////////////////////////////////

Environment::ObjectProvider<SetQuadLagrangeP2FluxReconstructionP1StateCoord,
               Framework::SetElementStateCoord,
               FluxReconstructionModule>
SetQuadLagrangeP2FluxReconstructionP1StateCoord("QuadLagrangeP2FluxReconstructionP1");

//////////////////////////////////////////////////////////////////////////////

void SetQuadLagrangeP2FluxReconstructionP1StateCoord::operator() (const vector<Framework::Node*>& nodes,
                                                          vector<Framework::State*>& states)
{
  cf_assert(states.size() == 4);
  cf_assert(nodes.size() == 9);

  // assign nodes to the states
//  states[0]->setSpaceCoordinates(nodes[0]);
//  states[1]->setSpaceCoordinates(nodes[3]);
//  states[2]->setSpaceCoordinates(nodes[1]);
//  states[3]->setSpaceCoordinates(nodes[2]);
}

//////////////////////////////////////////////////////////////////////////////

void SetQuadLagrangeP2FluxReconstructionP1StateCoord::update(const vector<Framework::Node*>& nodes,
                                    vector<Framework::State*>& states)
{
}

//////////////////////////////////////////////////////////////////////////////

  } // namespace ShapeFunctions

} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////
