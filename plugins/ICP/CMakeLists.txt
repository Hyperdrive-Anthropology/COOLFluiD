LIST ( APPEND ICP_files
ICP.hh
ICPInductionConvVarSet.hh
ICPInductionConvVarSet.ci
ICPInductionConvVarSet.cxx
ICPInductionDiffVarSet.hh
ICPInductionDiffVarSet.ci
ICPInductionDiffVarSet.cxx
ICPLTEPvtToCons.hh
ICPLTEPvtToCons.cxx
ICPLTE2DdPuvtToPuvt.hh
ICPLTE2DdPuvtToPuvt.cxx
ICPLTE2DPuvtToConsInPuvt.cxx
ICPLTE2DPuvtToConsInPuvt.hh
ICPLTEPhysicalModel.hh
ICPLTEPhysicalModel.ci
ICPLTEPhysicalModel.cxx
ICPReactionTerm.hh
ICPReactionTerm.ci
)

LIST ( APPEND ICP_requires_mods NavierStokes LTE )
LIST ( APPEND ICP_cflibs Framework NavierStokes ICP LTE )
CF_ADD_PLUGIN_LIBRARY ( ICP )

LIST ( APPEND ICPNEQ_files
ICPNEQ.hh
ICPNEQ2DPvtToPivtTv.hh
ICPNEQ2DPvtToPivtTv.cxx
ICPNEQ2DPuvtToRhoivtTv.hh
ICPNEQ2DPuvtToRhoivtTv.cxx
ICPNEQ2DPivtTvToCons.hh
ICPNEQ2DPivtTvToCons.cxx
ICPNEQ2DPivtTvToRhoivtTv.hh
ICPNEQ2DPivtTvToRhoivtTv.cxx
ICPNEQ2DRhoivtToCons.hh
ICPNEQ2DRhoivtToCons.cxx
ICPNEQ2DRhoivtTvToCons.hh
ICPNEQ2DRhoivtTvToCons.cxx
ICPNEQ2DRhoivtTvToPivtTv.hh
ICPNEQ2DRhoivtTvToPivtTv.cxx
ICPNEQInductionDiffVarSet.cxx
ICPNEQInductionConvVarSet.cxx
ICPNEQPhysicalModel.hh
ICPNEQPhysicalModel.ci
ICPNEQPhysicalModel.cxx
)

LIST ( APPEND ICPNEQ_requires_mods NavierStokes ICP NEQ )
LIST ( APPEND ICPNEQ_cflibs Framework NavierStokes ICP NEQ )
CF_ADD_PLUGIN_LIBRARY ( ICPNEQ )

CF_WARN_ORPHAN_FILES()
ADD_SUBDIRECTORY ( testcases )
