VpnDialer+
==========

VpnDialer+ is a lightweight utility that works with the Windows VPN Client to enable easy management of [split-tunnel](https://en.wikipedia.org/wiki/Split_tunneling) configurations.

Under most configurations, when you connect to a VPN, all traffic is sent over the VPN, whether it needs to be or not.

With split tunneling, you can connect to a VPN and route only traffic that is destined for specific IP addresses or ranges over the VPN, while using your local Internet connection directly for all other traffic.  This type of configuration has several benefits:

* You can connect to multiple VPNs at the same time and have traffic routed appropriately for each one.
* Your non-VPN traffic will move at the speed of your local Internet connection, not the speed of the VPN.
* Your non-VPN traffic is not subject to any filtering or snooping that might be in place on the VPN network.

App Features
------------

*Automatically monitor VPN connection status and apply routing updates when the VPN is connected/disconnected.
*Prevent idle disconnection by sending periodic ICMP packets to a target over the VPN.
*Compact program with very low memory footprint so you can leave it running at all times.
*Minimizes to the System Tray to keep out of your way.

Requirements
------------

Windows XP or later

Installation
------------

VpnDialer+ is a portable .exe with zero external dependencies.  Configuration is stored in the AppData folder.

Binaries for x86 and x64 Windows are available on the [releases page](https://github.com/saucecontrol/VpnDialerPlus/releases).
