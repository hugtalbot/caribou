#include "SystemExporter.h"

#include <sofa/core/ObjectFactory.h>

namespace SofaCaribou::GraphComponents::IO {

SystemExporter::SystemExporter()
: d_ode_solvers(initLink("ode_solvers", "Links to OdeSolver objects from which the ODE system will be retrieved. If none"
                                        " is provided, this component will search in its current context (ignoring child"
                                        " nodes) for ODE solvers."))
{

};

void SystemExporter::init ()
{
    
}

SOFA_DECL_CLASS(SystemExporter)

static int SystemExporterClass = sofa::core::RegisterObject("Caribou stiffness matrix exporter")
        .add< SofaCaribou::GraphComponents::IO::SystemExporter > (true)
;

} // namespace SofaCaribou::GraphComponents::IO