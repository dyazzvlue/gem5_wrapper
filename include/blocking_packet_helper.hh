#ifndef __GEM5_BLOCKING_PACKET_HELPER_HH__
#define __GEM5_BLOCKING_PACKET_HELPER_HH__

#include <map>
#include <systemc>
#include <tlm>

#include "sc_slave_port.hh"

namespace Gem5SystemC
{

enum pktType {Request, Response};

class BlockingPacketHelper
{
    public:
        BlockingPacketHelper();
        virtual ~BlockingPacketHelper() {};

        // TODO: maybe change core_id to socket_id?
        void updateBlockingMap(uint32_t core_id,
                            tlm::tlm_generic_payload *blocking_trans,
                            pktType type);
        tlm::tlm_generic_payload* getBlockingTrans(uint32_t core_id,
                                                pktType type);
        void init(uint32_t num);
        bool isBlockedPort(uint32_t core_id, pktType type);
        bool isBlockingTrans(tlm::tlm_generic_payload *blockingRequest,
                                pktType type);

        bool needToSendRequestRetry(uint32_t core_id);
        void updateRetryMap(uint32_t core_id, bool state);

        void setUsingGem5Cache(bool state) {this->usingGem5Cache = state;}

        /**
         * @brief Get the Blocking Response object
         * If there is any blocking response transaction, return it.
         * If not, return nullptr.
         *
         * @return tlm::tlm_generic_payload*
         */
        tlm::tlm_generic_payload* getBlockingResponse();

    private:
        int socket_num;
        /**
        * Using a map to save blocking transactions. Transaction will not be
        * blocked when a packet from another core is processing.
        *
        * key: core id  value: tlm_genric_payload
        */
        std::map<uint32_t, tlm::tlm_generic_payload*> blockingRequestMap;
        std::map<uint32_t, tlm::tlm_generic_payload*> blockingResponseMap;

        std::map<uint32_t, bool> needToSendRequestRetryMap;

        /**
        * A transaction after BEGIN_REQ has been sent but before END_REQ, which
        * is blocking the request channel (Exlusion Rule, see IEEE1666)
        */
        tlm::tlm_generic_payload *blockingRequest;
        /**
        * Did another gem5 request arrive while currently blocked?
        * This variable is needed when a retry should happen
        */
        bool needToSendRequestRetry_;

        // if packet from system writeback/functional/interrupt port,
        // always blocked
        bool isSystemPortBlocked = false;
        // if using gem5 cache
        bool usingGem5Cache = false;
};

}

#endif
