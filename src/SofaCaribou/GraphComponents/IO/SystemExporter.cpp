#include "SystemExporter.h"

#include <sofa/core/ObjectFactory.h>

namespace SofaCaribou::GraphComponents::IO {

void SystemExporter::init ()
{
    
}

SOFA_DECL_CLASS(SystemExporter)

static int SystemExporterClass = sofa::core::RegisterObject("Caribou stiffness matrix exporter")
        .add< SofaCaribou::GraphComponents::IO::SystemExporter > (true)
;

} // namespace SofaCaribou::GraphComponents::IO