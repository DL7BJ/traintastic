
#include "cbusmessage.hpp"
#include "../../../../utils/bit.hpp"
#include "../../../../utils/byte.hpp"

namespace CBUS {

struct PresenceOfNode : NodeMessage
{
  static constexpr uint8_t isConsumerNodeBit = 0;
  static constexpr uint8_t isProducerNodeBit = 1;
  static constexpr uint8_t flimModeBit = 2;
  static constexpr uint8_t isBootloaderCompatibleBit = 3;
  static constexpr uint8_t consumesOwnProducedEventsBit = 4;
  static constexpr uint8_t inLearnModeBit = 5;
  static constexpr uint8_t supportsServiceDiscoveryBit = 6;

  uint8_t manufacturerId;
  uint8_t moduleId;
  uint8_t flags;

  PresenceOfNode(uint16_t nodeNumber_, uint8_t manufacturerId_, uint8_t moduleId_,
        bool isConsumerNode_, bool isProducerNode_, bool flimMode_, bool isBootloaderCompatible_,
        bool consumesOwnProducedEvents_, bool inLearnMode_, bool supportsServiceDiscovery_)
    : NodeMessage(OpCode::PNN, nodeNumber_)
    , manufacturerId{manufacturerId_}
    , moduleId{moduleId_}
    , flags{0}
  {
    setBit<isConsumerNodeBit>(flags, isConsumerNode_);
    setBit<isProducerNodeBit>(flags, isProducerNode_);
    setBit<flimModeBit>(flags, flimMode_);
    setBit<isBootloaderCompatibleBit>(flags, isBootloaderCompatible_);
    setBit<consumesOwnProducedEventsBit>(flags, consumesOwnProducedEvents_);
    setBit<inLearnModeBit>(flags, inLearnMode_);
    setBit<supportsServiceDiscoveryBit>(flags, supportsServiceDiscovery_);
  }

  bool isConsumerNode() const
  {
    return getBit<isConsumerNodeBit>(flags);
  }

  bool isProducerNode() const
  {
    return getBit<isProducerNodeBit>(flags);
  }

  bool flimMode() const
  {
    return getBit<flimModeBit>(flags);
  }

  bool isBootloaderCompatible() const
  {
    return getBit<isBootloaderCompatibleBit>(flags);
  }

  bool consumesOwnProducedEvents() const
  {
    return getBit<consumesOwnProducedEventsBit>(flags);
  }

  bool inLearnMode() const
  {
    return getBit<inLearnModeBit>(flags);
  }

  bool supportsServiceDiscovery() const
  {
    return getBit<supportsServiceDiscoveryBit>(flags);
  }
};

}
