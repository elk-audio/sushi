/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI. If not, see http://www.gnu.org/licenses/
*/

#ifndef SUSHI_STANDALONE_FACTORY_H
#define SUSHI_STANDALONE_FACTORY_H

#include "include/sushi/sushi_interface.h"

#include "src/factories/factory_base.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

class StandaloneFactory : public FactoryBase
{
public:
    StandaloneFactory();
    virtual ~StandaloneFactory();

    void run(SushiOptions& options) override;

protected:

};

} // namespace sushi

#endif //SUSHI_STANDALONE_FACTORY_H
