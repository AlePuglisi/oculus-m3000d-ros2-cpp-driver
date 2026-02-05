#include "Oculus/Oculus/OssDataWrapper.h"

static void OSS_ReadMemoryEx(WrappedMemoryReadMessage wrapped)
{
    OculusMemoryReadMessage message = wrapped.message;
    message.head.msgId = OculusMessageType::messageMemoryRead;
    message.head.oculusId = 0x4F53;

    int size = sizeof(OculusMemoryReadMessage);


}

void OSS_ReadMemory(uint32_t type, uint32_t address)
{
    // TODO Range check the 'address' param

    WrappedMemoryReadMessage wrapped;
    wrapped.message.memType = (uint16_t)type;
    wrapped.message.address = address;
    wrapped.message.count = 1024;

    OSS_ReadMemoryEx(wrapped);
}
