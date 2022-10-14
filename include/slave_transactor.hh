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

#ifndef __GEM5_SLAVE_TRANSACTOR_HH__
#define __GEM5_SLAVE_TRANSACTOR_HH__

#include <tlm_utils/simple_initiator_socket.h>

#include <map>
#include <systemc>
#include <tlm>

#include "sc_slave_port.hh"
#include "sim_control_if.hh"

namespace Gem5SystemC
{

typedef tlm_utils::simple_initiator_socket<SCSlavePort> init_port_type;

class Gem5SlaveTransactor : public sc_core::sc_module
{
  public:
    // module interface

    // basic method
    tlm_utils::simple_initiator_socket<SCSlavePort> socket;
    sc_core::sc_port<Gem5SimControlInterface> sim_control;

  private:
    std::string portName;

  public:
    SC_HAS_PROCESS(Gem5SlaveTransactor);

    Gem5SlaveTransactor(sc_core::sc_module_name name,
                        const std::string& portName);

    void before_end_of_elaboration();
};

class Gem5SlaveTransactor_Multi : public sc_core::sc_module
{
  public:
    // module interface

    /**
     * @brief vector of SCSlavaPort sockets
     * The first socket is used to accept requests from gem5 system port.
     * (write back; interrupt; functional)
     *
     */
    sc_core::sc_vector<init_port_type> sockets;
    // basic method
    sc_core::sc_port<Gem5SimControlInterface> sim_control;

    // TODO: Rename
    std::map<uint32_t, std::vector<int>> socket_core_map;

  private:
    std::string portName;
    uint32_t socket_num; // as same as core num
    uint32_t count = 0 ; // used for generate socket name
    std::string getNameForNewSocket(std::string name);

  protected:
    static Gem5SlaveTransactor_Multi* instance;

  public:
    SC_HAS_PROCESS(Gem5SlaveTransactor);

    /*
     * Constructor
     * This class has a public constructor. This class can be set as a 
     * singleton if needed.
     */ 
    Gem5SlaveTransactor_Multi(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num);

    void before_end_of_elaboration();
    init_port_type* create_socket();
    uint32_t getSocketNum() {return socket_num;}
    bool isUsingGem5Cache () {return false;}
    void setSocketCoreMap(std::map<uint32_t, std::vector<int>> map){
        this->socket_core_map = map;
    }
    std::map<uint32_t, std::vector<int>> getSocketCoreMap()
        { return this->socket_core_map;}

    static Gem5SlaveTransactor_Multi* getInstance(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num);

};

}

#endif
