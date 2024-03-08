#ifndef __WIFI_HPP___
#define __WIFI_HPP___

#include <list>

namespace Net {

typedef std::list<std::string> SsidList;

bool Init();

// Calling this routine with ssidList = NULL will write the SSIDs to the 
// debug log only. 
bool ListNetworks(SsidList *ssidList = NULL);

// Returns true without waiting if we're already connected. Otherwise, it 
// will wait up to maxWaitMs milliseconds for an IP address to be obtained.
// Returns true if we were able to get an IP, or false on timeout.
bool WaitForConnection(uint32_t maxWaitMs);

} // namespace Net

#endif
