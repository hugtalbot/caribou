#ifndef SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H
#define SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H

#include <sofa/core/objectmodel/BaseObject.h>
#include <sofa/core/behavior/OdeSolver.h>

namespace SofaCaribou::GraphComponents::IO {

using namespace sofa::core::objectmodel;
using namespace sofa::core::behavior;

class SystemExporter : public sofa::core::objectmodel::BaseObject
{
public:
    SOFA_CLASS(SystemExporter, BaseObject);

    template <typename ObjectType>
    using MultiLink = MultiLink<SystemExporter, ObjectType, BaseLink::FLAG_NONE>;

    SystemExporter();

    void init() override;

private:
    /**
     * Links to OdeSolver objects from which the ODE system will be retrieved.
     * If none is provided, this component will search in its current context (ignoring child nodes) for ODE solvers.
     */
    MultiLink<OdeSolver> d_ode_solvers;
};

} // namespace SofaCaribou::GraphComponents::IO
#endif //SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H
