[![REUSE status](https://api.reuse.software/badge/github.com/SAP/systemd-wrapper-lib)](https://api.reuse.software/info/github.com/SAP/systemd-wrapper-lib)

# systemd-wrapper-lib

## About this project

High level access to libsystemd calls that hides D-Bus communication

## Requirements and Setup

systemd-wrapper-lib is an abstraction layer to libsystemd from the [systemd](https://www.freedesktop.org/wiki/Software/systemd/) project
and therefore we have chosen the library name libsdw for 'systemd wrapper' or 'sd_* function wrapper'.
It covers a small subset of the systemd [D-Bus API](https://www.freedesktop.org/wiki/Software/systemd/dbus/)
of libsystemd by abstracting the D-Bus related data types and D-Bus communication.
The connection to the system bus is created with [sd_bus_open_system()](https://www.freedesktop.org/software/systemd/man/sd_bus_open_system.html#)
and is held in a single bus connection object.
libsdw is contained in the source file sdw.cpp, it depends on the libsystemd header files.

If you want to integrate a service in a systemd environment you might have to implement some basic features on top of libsystemd or you can use the following set of functions from libsdw:
- start/stop/restart/enable/disable a service
- read/check some service properties, e.g. active/substate
- trigger a reload of the systemd config
- wrap sd_notify() calls

sdwc is a simple client for libsdw, that covers most of the libsdw functions and provides a cli.

### How to build and launch
#### Clone this repo somewhere on your local disk
 ```sh
  #> git clone https://github.com/SAP/systemd-wrapper-lib.git
  ```
#### Adjust the make file to your needs and build the client
 ```sh
  #> make
  ```
#### Run the client to get a brief description of the available functions
```sh
  #> ./sdwc -h
  ```

## Support, Feedback, Contributing

This project is open to feature requests/suggestions, bug reports etc. via [GitHub issues](https://github.com/SAP/systemd-wrapper-lib/issues). Contribution and feedback are encouraged and always welcome. For more information about how to contribute, the project structure, as well as additional contribution information, see our [Contribution Guidelines](CONTRIBUTING.md).

## Security / Disclosure
If you find any bug that may be a security problem, please follow our instructions at [in our security policy](https://github.com/SAP/systemd-wrapper-lib/security/policy) on how to report it. Please do not create GitHub issues for security-related doubts or problems.

## Code of Conduct

We as members, contributors, and leaders pledge to make participation in our community a harassment-free experience for everyone. By participating in this project, you agree to abide by its [Code of Conduct](https://github.com/SAP/.github/blob/main/CODE_OF_CONDUCT.md) at all times.

## Licensing

Copyright 2023 SAP SE or an SAP affiliate company and systemd-wrapper-lib contributors. Please see our [LICENSE](LICENSE) for copyright and license information. Detailed information including third-party components and their licensing/copyright information is available [via the REUSE tool](https://api.reuse.software/info/github.com/SAP/systemd-wrapper-lib).
