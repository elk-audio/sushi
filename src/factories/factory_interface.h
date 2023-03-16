#ifndef SUSHI_FACTORY_INTERFACE_H
#define SUSHI_FACTORY_INTERFACE_H

#include "include/sushi/sushi.h"

namespace sushi {

class FactoryInterface
{
public:
    FactoryInterface() = default;
    virtual ~FactoryInterface() = default;

    /**
     * @brief Run this - once - to construct sushi. If construction fails,
     *        fetch InitStatus using sushi_init_status() to find out why.
     * @param options A populated SushiOptions structure.
     *        Not that it is passed in by reference - factories may choose to modify it.
     * @return A pair with: unique_ptr with the constructed instance if successful, of empty if not.
     *         And, the status of the Initialization carried out by run().
     */
    virtual std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) = 0;
};

} // namespace sushi

#endif // SUSHI_FACTORY_INTERFACE_H
