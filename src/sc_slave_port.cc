/*
 * Copyright (c) 2015, University of Kaiserslautern
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

#include "blocking_packet_helper.hh"
#include "sc_ext.hh"
#include "sc_mm.hh"
#include "sc_slave_port.hh"
#include "slave_transactor.hh"
// This head file is used to set chiattr in Neutra Pkt
// #include "common/chi-utils.h" 

namespace Gem5SystemC
{

/**
 * Instantiate a tlm memory manager that takes care about all the
 * tlm transactions in the system
 */
MemoryManager mm;

/**
 * Convert a gem5 packet to a TLM payload by copying all the relevant
 * information to a previously allocated tlm payload
 */
void
packet2payload(gem5::PacketPtr packet, tlm::tlm_generic_payload &trans)
{
    trans.set_address(packet->getAddr());
    /* Check if this transaction was allocated by mm */
    sc_assert(trans.has_mm());

    uint32_t size = packet->getSize();
    unsigned char *data = packet->getPtr<unsigned char>();

    trans.set_data_length(size);
    trans.set_streaming_width(size);
    trans.set_data_ptr(data);

    if (packet->isRead()) {
        trans.set_command(tlm::TLM_READ_COMMAND);
    }
    else if (packet->isInvalidate()) {
        /* Do nothing */
    } else if (packet->isWrite()) {
        trans.set_command(tlm::TLM_WRITE_COMMAND);
    } else {
        SC_REPORT_FATAL("SCSlavePort", "No R/W packet");
    }
}

/**
 * Similar to TLM's blocking transport (LT)
 */
gem5::Tick
SCSlavePort::recvAtomic(gem5::PacketPtr packet)
{
    CAUGHT_UP;
    SC_REPORT_INFO("SCSlavePort", "recvAtomic hasn't been tested much");

    panic_if(packet->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(packet->isRead() || packet->isWrite()),
             "Should only see read and writes at TLM memory\n");

    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

    /* Prepare the transaction */
    tlm::tlm_generic_payload * trans = mm.allocate();
    trans->acquire();
    packet2payload(packet, *trans);

    /* Attach the packet pointer to the TLM transaction to keep track */
    Gem5Extension* extension = new Gem5Extension(packet);
    uint32_t socket_id = this->getSocketId(packet->requestorId());
    extension->setCoreID(socket_id);
    trans->set_auto_extension(extension);

    /* Execute b_transport: */
    if (packet->cmd == gem5::MemCmd::SwapReq) {
        SC_REPORT_FATAL("SCSlavePort", "SwapReq not supported");
    } else if (packet->isRead()) {
        if (transactor != nullptr) {
            transactor->socket->b_transport(*trans, delay);
        } else if (transactor_multi != nullptr) {
            transactor_multi->sockets[socket_id]->b_transport(*trans, delay);
        } else{
            // no transactor , Exit
            SC_REPORT_FATAL("SCSlavePort", "No binded transactor, please check");
        }
    } else if (packet->isInvalidate()) {
        // do nothing
    } else if (packet->isWrite()) {
        /*  
          For Neutra work, set up chi attr and opcode 
          // TODO: Setting opcode more general
        */
        /*
        auto chiattr = new chiattr_extension;
        chiattr->SetOpcode(27);
        trans->set_extension(chiattr);
        */
        if (transactor != nullptr) {
            transactor->socket->b_transport(*trans, delay);
        } else if (transactor_multi != nullptr) {
            transactor_multi->sockets[socket_id]->b_transport(*trans, delay);
        } else {
            // no transactor , Exit
            SC_REPORT_FATAL("SCSlavePort", "No binded transactor, please check");
        }
    } else {
        SC_REPORT_FATAL("SCSlavePort", "Typo of request not supported");
    }

    if (packet->needsResponse()) {
        packet->makeResponse();
    }

    trans->release();

    return delay.value();
}

/**
 * Similar to TLM's debug transport
 */
void
SCSlavePort::recvFunctional(gem5::PacketPtr packet)
{
    /* Prepare the transaction */
    tlm::tlm_generic_payload * trans = mm.allocate();
    trans->acquire();
    packet2payload(packet, *trans);

    /* Attach the packet pointer to the TLM transaction to keep track */
    Gem5Extension* extension = new Gem5Extension(packet);
    uint32_t socket_id = this->getSocketId(packet->requestorId());
    extension->setCoreID(socket_id);
    trans->set_auto_extension(extension);

    /* Execute Debug Transport: */
    uint32_t bytes;
    if (transactor != nullptr) {
        bytes = transactor->socket->transport_dbg(*trans);
    } else if (transactor_multi != nullptr) {
        bytes = transactor_multi->sockets[socket_id]->transport_dbg(*trans);
    } else {
        SC_REPORT_FATAL("SCSlavePort", "No binded transactor, please check");
    }
    if (bytes != trans->get_data_length()) {
        SC_REPORT_FATAL("SCSlavePort","debug transport was not completed");
    }

    trans->release();
}

bool
SCSlavePort::recvTimingSnoopResp(gem5::PacketPtr packet)
{
    /* Snooping should be implemented with tlm_dbg_transport */
    SC_REPORT_FATAL("SCSlavePort","unimplemented func.: recvTimingSnoopResp");
    return false;
}

void
SCSlavePort::recvFunctionalSnoop(gem5::PacketPtr packet)
{
    /* Snooping should be implemented with tlm_dbg_transport */
    SC_REPORT_FATAL("SCSlavePort","unimplemented func.: recvFunctionalSnoop");
}

/**
 *  Similar to TLM's non-blocking transport (AT)
 */
bool
SCSlavePort::recvTimingReq(gem5::PacketPtr packet)
{
    CAUGHT_UP;
    panic_if(packet->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(packet->isRead() || packet->isWrite()),
             "Should only see read and writes at TLM memory\n");

    /* We should never get a second request after noting that a retry is
     * required */
    sc_assert(!needToSendRequestRetry);
    // get target socket id
    uint32_t socket_id = this->getSocketId(packet->requestorId()); 

    if (packet->hasCpuClusterId()) {
        if (packet->cpuClusterId() != socket_id){
            socket_id = packet->cpuClusterId();
        }
    }
    
    /* Remember if a request comes in while we're blocked so that a retry
     * can be sent to gem5 */

    // only request from system port will be set to blocking request
    if (blockingRequest && usingGem5Cache) {
        needToSendRequestRetry = true;
        return false;
    }
    // packet from core model will be set to blk_pkt_helper
    if (blk_pkt_helper->isBlockedPort(socket_id, pktType::Request)) {
        blk_pkt_helper->updateRetryMap(socket_id, true);
        return false;
    }

    /*  NOTE: normal tlm is blocking here. But in our case we return false
     *  and tell gem5 when a retry can be done. This is the main difference
     *  in the protocol:
     *  if (requestInProgress)
     *  {
     *      wait(endRequestEvent);
     *  }
     *  requestInProgress = trans;
    */

    /* Prepare the transaction */
    tlm::tlm_generic_payload * trans = mm.allocate();
    trans->acquire();
    packet2payload(packet, *trans);

    /* Attach the packet pointer to the TLM transaction to keep track */
    Gem5Extension* extension = new Gem5Extension(packet);

    extension->setCoreID(socket_id);
    trans->set_auto_extension(extension);

    if (trans->is_write()){
        /*
          For Neutra work, set up chi attr and opcode
          // TODO change to a general method
        */
        /*
        auto chiattr = new chiattr_extension;
        chiattr->SetOpcode(27);
        trans->set_extension(chiattr);
        */
    }

    /*
     * Pay for annotated transport delays.
     *
     * The header delay marks the point in time, when the packet first is seen
     * by the transactor. This is the point int time, when the transactor needs
     * to send the BEGIN_REQ to the SystemC world.
     *
     * NOTE: We drop the payload delay here. Normally, the receiver would be
     *       responsible for handling the payload delay. In this case, however,
     *       the receiver is a SystemC module and has no notion of the gem5
     *       transport protocol and we cannot simply forward the
     *       payload delay to the receiving module. Instead, we expect the
     *       receiving SystemC module to model the payload delay by deferring
     *       the END_REQ. This could lead to incorrect delays, if the XBar
     *       payload delay is longer than the time the receiver needs to accept
     *       the request (time between BEGIN_REQ and END_REQ).
     *
     * TODO: We could detect the case described above by remembering the
     *       payload delay and comparing it to the time between BEGIN_REQ and
     *       END_REQ. Then, a warning should be printed.
     */
    auto delay = sc_core::sc_time::from_value(packet->payloadDelay);
    // reset the delays
    packet->payloadDelay = 0;
    packet->headerDelay = 0;

    /* Starting TLM non-blocking sequence (AT) Refer to IEEE1666-2011 SystemC
     * Standard Page 507 for a visualisation of the procedure */
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    tlm::tlm_sync_enum status;

    if (transactor != nullptr){
        status = transactor->socket->nb_transport_fw(*trans, phase, delay);
    } else if (transactor_multi != nullptr) {
        status = transactor_multi->sockets[socket_id]->nb_transport_fw(*trans,
                                                                phase, delay);
    }else {
        SC_REPORT_FATAL("SCSlavePort", "No binded transactor, please check");
    }
    /* Check returned value: */
    if (status == tlm::TLM_ACCEPTED) {
        sc_assert(phase == tlm::BEGIN_REQ);
        /* Accepted but is now blocking until END_REQ (exclusion rule)*/
        if (socket_id == 0 && usingGem5Cache){ // transaction from system port
            blockingRequest = trans;
        }else {
            blk_pkt_helper->updateBlockingMap(socket_id, trans,
                                                    pktType::Request);
        }
    } else if (status == tlm::TLM_UPDATED) {
        /* The Timing annotation must be honored: */
        sc_assert(phase == tlm::END_REQ || phase == tlm::BEGIN_RESP);
        PayloadEvent<SCSlavePort> * pe;
        pe = new PayloadEvent<SCSlavePort>(*this,
            &SCSlavePort::pec, "PEQ");
        pe->notify(*trans, phase, delay);
    } else if (status == tlm::TLM_COMPLETED) {
        /* Transaction is over nothing has do be done. */
        sc_assert(phase == tlm::END_RESP);
        trans->release();
    }

    return true;
}

void
SCSlavePort::pec(
    PayloadEvent<SCSlavePort> * pe,
    tlm::tlm_generic_payload& trans,
    const tlm::tlm_phase& phase)
{
    sc_time delay;

    if (phase == tlm::END_REQ ||
              (blk_pkt_helper->isBlockingTrans(&trans, pktType::Request)
              && phase == tlm::BEGIN_RESP) ||
              (&trans == blockingRequest && phase == tlm::BEGIN_RESP)
              ) {
        // system port is blocked, send retry
        if (&trans == blockingRequest && usingGem5Cache){
            sc_assert(&trans == blockingRequest);
            blockingRequest = NULL;
            if (needToSendRequestRetry) {
                needToSendRequestRetry = false;
                sendRetryReq();
            }
        } else // cpu port is blocked, send retry
        {
            sc_assert(blk_pkt_helper->isBlockingTrans(&trans,
                                                            pktType::Request));
            uint32_t core_id =
                                Gem5Extension::getExtension(trans).getCoreID();
            blk_pkt_helper->updateBlockingMap(core_id, NULL,
                                                    pktType::Request);
            /* Did another request arrive while blocked, schedule a retry */
            if (blk_pkt_helper->needToSendRequestRetry(core_id)){
                blk_pkt_helper->updateRetryMap(core_id, false);
                sendRetryReq();
            }
        }
    }
    if (phase == tlm::BEGIN_RESP)
    {
        CAUGHT_UP;

        auto& extension = Gem5Extension::getExtension(trans);
        auto packet = extension.getPacket();
        uint32_t core_id = extension.getCoreID();
        if (core_id == 0 && usingGem5Cache){
            sc_assert(!blockingResponse);
        }else {
            sc_assert(!blk_pkt_helper->getBlockingTrans(core_id,
                                                            pktType::Response));
        }

        bool need_retry = false;

        // If there is another gem5 model under the receiver side, and already
        // make a response packet back, we can simply send it back. Otherwise,
        // we make a response packet before sending it back to the initiator
        // side gem5 module.
        if (packet->needsResponse()) {
            packet->makeResponse();
        }
        if (packet->isResponse()) {
            need_retry = !sendTimingResp(packet);
        }

        if (need_retry) {
            if (core_id == 0 && usingGem5Cache){
            //if (core_id == 0){
                blockingResponse = &trans;
            }else {
                blk_pkt_helper->updateBlockingMap(core_id, &trans,
                                                        pktType::Response);
            }
        } else {
            if (phase == tlm::BEGIN_RESP) {
                /* Send END_RESP and we're finished: */
                tlm::tlm_phase fw_phase = tlm::END_RESP;
                sc_time delay = SC_ZERO_TIME;
                if (transactor != nullptr) {
                    transactor->socket->nb_transport_fw(trans, fw_phase, delay);
                } else if (transactor_multi != nullptr) {
                    transactor_multi->sockets[core_id]->nb_transport_fw(trans,
                                                            fw_phase, delay);
                } else {
                    SC_REPORT_FATAL("SCSlavePort",
                                    "No binded transactor, please check");
                }
                /* Release the transaction with all the extensions */
                trans.release();
            }
        }
    }
    delete pe;
}

void
SCSlavePort::recvRespRetry()
{
    CAUGHT_UP;

    /* Retry a response */
    //sc_assert(blockingResponse);
    auto response = blk_pkt_helper->getBlockingResponse();
    while (response != nullptr || blockingResponse != NULL) {
        tlm::tlm_generic_payload *trans;
        gem5::PacketPtr packet;
        uint32_t core_id;
        if (blockingResponse != NULL){
            trans = blockingResponse;
            blockingResponse = NULL;
            packet = Gem5Extension::getExtension(trans).getPacket();
            core_id = Gem5Extension::getExtension(trans).getCoreID();
        }else if (response != nullptr){
            trans = response;
            packet = Gem5Extension::getExtension(trans).getPacket();
            core_id = Gem5Extension::getExtension(trans).getCoreID();
            blk_pkt_helper->updateBlockingMap(core_id, NULL,
                                                    pktType::Response);
        }

        bool need_retry = !sendTimingResp(packet);

        sc_assert(!need_retry);

        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        tlm::tlm_phase phase = tlm::END_RESP;
        if (transactor != nullptr ){
            transactor->socket->nb_transport_fw(*trans, phase, delay);
        } else if (transactor_multi != nullptr) {
            transactor_multi->sockets[core_id]->nb_transport_fw(*trans,
                                                                phase, delay);
        } else {
            SC_REPORT_FATAL("SCSlavePort",
                        "No binded transactor, please check");
        }

        // Release transaction with all the extensions
        trans->release();
        response = blk_pkt_helper->getBlockingResponse();
    }
}

tlm::tlm_sync_enum
SCSlavePort::nb_transport_bw(tlm::tlm_generic_payload& trans,
    tlm::tlm_phase& phase,
    sc_core::sc_time& delay)
{
    PayloadEvent<SCSlavePort> * pe;
    pe = new PayloadEvent<SCSlavePort>(*this, &SCSlavePort::pec, "PE");
    pe->notify(trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}

SCSlavePort::SCSlavePort(const std::string &name_,
    const std::string &systemc_name,
    gem5::ExternalSlave &owner_) :
    gem5::ExternalSlave::ExternalPort(name_, owner_),
    blockingRequest(NULL),
    needToSendRequestRetry(false),
    blockingResponse(NULL),
    transactor(nullptr),
    blk_pkt_helper(new BlockingPacketHelper())
{

}

void
SCSlavePort::bindToTransactor(Gem5SlaveTransactor* transactor)
{
    sc_assert(this->transactor == nullptr);

    this->transactor = transactor;

    transactor->socket.register_nb_transport_bw(this,
                                                &SCSlavePort::nb_transport_bw);
}

void
SCSlavePort::bindToTransactor(Gem5SlaveTransactor_Multi* transactor)
{
    sc_assert(this->transactor == nullptr);

    this->transactor_multi = transactor;
    for (int i = 0; i < transactor->getSocketNum(); i++){
        transactor->sockets[i].register_nb_transport_bw(this,
                                                &SCSlavePort::nb_transport_bw);
    }
    this->usingGem5Cache = transactor->isUsingGem5Cache(); //TODO
    // initiate blocking packet helper
    this->blk_pkt_helper->setUsingGem5Cache(
                                            transactor->isUsingGem5Cache());
    this->initSocketMap();
    this->blk_pkt_helper->init(this->socket_map.size());
    // print the socket map , TODO: can be removed
    std::cout << "Print the socket port map" << std::endl;
    auto iter = this->socket_map.begin();
    while (iter != this->socket_map.end()){
        auto it_2 = iter->second;
        auto it_3 = it_2.begin();
        while (it_3 != it_2.end()){
            std::cout << iter->first << " " << *it_3<< std::endl;
            it_3 ++;
        }
        iter ++;
    }
    std::cout << "==============================" <<std::endl;

}

gem5::ExternalSlave::ExternalPort*
SCSlavePortHandler::getExternalPort(const std::string &name,
                                    gem5::ExternalSlave &owner,
                                    const std::string &port_data)
{
    // Create and register a new SystemC slave port
    auto* port = new SCSlavePort(name, port_data, owner);
    control.registerSlavePort(port_data, port);
    return port;
}

uint32_t SCSlavePort::getSocketId(gem5::RequestorID id)
{
    // TODO: different cpu can used same port is set as a cpu cluster
    for (auto it = socket_map.begin();it != socket_map.end(); it++){
        auto it_find = std::find(it->second.begin(),it->second.end(),id);
        if (it_find != it->second.end()){
            if (this->usingGem5Cache){
                // socket0 is used for system port
                return it->first + 1;
            }else {
                return it->first;
            }
        }
    }
    /* Packet from system requestor like functional or write back will use this
     * socket. If not use gem5 cache, only functional packet will using this
     * port. We can using the first socket to transfer the packet.
     */
    return 0;
}

void SCSlavePort::
updateCorePortMap(std::map<const std::string, std::list<gem5::RequestorID>> map)
{
    std::cout << "Update the core port map" << std::endl;
    this->cpu_port_map = map;
    // init cpu_vec
    auto it = this->cpu_port_map.begin();
    while (it != this->cpu_port_map.end()){
        this->cpu_vec.push_back(it->first);
        it++;
    }

    // print the map, TODO: can be removed
    std::cout << "==============================" <<std::endl;
    std::cout << "Print the core port map" << std::endl;
    it = this->cpu_port_map.begin();
    while (it != this->cpu_port_map.end()){
        auto it_2 = it->second;
        auto it_3 = it_2.begin();
        while (it_3 != it_2.end()){
            std::cout << it->first << " " << *it_3<< std::endl;
            it_3 ++;
        }
        it ++;
    }
    std::cout << "==============================" <<std::endl;
}

void SCSlavePort::initSocketMap()
{
    if (transactor_multi == nullptr) {
        // do nothing
        return;
    }
    std::cout << "initSocketMap start " << std::endl;
    std::map<uint32_t, std::vector<int>> config_map;
    config_map = transactor_multi->getSocketCoreMap();
    if (config_map.empty()){
        // using default config
        uint32_t count = 0;
        std::cout << "using default " << std::endl;
        auto iter = this->cpu_port_map.begin();
        while (iter != this->cpu_port_map.end()){
            auto requestorId_list = iter->second;
            this->socket_map.insert(
                std::pair<uint32_t, std::list<gem5::RequestorID>>
                (count, requestorId_list));
            count++;
            iter++;
        }
    } else {
        // set by config map
        auto iter = config_map.begin();
        while (iter != config_map.end()){
            int socket_id = iter->first;
            if (usingGem5Cache){
                //socket_id++;
            }
            auto core_vec = iter->second;
            std::cout << "using config file "<<std::endl;
            for (int i = 0; i < core_vec.size(); i++){
                int core_id = core_vec[i];
                if (core_id >= this->cpu_vec.size()) { // TODO
                    continue;
                }
                const std::string core_name = this->cpu_vec[core_id];
                this->socket_map[socket_id].insert(
                    this->socket_map[socket_id].end(),
                    cpu_port_map[core_name].begin(),
                    cpu_port_map[core_name].end()
                );
            }
            iter++;
        }

    }
    std::cout << "initSocketMap completed" << std::endl;
}

}
