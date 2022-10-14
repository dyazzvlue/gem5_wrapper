#ifndef __GEM5_WRAPPER_H
#define __GEM5_WRAPPER_H

#include <systemc>
#include <tlm>

#include "sc_slave_port.hh"
#include "sim_control.hh"
#include "slave_transactor.hh"
#include "txn_router.h"
#include "memory.h"
#include "simple_bus.h"

namespace Gem5SystemC{

/**
 * @brief This class is the top class for gem5-tlm co-simulation. It will create
 * a singleleton sim controller and initiate it by config files and create 
 * needed number slave transactors and sockets 
 * 
 */

using Transactor = Gem5SlaveTransactor_Multi;

class Gem5Wrapper
{
    public:
        static Gem5Wrapper* getInstance();
        Gem5Wrapper();
        virtual ~Gem5Wrapper() {};
        
        void createSimControl(sc_core::sc_module_name name,
                            const std::string& configFile,
                            uint64_t simulationEnd,
                            const std::string& gem5DebugFlags);
        Gem5SimControl* getSimControll() { return this->sim_control;}

        void createSingletonTransactor(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num);

        void createTransactor(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num);                        

        TxnRouter* createTxnRouter(uint32_t id, sc_core::sc_module_name name,
                            uint64_t mem_start_addr,
                            uint64_t mem_size,
                            bool debug);

        Transactor* getTransactor(uint32_t transactor_id = 0);

        SimpleBus<2,1>* createSimpleBus(uint64_t mem_start_addr,
                                        uint64_t mem_end_addr);

        void init(); // TODO
        void bindSimControl2Transactor();
        void bindTxnRouter2Transactor(uint32_t txn_id, uint32_t socket_id,
                                    uint32_t transactor_id = 0);
        void bindTxnRouter2Bus(uint32_t txn_id, uint32_t socket_id);
        void bindBus2Memory(); // TODO

    protected:
        static Gem5Wrapper* instance;
        Gem5SimControl* sim_control;
        
        // a vector to store transactors
        std::vector<Transactor*> transactors;
        // a map to store txn_routers
        std::map<uint32_t, TxnRouter*> txn_routers;

        SimpleBus<2,1>* busInstance = nullptr;
        bool isBusCreated = false;



    private:
        // make sure that simControl only be binded once
        bool simControlBinded = false;

};

}


#endif
