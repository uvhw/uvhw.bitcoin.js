Graviocoin Core 0.1.8
=====================

Setup
---------------------
[Graviocoin Core](http://gravio.net/en/download) is the original Graviocoin client and it builds the backbone of the network. However, it downloads and stores the entire history of Graviocoin transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

Running
---------------------
The following are some helpful notes on how to run Graviocoin on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/graviocoin-qt` (GUI) or
- `bin/graviocoind` (headless)

### Windows

Unpack the files into a directory, and then run graviocoin-qt.exe.

### OS X

Drag Graviocoin-Core to your applications folder, and then run Graviocoin-Core.

### Need Help?

* See the documentation at the [Graviocoin Wiki](https://graviocoin.info/)
for help and more information.
* Ask for help on [#graviocoin](http://webchat.freenode.net?channels=graviocoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net?channels=graviocoin).
* Ask for help on the [GraviocoinTalk](https://graviocointalk.io/) forums.

Building
---------------------
The following are developer notes on how to build Graviocoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Graviocoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/bitcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Unit Tests](unit-tests.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [GraviocoinTalk](https://graviocointalk.io/) forums.
* Discuss project-specific development on #graviocoin on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net/?channels=graviocoin).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)

License
---------------------
Distributed under the [MIT software license](http://www.opensource.org/licenses/mit-license.php).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
