#pragma once

#include <string>

#ifndef _WIN32
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#else
#include <vector>
#include <windows.h>
#include "netheaders-windows.h"
#include <Mantids29/Helpers/appexec.h>
#endif

namespace Mantids29 { namespace Network { namespace Interfaces {

// LICENSE WARNING: This class is licensed under GPLv2 (not LGPL) for NETIF_VIRTUAL_WIN interfaces.

/**
 * @brief The NetworkInterfaceConfiguration class Network Interface Configuration
 */
class NetworkInterfaceConfiguration
{
public:
    enum NetIfType
    {
        NETIF_GENERIC_LIN,
        NETIF_VIRTUAL_WIN
    };

    NetworkInterfaceConfiguration();
    ~NetworkInterfaceConfiguration();

#ifndef _WIN32
    /**
     * @brief openInterface Open the interface given the interface name (Linux)
     * @param _ifaceName interface name.
     * @return true if openned
     */
    bool openInterface(const std::string & _ifaceName);
#else
    /**
     * @brief openTAPW32Interface Pass the interface previously openned with VirtualNetworkInterface (TAP-WINDOWS6)
     * @param fd file descriptor of the interface.
     * @param adapterIndex adapter index (used for netsh operations)
     */
    void openTAPW32Interface(HANDLE fd, ULONG adapterIndex);
#endif

    // Getters:
    /**
     * @brief getMTU Get the interface MTU
     * @return interface MTU
     */
    int getMTU();
    /**
     * @brief getEthernetAddress Get the ethernet MAC Address of the interface
     * @return mac address is setted in the return h_dest
     */
    ethhdr getEthernetAddress();
    /**
     * @brief getLastError Get last error
     * @return empty if no error, or a string containing the last error
     */
    std::string getLastError() const;

    // Setters:
    /**
     * @brief setIPv4Address Set the IPv4 Address and Netmask
     * @param _address IPv4 Address
     * @param _netmask Netmask
     */
    void setIPv4Address(const in_addr &_address,const in_addr &_netmask);
    /**
     * @brief setMTU Set the MTU
     * @param _mtu MTU value (eg. 1500)
     */
    void setMTU(int _mtu);
    /**
     * @brief setPromiscMode Set the interface in promiscous mode
     * @param state true for promisc
     */
    void setPromiscMode(bool state=true);
    /**
     * @brief setUP Set the interface UP or DOWN
     * @param state true for UP, false for DOWN
     */
    void setUP(bool state=true);

    // Apply:
    /**
     * @brief apply Apply the settings
     * @return true if successfully applied.
     */
    bool apply();

    /**
     * @brief getNetIfType Get Network Interface Type.
     * @return Network Interface Type (Generic Linux, Virtual TAP Windows)
     */
    NetIfType getNetIfType() const;

#ifdef _WIN32
    static Mantids29::Helpers::AppExec::sAppExecCmd createRouteCMD( const std::vector<std::string> & routecmdopts );
    static Mantids29::Helpers::AppExec::sAppExecCmd createNetSHCMD( const std::vector<std::string> & netshcmdopts );
#endif

private:
#ifndef _WIN32
    struct ifreq ifr;
    int fd;
#else
    ULONG adapterIndex;
    HANDLE fd;
    static std::string getNetSHExecPath();
    static std::string getRouteExecPath();
#endif


    in_addr address, netmask;

    std::string interfaceName;
    std::string lastError;
    int MTU;
    bool promiscMode,stateUP;
    bool changeIPv4Addr, changeMTU, changeState, changePromiscMode;
    NetIfType netifType;
};

}}}

