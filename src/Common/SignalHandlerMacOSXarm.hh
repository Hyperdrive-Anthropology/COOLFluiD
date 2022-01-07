// Copyright (C) 2012 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef COOLFluiD_Common_SignalHandlerMacOSXarm_hh
#define COOLFluiD_Common_SignalHandlerMacOSXarm_hh

//////////////////////////////////////////////////////////////////////////////

#include "Common/SignalHandler.hh"

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Common {

//////////////////////////////////////////////////////////////////////////////

/// This class handles of signals from the MacOSX operating system
/// @author Tiago Quintino
class Common_API SignalHandlerMacOSXarm : public SignalHandler {

public: // methods

  /// Constructor
  SignalHandlerMacOSXarm();

  /// Default destructor
  virtual ~SignalHandlerMacOSXarm();

  /// Regists the signal handlers that will be handled by this class
  virtual void registSignalHandlers();

protected:
  
  /// SIGFPE signal handler
  static int handleSIGFPE(int signal);

  /// SIGSEGV signal handler
  static int handleSIGSEGV(int signal);

}; // end of class SignalHandlerMacOSXarm

//////////////////////////////////////////////////////////////////////////////

  } // namespace Common

} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////

#endif // COOFluiD_Common_SignalHandlerMacOSXarm_hh
