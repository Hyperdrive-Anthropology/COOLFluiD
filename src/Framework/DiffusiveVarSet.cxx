// Copyright (C) 2012 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "DiffusiveVarSet.hh"
#include "PhysicalModel.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Framework {

//////////////////////////////////////////////////////////////////////////////

DiffusiveVarSet::DiffusiveVarSet
(const std::string& name,
 Common::SafePtr<Framework::PhysicalModelImpl> model) :
  Common::OwnedObject(),
  Config::ConfigObject(name),
  Common::NullableObject(),
  _entityID(0),
  _varNames(),
  _faceCoord(CFNULL),
  _flux(),
  _fluxVec(),
  _iState(0),
  _jState(0),
  _freezeDiffCoeff(false)
{
}

//////////////////////////////////////////////////////////////////////////////

DiffusiveVarSet::~DiffusiveVarSet()
{
}

//////////////////////////////////////////////////////////////////////////////

  } // namespace Framework

} // namespace COOLFluiD

