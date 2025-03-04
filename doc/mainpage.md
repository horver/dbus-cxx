dbus-cxx Library {#mainpage}
===


# About
dbus-cxx is a C++ implementation of the DBus protocol. It does not use the reference C
implementation of libdbus at all in order to provide a tight C++-like interface, and to
fix known multithreading problems with libdbus.  libsigc++ is used to help provide an
OO interface to the bus.
Also included is dbus-cxx-xml2cpp to generate proxy and adapter interfaces from DBus XML
introspection-like documents.

 <br><hr>

 If you want to jump into the code quickly check out the quick start guide:
 \ref quick_start
 
 <br><hr>
 
 \par dbus-cxx Github Project Page
 The dbus-cxx Github project page can be found here:

 \par
 <a href="https://dbus-cxx.github.io">https://dbus-cxx.github.io</a>

 <br><hr>

 <h2>Download dbus-cxx packages or get the source code</h2>
 

 \b Pre-built packages

 Pre-built Debian packages may be found here: <a href="http://apt.rm5248.com/">http://apt.rm5248.com/</a>.  These may or may not work properly with Ubuntu.

 Debian/Ubuntu package description
 - \b dbus-cxx2 - Libraries needed to run applications
 - \b dbus-cxx-dev - Headers and libraries for developing applications with dbus-cxx
 - \b dbus-cxx-doc - Developer's documentation including devhelp docs
 - \b dbus-cxx-tools - Development tools, et. al. such as  [dbus-cxx-xml2cpp](@ref xml2cpp)
 - \b dbus-cxx-glib2 - Libraries need to run applications that integrate dbus-cxx into glib
 - \b dbus-cxx-glib-dev - Headers and libraries for developing applications that will integrate dbus-cxx into glib
 - \b dbus-cxx-qt2 - Libraries need to run applications that integrate dbus-cxx into Qt
 - \b dbus-cxx-qt-dev - Headers and libraries for developing applications that will integrate dbus-cxx into Qt
 

 \htmlonly <img src="sourcecode-small.png" alt=""/> \endhtmlonly
 \par Source Code
 \htmlonly <img src="download-small.png" alt=""/> \endhtmlonly
 \b Releases - (.bz2, .gz, .zip) can be found on <a href="http://sourceforge.net/projects/dbus-cxx/files">Sourceforge</a> and <a href="https://github.com/dbus-cxx/dbus-cxx/releases">Github</a>

  [Source building directions](@ref building-from-source)

 \par
 \b Git \b Repository
 - You can browse the git repository at this url:
   - <a href="https://github.com/dbus-cxx/dbus-cxx">https://github.com/dbus-cxx/dbus-cxx</a>
 - You can also check out a copy of the repository with this command:
   - \verbatim git clone https://github.com/dbus-cxx/dbus-cxx.git \endverbatim

 <br><hr>

 \par Dependencies... and where to get them
 \b dbus: <a href="http://dbus.freedesktop.org">http://dbus.freedesktop.org</a> - Should be installed by default (runtime dependency) <br/>
 \b libsigc++ <a href="https://github.com/libsigcplusplus/libsigcplusplus">https://github.com/libsigcplusplus/libsigcplusplus</a> (compile dependency)
 See the README file for more information on compile-time dependencies <br/>
 \b libcppgenerate <a href="https://github.com/rm5248/libcppgenerate">https://github.com/rm5248/libcppgenerate</a> (compile dependency)

 <br><hr>

 \htmlonly <img src="documents-small.png" alt=""/> \endhtmlonly
 \par Documentation, Tutorials, Guides, Quick Start, et. al.

 \par Quick Start Guide
 If you want to jump into the code quickly check out the \ref quick_start guide.
 
 \par dbus-cxx-xml2cpp Reference
 Documentation on using  [dbus-cxx-xml2cpp](@ref xml2cpp) to convert DBus XML introspection
 documents into C++ proxies and adapters.
 
 \par API Reference
 The API documentation (including the pages you are reading now) have been generated
 with Doxygen.

 \par
 The most current documentation for dbus-cxx is available online here:
 - dbus-cxx - <a href="https://dbus-cxx.github.io">https://dbus-cxx.github.io</a>

 \par Key sections within the API reference
 - Namespaces
   - DBus
 - Detailed class documentation in the <a href="hierarchy.html">Class Hierarchy</a>
 - <a href="annotated.html">Class List</a>
 - <a href="examples.html">Examples</a>

 <br><hr>

 \htmlonly <img src="mail-bulk-solid.png" alt="" width="50" height="50"/> \endhtmlonly
 \par Mailing Lists
 <b>Note that the old Sourceforge mailing lists are monitored, but new users should use the Google group.</b>
 <br/>
 - Google group(preferred)
   - <a href="https://groups.google.com/forum/#!forum/dbus-cxx">Dbus-cxx Google Group</a>
 - Sourceforge Users List (for those using dbus-cxx in their applications)
   - <a href="http://sourceforge.net/mailarchive/forum.php?forum_name=dbus-cxx-users">Archives</a>
   - <a href="http://lists.sourceforge.net/mailman/listinfo/dbus-cxx-users">Subscribe/Unsubscribe</a>
 - Sourceforge Development List (for discussion related to the development of dbus-cxx itself)
   - <a href="http://sourceforge.net/mailarchive/forum.php?forum_name=dbus-cxx-devel">Archives</a>
   - <a href="http://lists.sourceforge.net/mailman/listinfo/dbus-cxx-devel">Subscribe/Unsubscribe</a>

 

 <br><hr>

 \par Bugs

 \par 
 Found a bug?  Open a bug on our [Github bug tracker.](https://github.com/dbus-cxx/dbus-cxx/issues)
 If you need help using the library, the mailing list is the better location to ask for help.

 <br><hr>

 \par Patches
 Patches are always welcome!!!

 \par 
 You can either submit a patch through the mailing list, or create a pull request on Github.

 <br><hr>

 \par Feature Requests

 \par
 Either open a bug in the issue tracker, or provide your request to the mailing list.  If it is a complicated feature request,
 the mailing list may be better to discuss what needs to be done.

 <br><hr>

 \par License
 dbus-cxx is released under the \htmlonly <a href="https://www.gnu.org/licenses/lgpl-3.0.txt"><img src="lgplv3.png" alt="LGPLv3"/></a> \endhtmlonly
