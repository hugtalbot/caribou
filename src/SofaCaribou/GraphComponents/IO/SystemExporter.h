#ifndef SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H
#define SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H

#include <sofa/core/objectmodel/BaseObject.h>

namespace SofaCaribou::GraphComponents::IO {

class SystemExporter : public sofa::core::objectmodel::BaseObject
{
public:
    SOFA_CLASS(SystemExporter, BaseObject);

    void init() override;
};

} // namespace SofaCaribou::GraphComponents::IO
#endif //SOFACARIBOU_GRAPHCOMPONENTS_IO_SYSTEMEXPORTER_H
