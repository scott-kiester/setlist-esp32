#ifndef __WIFI_HPP___
#define __WIFI_HPP___

#include <list>

namespace Net {

typedef std::list<std::string> SsidList;

bool Init();

// Calling this routine with ssidList = NULL will write the SSIDs to the 
// debug log only. 
bool ListNetworks(SsidList *ssidList = NULL);

} // namespace Net

#endif
