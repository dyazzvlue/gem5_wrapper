/**
 * @file txn_router.h
 * @author cw (wei.c.chen@intel.com)
 * @brief
 * @version 0.1
 * @date 2022-04-08
 *
 * Copyright (C) 2020, Intel Corporation. All rights reserved.
 *
 * A txn_router on one side is connected to the memory interface of a core,
 * on the other side connected to a CHI RN-F and a TLM bus. It accepts
 * incoming requests and route them to the downstream interfaces by telling
 * whether one is a memory transaction or not. Non-memory transactions can be
 * DMA register configurations, CLINT events and so on. It helps to integrate
 * CHI modules without messing the VP itself and the CHI library.
 */

#ifndef TXN_ROUTER_H
#define TXN_ROUTER_H

#include <iostream>
#include <iomanip>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <systemc>

using namespace sc_core;
using namespace sc_dt;
namespace Gem5SystemC
{

class TxnRouter : public sc_core::sc_module
{
public:
    TxnRouter(sc_core::sc_module_name,
              uint64_t mem_start_addr_,
              uint64_t mem_size_,
              bool debug_)
        : mem_start_addr(mem_start_addr_),
        mem_size(mem_size_),
        debug(debug_),
        m_peq(this, &TxnRouter::peq_cb),
        transaction_in_progress(0),
        response_in_progress(false),
        next_response_pending(0),
        end_req_pending(0)
        {
        tsock.register_b_transport(this, &TxnRouter::b_transport);
        tsock.register_transport_dbg(this, &TxnRouter::transport_dbg);
        tsock.register_nb_transport_fw(this, &TxnRouter::nb_transport_fw);
        isock_mem.register_nb_transport_bw(this, &TxnRouter::nb_transport_bw_resp);

        SC_THREAD(execute_transaction_process);

    }
    SC_HAS_PROCESS(TxnRouter);

private:
    void b_transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay)
    {
        // receive Atomic request from gem5 world
        if (debug) {
            SC_REPORT_INFO(this->name(), "b_transport ");
        }

        if (ToMem(trans)) {
            if (debug){
                SC_REPORT_INFO(this->name(), "receive mem request");
            }
            isock_mem->b_transport(trans, delay);
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
            //execute_transaction(trans);
        }
        else {
            if (debug) {
               SC_REPORT_INFO(this->name(), "receive to mem request ");
            }
            isock_bus->b_transport(trans, delay);
        }
    }

    unsigned int transport_dbg(tlm::tlm_generic_payload &trans)
    {
        // recvFunctional request from gem5 world
        // used for loading the binary (recvFunctional)
        // send to the memory directly
        if (debug){
            //SC_REPORT_INFO(this->name(), "transport_dbg");
        }
        if (ToMem(trans)) {
            if (debug) {
                //SC_REPORT_INFO(this->name(), "receive Functional request ");
            }
            // Using SimpleBus to access memory
            return isock_bus->transport_dbg(trans);
        }
        else {
            SC_REPORT_FATAL("TXN_ROUTER", "Address out of range. Please check");
        }
        return 0;
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                tlm::tlm_phase& phase,
                sc_time& delay)
    {
        // receive TimingReq from gem5 world
        if (debug) {
            SC_REPORT_INFO(this->name(), "nb_transport_fw ");
            std::cout<<  "Addr : " << std::setw(8) << std::hex
                << trans.get_address() 
                << " data_ptr: " << trans.get_data_ptr() 
                << std::endl;
        }
        if (ToMem(trans)) {
            if (debug) {
                SC_REPORT_INFO(this->name(), " receive to mem request ");
                std::cout << "Addr : " << std::setw(8) << std::hex
                    << trans.get_address() << std::endl;
            }
            m_peq.notify(trans, phase, delay);
        }
        else {
            SC_REPORT_FATAL(this->name(), "Address out of range. Please check");
        }
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_bw_resp(tlm::tlm_generic_payload& trans,
                tlm::tlm_phase& phase,
                sc_time& delay)
    {
        if (debug){
             SC_REPORT_INFO(this->name(), "nb_transport_bw_resp ");
        }
        return tlm::TLM_ACCEPTED;
    }

    // same as check_address in sc_target
    bool ToMem(tlm::tlm_generic_payload& trans) {
        auto addr = trans.get_address();
        if (addr < mem_start_addr)
            return false;
        if (addr >= mem_start_addr + mem_size)
            return false;
        return true;
    }

    /* Helping functions and processes */
    void send_response(tlm::tlm_generic_payload &trans)
    {
        // send response when receive the result from memory

        tlm::tlm_sync_enum status;
        tlm::tlm_phase bw_phase;
        sc_time delay;

        response_in_progress = true;
        bw_phase = tlm::BEGIN_RESP;
        delay = sc_time(10.0, SC_NS);
        if (debug){
           std::cout << sc_time_stamp() << " " << this->name()
                << " send response addr: " << std::setw(8) << std::hex
                << trans.get_address() << std::endl;
           std::cout << "data_ptr: " << trans.get_data_ptr() << std::endl;
        }
        status = tsock->nb_transport_bw( trans, bw_phase, delay );

        if (status == tlm::TLM_UPDATED) {
            /* The timing annotation must be honored */
            m_peq.notify(trans, bw_phase, delay);
        } else if (status == tlm::TLM_COMPLETED) {
            /* The initiator has terminated the transaction */
            transaction_in_progress = 0;
            response_in_progress = false;
        }
        trans.release();
    }

    /* Callback of Payload Event Queue */
    void peq_cb(tlm::tlm_generic_payload& trans,
                const tlm::tlm_phase& phase)
    {
        sc_time delay;

        if (phase == tlm::BEGIN_REQ) {
            if (debug) {
                SC_REPORT_INFO(this->name(), " Begin request");
                std::cout << "Addr : " << std::setw(8) << std::hex <<
                    trans.get_address() << std::endl;
            }
            trans.acquire();
            if (!transaction_in_progress) {
                send_end_req(trans);
            }else{
                /* On receiving END_RESP, the target can release the transaction and
                * allow other pending transactions to proceed */
                end_req_pending = &trans;
            }
        }else if (phase == tlm::END_RESP) {
            /* On receiving END_RESP, the target can release the transaction and
            * allow other pending transactions to proceed */
            if (debug) {
                SC_REPORT_INFO(this->name(), "end response");
                std::cout << "Addr : " << std::setw(8) << std::hex <<
                    trans.get_address() << std::endl;
                if (trans.get_data_ptr()){
                    std::cout << "data_ptr : " << trans.get_data_ptr() << std::endl;
                }
                
            }
            if (!response_in_progress){
                SC_REPORT_FATAL(this->name(), "Illegal transaction phase END RESP");
            }

            transaction_in_progress = 0;

            // ready to issue the next BEGIN_RESP
            response_in_progress = false;
            if (next_response_pending) {
                send_response( *next_response_pending );
                next_response_pending = 0;
            }

            /* ... and to unblock the initiator by issuing END_REQ */
            if (end_req_pending) {
                if (debug){
                    std::cout << sc_time_stamp()
                        << this->name()
                        << " end_req_pending addr: "
                        << std::setw(8) << std::hex
                        << end_req_pending->get_address()
                        << std::endl;
                }
                send_end_req( *end_req_pending );
                end_req_pending = 0;
            }
        } else  //tlm::END_REQ or tlm::BEGIN_RESP
        {
            SC_REPORT_FATAL(this->name() , "Illegal transaction phase");
        }
    }

    void send_end_req(tlm::tlm_generic_payload& trans)
    {
        if (debug) {
            SC_REPORT_INFO(this->name(), "send_end_req");
            std::cout << "Addr : " << std::setw(8) << std::hex
                << trans.get_address() << std::endl;
        }
        tlm::tlm_phase bw_phase;
        sc_time delay;

        bw_phase = tlm::END_REQ;
        delay = sc_time(10.0, SC_NS);

        tlm::tlm_sync_enum status;
        status = tsock->nb_transport_bw(trans, bw_phase, delay);
        if (debug) {
            std::cout <<  sc_time_stamp() << " " << this->name()
                << " send_end_req nb_transport_bw "
                <<" addr: " << std::setw(8) << std::hex
                << trans.get_address() << std::endl;
        }
        delay = delay + sc_time(15.0, SC_NS); // latency
        target_done_event.notify(delay);

        assert(transaction_in_progress == 0);
        transaction_in_progress = &trans;

    }

    void execute_transaction_process(){
        while (true){
            wait(target_done_event);
            if (debug) {
                SC_REPORT_INFO(this->name(), "execute transaction process");
                std::cout << std::setw(8) << std::hex
                    << "Addr : " << transaction_in_progress->get_address()
                    << std::endl;
            }
            // Execute the read or write commands
            // In this case , forward to next IP by isock_mem;
            execute_transaction(*transaction_in_progress); // TODO

            if (response_in_progress)
            {
                /* Target allows only two transactions in-flight */
                if (next_response_pending)
                {
                    SC_REPORT_FATAL("TLM-2", "Attempt to have two pending "
                                "responses in target");
                }
                next_response_pending = transaction_in_progress;
            }
            else
            {
                send_response(*transaction_in_progress);  // TODO
            }
        }
    }

    void execute_transaction(tlm::tlm_generic_payload& trans)
    {
        // Forward the transaction to next component
        if (debug) {
            SC_REPORT_INFO(this->name(), " execute transaction");
            std::cout << std::setw(8) << std::hex << "Addr : "
                << trans.get_address() << std::endl;
        }
        sc_time delay;
        delay = sc_time(10.0, SC_NS);
        isock_mem->b_transport(trans, delay);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }


public:
    // tsock <-> cpu_side_port (gem5 slave transactor)
    tlm_utils::simple_target_socket<TxnRouter> tsock;
    tlm_utils::simple_initiator_socket<TxnRouter> isock_mem;
    tlm_utils::simple_initiator_socket<TxnRouter> isock_bus;

    tlm_utils::peq_with_cb_and_phase<TxnRouter> m_peq;
    tlm::tlm_generic_payload*  transaction_in_progress;
    sc_event                   target_done_event;
    bool                       response_in_progress;
    tlm::tlm_generic_payload*  next_response_pending;
    tlm::tlm_generic_payload*  end_req_pending;

private:
    uint64_t mem_start_addr;
    uint64_t mem_size;
    bool debug;

};
} // namespace Gem5SystemC

#endif
