## uca-andor

Andor SDK plugin for [libuca](https://github.com/ufo-kit/libuca).

This document sums up the installation procedure for the entire configuration of
the Andor Neo Camera access through libuca.  Though this document cannot be at
100% exaustive (especially concerning Linux in itself), it tries to describe as
precisely as possible all the installation manipulations in order to make the
system work without encountering any difficulty.

The plugin has been tested with the Neo-2-6-6089-CLB framegrabber, the Andor SDK
3.13.300001 and the Bitflow driver 9.05 using the following OS configurations:

* Debian 8.8.0 (Linux 3.16.0): works but a bit weirdly. For some reason, under
  this OS many commands in the terminal have to be called with `sudo` though it
  should not be necessary (error example encountered if not using sudo:
  `ldconfig: not found`). Some libuca commands may also not work executed in
  sudo mod. It may be due to an error in the user configuration under debian 8,
  but in any case this is not due to an installation error of libuca or the SDK.
* Ubuntu 16.04 LTS (Linux 4.8.0): does *not* work.  The communication link
  between the computer and the frame grabber seems to exist, but each time we
  try to access the camera it crash the bitflow driver module (terminal freeze,
  and impossible to remove the bitflow.ko driver module or kill the process...
  only solution = reboot). After running successfully the configuration #3
  below, it appears that this is a kernel compatibility problem.
* Ubuntu 16.04 LTS (Linux 4.4.0): does work.


### Quick start

Here's the most conscise way to get to a running system:

1. Untar `andor-linux-sdk3-x.xx.xxxxx.x.tgz`
2. Build `andor/bitflow/drv` with the install script (see https://www.kernel.org/doc/Documentation/kbuild/modules.txt)
3. Install module with `make -C /lib/modules/$(uname -r)/build M=$PWD modules_install`
4. Run `depmod -a`
5. Clone https://github.com/ufo-kit/libuca and https://github.com/ufo-kit/uca-andor
6. Copy `uca-andor/etc/Makefile` to the root of `andor/` and run `LIBDIR=lib64 make install`
7. Build libuca and the uca-andor plugin


### Preparation of the Linux environment

To install the older 4.4.0 kernel version on an Ubuntu 16.04:

    $ wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.4-wily/linux-headers-4.4.0-040400_4.4.0-040400.201601101930_all.deb
    $ wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.4-wily/linux-headers-4.4.0-040400-generic_4.4.0-040400.201601101930_amd64.deb
    $ wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.4-wily/linux-image-4.4.0-040400-generic_4.4.0-040400.201601101930_amd64.deb
    $ sudo dpkg -i *.deb

You can then set this boot configuration as default by installing `sudo apt-get
install grub-customizer` and move "Ubuntu, with Linux 4.4.0-040400-generic" in
the customizer to the top of the loading list. You may have to do this step
again for example if the Linux kernel is updated because the newest version will
be placed on the top of the list. To remove the kernel run:

    $ sudo apt-get remove linux-headers-4.4.0-* linux-image-4.4.0-*; sudo update-grub


### Installation of the Andor SDK

1. Download the Andor SDK and extract the archive which should create a new
   `andor` subdirectory.
2. Change directory to this newly created andor folder and run the installer i.e.:

        $ sudo ./install_andor

   If your platform cannot be determinate automatically, the installer will ask
   for it with the following message:

        Platform cannot be automatically determined. Please select platform to install:
        1. 32-bit
        2. 64-bit
        3. Exit
        Selection:

   to any other question, always answer yes (type `y`)
3. To make sure that the installation is successful, you can check that a new
   folder `/usr/local/mod` as been created containing the file `bitflow.ko`. If
   not, try again the previous step or try running the `install` script in the
   /andor/bitflow folder (that should not be necessary)
4. You can test the correct installation of the software by running the
   `listdevices` example. Change to move to andor/examples/listdevices run

      $ make
      $ ./listdevices

   Please note that until the bitflow driver is correctly set (not yet
   done according to this document), the camera will not be detected, you
   should thus see the following message:

      Found 2 Devices.
      Device 0 : SIMCAM CMOS
      Device 1 : SIMCAM CMOS
      Press any key and enter to exit."

   if not, you may need to set the `LD_LIBRARY_PATH` to the requested missing
   library asked by the error message.


### Installation of the Bitflow driver

1. Edit `/etc/rc.local` and add the following lines just before `exit 0`

      /sbin/modprobe v4l2_common
      /sbin/modprobe v4l1_compat 	# only if your kernel version is below 2.6.38)
      /sbin/modprobe videodev
      /sbin/insmod /usr/local/mod/bitflow.ko fwDelay1=200 customFlags=1
      chmod a+rw /dev/video*/sbin/modprobe videodev"
2. Add the `nopat` kernel option to the bootloader by editing
   `/etc/default/grub`, adding `nopat` to the line `GRUB_CMDLINE_LINUX_DEFAULT`
   and running `sudo update-grub`. You can check that the option is active by
   printing out `/proc/cmdline`, if it is not not the case, reboot your
   computer.
4. Install `libnuma-dev`.
5. Reboot the machine and check that the drivers are loaded correctly:

      $ lsmod
      Module                  Size  Used by
      ...                      ...  ...
      bitflow               163301  0
      v4l2_common            12995  0
      videodev              126451  2 v4l2_common,bitflow
      ...                      ...  ...


When running `listdevices` you should see

    Found 3 Devices.
    Device 0 : DC-152Q-C00-FI
    Device 1 : SIMCAM CMOS
    Device 2 : SIMCAM CMOS
    Press any key and enter to exit."

The `image` example program should allow you to capture a clear image (with the
correct lens obviously).

You may obtain an communication error (code 10) when trying to run those
examples, indicating that the communication link has crashed (but that there is
actually a request sent to the frame grabber, indicating that the link is here).
In this case, reboot both camera and computer.


### Installation of libuca and the uca-andor plugin

Install dependencies and build tools of libuca:

    $ sudo apt install libglib2.0 cmake gcc libgtk+2.0-dev gobject-introspection libgirepository1.0-dev git libtiff5-de

Clone, build and install libuca:

    $ git clone https://github.com/ufo-kit/libuca
    $ cd libuca && mkdir build && cd build
    $ cmake .. && make && sudo make install

Now clone, build and install the Andor plugin:

    $ git clone https://github.com/ufo-kit/uca-andor
    $ cd uca-andor && mkdir build && cd build
    $ cmake .. && make && sudo make install


### Usage

If the installation of bitflow and SDK was successful, you should be able to use
those libuca programs with the "andor" camera. By using

    $ uca-info andor

you should obtain:

    RO | name                      | "DC-152Q-C00-FI"
    ...

Using

    $ uca-camera-control -c andor

should allow you to control the actual camera. If the name displayed by
`uca-info` is "SIMCAM CMOS (model)" or uca-camera-control give only a black
screen as frames, that means that there is no communication between the computer
and the camera (camera turned off, bitflow or SDK not correctly installed...).
If uca programs return a communication error (code 10), it means that the link
is actualy here but something has crashed (happens sometimes when trying to stop
camera acquisition). Rebooting should solve the problem.

For some reason, the libuca graphic camera controller can crash when trying to
stop the acquisition (communication error code 10), when this happen all
comunication will then be impossible no matter the way took to communicate with
the camera.  Rebooting both computer and camera usually solve the problem.
