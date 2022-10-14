#include "gem5_wrapper.hh"
#include "assert.h"

namespace Gem5SystemC
{
    Gem5Wrapper* Gem5Wrapper::instance = nullptr;

    Gem5Wrapper* Gem5Wrapper::getInstance()
    {
        if (instance == nullptr){
            instance = new Gem5Wrapper();
        }
        return instance;
     }
    // Constructor
    Gem5Wrapper::Gem5Wrapper(): sim_control(nullptr){};

    void Gem5Wrapper::createSimControl(sc_core::sc_module_name name,
                            const std::string& configFile,
                            uint64_t simulationEnd,
                            const std::string& gem5DebugFlags)
    {
        if (this->sim_control != nullptr){
            // already created
            return;
        }
        this->sim_control = Gem5SimControl::getInstance(name, configFile,
                                                simulationEnd, gem5DebugFlags);
    }

    void Gem5Wrapper::createSingletonTransactor(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num)
    {
        Transactor* transactor = 
            Transactor::getInstance(name, portName, socket_num);
        transactors.push_back(transactor);

    }

    void Gem5Wrapper::createTransactor(sc_core::sc_module_name name,
                        const std::string& portName,
                        uint32_t socket_num)
    {
        Transactor* transactor = new Transactor(name, portName, socket_num);
        transactors.push_back(transactor);
    }

    Transactor* Gem5Wrapper::getTransactor(uint32_t transactor_id )
    {
        assert(transactor_id < transactors.size());
        return transactors[transactor_id];
    }

    TxnRouter* Gem5Wrapper::createTxnRouter(uint32_t id, sc_core::sc_module_name name,
                            uint64_t mem_start_addr,
                            uint64_t mem_size,
                            bool debug)
    {
        TxnRouter* txn_router = new TxnRouter(name, mem_start_addr, 
                                    mem_size,debug);
        txn_routers.insert(std::make_pair(id, txn_router));
        return txn_router;
    }

    SimpleBus<2,1>* Gem5Wrapper::
    createSimpleBus(uint64_t mem_start_addr, uint64_t mem_end_addr)
    {
        if (isBusCreated == true && busInstance != nullptr){
            return busInstance;
        }else{
            auto bus = new SimpleBus<2,1>("simplebus");
            bus->ports[0] = new PortMapping(mem_start_addr, mem_end_addr);
            busInstance = bus;
            isBusCreated  = true;
            return bus;
        }
    }

    void Gem5Wrapper::bindSimControl2Transactor()
    {
        if (this->sim_control == nullptr){
            std::cerr << "Need init sim control first " << std::endl;
            return;
        }
        if (this->simControlBinded == true){
            std::cerr << "No need to bind sim control again , skip " << std::endl;
            return;
        }
        for (auto it = transactors.begin(); it != transactors.end(); it++){
            (*it)->sim_control.bind(*sim_control);
        }
        this->simControlBinded = true;
    }

    void Gem5Wrapper::
    bindTxnRouter2Transactor(uint32_t txn_id, uint32_t socket_id, 
            uint32_t transactor_id)
    {
        assert(transactor_id < transactors.size());
        auto transactor = transactors[transactor_id];
        assert(transactor != nullptr);
        assert(socket_id < transactor->getSocketNum());
        if (txn_routers.find(txn_id) != txn_routers.end()){
            transactor->sockets[socket_id].bind(txn_routers.at(txn_id)->tsock);
        }
        
    }

    void Gem5Wrapper::bindTxnRouter2Bus(uint32_t txn_id, uint32_t socket_id)
    {
        assert(busInstance != nullptr);
        if (txn_routers.find(txn_id) != txn_routers.end()){
            txn_routers.at(txn_id)->isock_bus(busInstance->tsocks[socket_id]);
        }   
    }
    
    void Gem5Wrapper::bindBus2Memory()
    {
        
    }
}

