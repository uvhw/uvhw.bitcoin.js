Graviocoin Core version 0.1.7:
==============================

  <http://gravio.net/downloads/>

Main changes:
- Changed diffuculty retargeting algorithm. Diff retarget now works after each new block.

Please report bugs using the issue tracker at github:

  <https://github.com/gravio-net/graviocoin/issues>

Graviocoin Core version 0.1.6:
==============================

  <http://gravio.net/downloads/>

This is a base release of Graviocoin. Main changes:
- Optional “link” added to the resource in the GRAVIO Ecosystem into the body of transaction (each txout now has a blob, where link is stored).
- "Send money" dialog changed to support blob info
- "Transactio details" dialog changed to support blob info

Please report bugs using the issue tracker at github:

  <https://github.com/gravio-net/graviocoin/issues>

Compatibility
==============

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
an OS initially released in 2001. This means that not even critical security
updates will be released anymore. Without security updates, using a graviocoin
wallet on a XP machine is irresponsible at least.

In addition to that, with 0.12.x there have been varied reports of Bitcoin Core
randomly crashing on Windows XP. It is [not clear](https://github.com/bitcoin/bitcoin/issues/7681#issuecomment-217439891)
what the source of these crashes is, but it is likely that upstream
libraries such as Qt are no longer being tested on XP.

We do not have time nor resources to provide support for an OS that is
end-of-life. From 0.1.6 on, Windows XP is no longer supported. Users are
suggested to upgrade to a newer version of Windows, or install an alternative OS
that is supported.

No attempt is made to prevent installing or running the software on Windows XP,
you can still do so at your own risk, but do not expect it to work: do not
report issues about Windows XP to the issue tracker.

From 0.13.1 onwards OS X 10.7 is no longer supported. 0.13.0 was intended to work on 10.7+,
but severe issues with the libc++ version on 10.7.x keep it from running reliably.
0.13.1 now requires 10.8+, and will communicate that to 10.7 users, rather than crashing unexpectedly.

Notable changes
===============

New Multisig Address Prefix
---------------------------

Graviocoin Core now supports P2SH addresses beginning with M on mainnet and Q on testnet.
P2SH addresses beginning with 3 on mainnet and m or n on testnet will continue to be valid.
Old and new addresses can be used interchangeably.

miniupnp CVE-2017-8798
----------------------

Bundled miniupnpc was updated to 2.0.20170509. This fixes an integer signedness error (present in MiniUPnPc v1.4.20101221 through v2.0) that allows remote attackers (within the LAN) to cause a denial of service or possibly have unspecified other impact.

This only affects users that have explicitly enabled UPnP through the GUI setting or through the -upnp option, as since the last UPnP vulnerability (in Graviocoin Core 0.10.4) it has been disabled by default.

If you use this option, it is recommended to upgrade to this version as soon as possible.

Reset Testnet
-------------

Testnet3 has been deprecated and replaced with Testnet4. The server port has been changed to 43335 however the RPC port remains
the same (43332).

Testnet faucets can be located at:
- http://testnet.graviocointools.com
- http://testnet.thrasher.io

Developers who require the new testnet blockchain paramaters can find them [here](https://github.com/graviocoin-net/graviocoin/blob/gio.0.1.6/src/chainparams.cpp#L214).

Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers](/doc/release-notes)
- [The Litecoin Core Developers](/doc/release-notes)
- Adrian Gallagher
- Shaolin Fry
- Xinxi Wang
- Out0fmemory
- Erasmospunk
- Romanornr
