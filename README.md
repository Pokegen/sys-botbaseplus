# sys-botbaseplus

This is a modified C++ port of [olliz0r sys-botbase](https://github.com/olliz0r/sys-botbase) which is also based on [jakibakis sys-netcheat](https://github.com/jakibaki/sys-netcheat) 

This sysmodule also adds optional HTTP support, which has a few advantages over raw tcp:

- HTTP is a standardized protocol with well defined uses
- Simple & Easy implementation on the client side
- Usable from mostly everywhere (e.g. Browsers, Apps, Server Applications etc.)


## Prerequisites

If you wanna run this you need a switch with cfw installed (preferably Atmosphere 0.10.2+)

## Getting Started

> This sys-module was only tested on Atmosphere 0.12.0, so if you use another version or cfw your experience might vary

1. Copy the sys-botbaseplus.nsp file to sdmc://atmosphere/contents/430000000000000C rename it to exefs.nsp
2. Create a new folder in sdmc://atmosphere/contents/430000000000000C names "flags"
3. Create a empty file called boot2.flag inside this folder
4. Restart your switch

## Contributing

### Building

1. Install gcc (needs support for C++20) & make
2. Install dependencies needed to build [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/building.md)
3. Clone repo and init all submodules
4. Execute the Makefile

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/DevYukine/sys-botbaseplus/tags). 

## Authors

* **jakibaki** - *Creation of sys-netcheat* - [jakibaki](https://github.com/jakibaki)
* **olliz0r** - *Creation of sys-botbase* - [olliz0r](https://github.com/olliz0r)
* **yhirose** - *Creation of cpp-httplib* - [yhirose](https://github.com/yhirose)
* **zaksabeast** - *Creation of sys-http & modification of cpp-httplib to work with Switch* - [zaksabeast](https://github.com/zaksabeast)
* **DevYukine** - *Initial work* - [DevYukine](https://github.com/DevYukine)

See also the list of [contributors](https://github.com/DevYukine/sys-botbaseplus/contributors) who participated in this project.

Also a big thanks to the people on the ReSwitched Discord Server (especially SciresM & Behemoth) and olliz0r who helped me during development 😊

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE.md](LICENSE.md) file for details
