#include "blocking_packet_helper.hh"

namespace Gem5SystemC
{
BlockingPacketHelper::BlockingPacketHelper()
{

}

void BlockingPacketHelper::init(uint32_t _num)
{
    this->socket_num = _num;
    if (usingGem5Cache) {
        //this->socket_num++; // socket for system port
    }
    for (uint32_t i =0; i <= this->socket_num; i++){
        std::pair<uint32_t, tlm::tlm_generic_payload*> p(i, nullptr);
        this->blockingRequestMap.insert(p);
        this->blockingResponseMap.insert(p);
        std::pair<uint32_t, bool> p2(i, false);
        this->needToSendRequestRetryMap.insert(p2);
    }
}

void BlockingPacketHelper::
updateBlockingMap(uint32_t core_id,
                    tlm::tlm_generic_payload *blocking_trans,
                    pktType type)
{
    assert (core_id <= this->socket_num);
    if (core_id == 0 && usingGem5Cache){ // system port
        if (blocking_trans == NULL){
            this->isSystemPortBlocked = false;
        }else{
            this->isSystemPortBlocked = true;
        }
    }
    std::map<uint32_t, tlm::tlm_generic_payload*>::iterator iter;
    switch (type)
    {
    case Request: // TODO
        iter = this->blockingRequestMap.find(core_id);
        if (iter != this->blockingRequestMap.end()){
            iter->second =  blocking_trans;
            return;
        }
        break;
    case Response:
        iter = this->blockingResponseMap.find(core_id);
        if (iter != this->blockingResponseMap.end()){
            iter->second =  blocking_trans;
            return;
        }
        break;
    default:
        break;
    }
    std::cerr << "[Error] updateBlocking Map " << core_id << " "
        << type << std::endl;
    SC_REPORT_FATAL("SCSlavePort", "Invaild blocking request");
}

tlm::tlm_generic_payload*
BlockingPacketHelper::getBlockingTrans(uint32_t core_id,
                                        pktType type)
{
    assert (core_id < this->socket_num);
    std::map<uint32_t, tlm::tlm_generic_payload*>::iterator iter;
    switch ( (type))
    {
    case Request:
        iter = this->blockingRequestMap.find(core_id);
        if (iter != this->blockingRequestMap.end())
        {
            return iter->second;
        }
        break;
    case Response:
        iter = this->blockingResponseMap.find(core_id);
        if (iter != this->blockingResponseMap.end())
        {
            return iter->second;
        }
        break;
    default:
        break;
    }
    std::cerr << "[Error] getBlockingTrans " << core_id << " "
        << type << std::endl;
    SC_REPORT_FATAL("SCSlavePort", "Invaild blocking request");
    return NULL;
}

bool BlockingPacketHelper::isBlockedPort(uint32_t core_id,  pktType type)
{
    if (isSystemPortBlocked) {
        return true;
    }
    if (this->getBlockingTrans(core_id, type))
    {
        return true;
    } else
    {
        return false;
    }
}

bool BlockingPacketHelper::
isBlockingTrans(tlm::tlm_generic_payload *blockingRequest, pktType type)
{
    if (blockingRequest == NULL)
    {
        return false;
    }
    auto it = this->blockingRequestMap.begin();
    while (it != this->blockingRequestMap.end()){
        if (it->second == blockingRequest){
            return true;
        }
        it++;
    }
    return false;
}

bool BlockingPacketHelper::needToSendRequestRetry(uint32_t core_id)
{
    assert(core_id < this->socket_num);
    auto iter = this->needToSendRequestRetryMap.find(core_id);
    if (iter != this->needToSendRequestRetryMap.end()){
        return iter->second;
    }
    // Not found, should throw exception?
    SC_REPORT_FATAL("SCSlavePort", "Invaild send request retry");
    return false;
}

void BlockingPacketHelper::updateRetryMap(uint32_t core_id,
                                        bool state)
{
    assert(core_id < this->socket_num);
    auto it = this->needToSendRequestRetryMap.begin();
    while (it != this->needToSendRequestRetryMap.end()){
        if (it->first == core_id) {
            it->second = state;
        }
        it++;
    }
    return;
}

tlm::tlm_generic_payload* BlockingPacketHelper::getBlockingResponse()
{
    auto it = this->blockingResponseMap.begin();
    while (it != this->blockingResponseMap.end()){
        if (it->second != NULL){
            return it->second;
        }
        it++;
    }
    return NULL;
}

}
