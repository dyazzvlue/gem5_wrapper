/*
 * Copyright (c) 2016, Dresden University of Technology (TU Dresden)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sc_slave_port.hh"
#include "sim_control.hh"
#include "slave_transactor.hh"

namespace Gem5SystemC
{

Gem5SlaveTransactor::Gem5SlaveTransactor(sc_core::sc_module_name name,
                                         const std::string& portName)
    : sc_core::sc_module(name),
      socket(portName.c_str()),
      sim_control("sim_control"),
      portName(portName)
{
    if (portName.empty()) {
        SC_REPORT_ERROR(name, "No port name specified!\n");
    }
}

void
Gem5SlaveTransactor::before_end_of_elaboration()
{
    auto* port = sim_control->getSlavePort(portName);

    port->bindToTransactor((Gem5SlaveTransactor*)this);
}


Gem5SlaveTransactor_Multi* Gem5SlaveTransactor_Multi::instance = nullptr;

Gem5SlaveTransactor_Multi::Gem5SlaveTransactor_Multi(sc_core::sc_module_name name,
                                         const std::string& portName,
                                         uint32_t socket_num)
    : sc_core::sc_module(name),
      sockets(portName.c_str()),
      sim_control("sim_control"),
      portName(portName),
      socket_num(socket_num)
{
    if (portName.empty()) {
        SC_REPORT_ERROR(name, "No port name specified!\n");
    }
    sockets.init(this->socket_num,
                sc_bind(&Gem5SlaveTransactor_Multi::create_socket, this));
}

void
Gem5SlaveTransactor_Multi::before_end_of_elaboration()
{
    auto* port = sim_control->getSlavePort(portName);
    port->bindToTransactor(this);
}

init_port_type* Gem5SlaveTransactor_Multi::create_socket()
{
    std::string name = getNameForNewSocket(portName);
    std::cout << "create socket: " << name << std::endl;
    init_port_type* socket_p = new init_port_type(name.c_str());
    return socket_p;
}

std::string Gem5SlaveTransactor_Multi::getNameForNewSocket(std::string name)
{
    assert(count < socket_num);
    std::string rename;
    rename += name;
    rename += std::to_string(count);
    count++;
    return rename;
}

Gem5SlaveTransactor_Multi* Gem5SlaveTransactor_Multi::getInstance(sc_core::sc_module_name name,
                        const std::string& portName, uint32_t socket_num)
{
    if (instance == nullptr)
    {
        std::cout << "create new Gem5SlaveTransactor" << std::endl;
        instance = new Gem5SlaveTransactor_Multi(name, portName, socket_num);
    }else{
        std::cout << "Return existed Gem5SlaveTransactor" << std::endl;
    }
    return instance;
}

}
