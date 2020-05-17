# sys-botbaseplus

This is a C++ port of [olliz0r sys-botbase](https://github.com/olliz0r/sys-botbase) which is also based on [jakibakis sys-netcheat](https://github.com/jakibaki/sys-netcheat)

## Prerequisites

If you wanna run this you need a switch with cfw installed (preferably Atmosphere 0.10.2+)

## Getting Started

> This was only tested on Atmosphere 0.10.2, so if you use another version or cfw your experience might vary

1. Copy the sys-botbase.nsp file to sdmc://atmosphere/contents/430000000000000B rename it to exefs.nsp
2. Create a new folder in sdmc://atmosphere/contents/430000000000000B names "flags"
3. Create a empty file called boot2.flag inside this folder
4. Restart your switch

## Contributing

### Building

> This will only build on Linux due to the Http library [Pistache](http://pistache.io/)

1. Install gcc (needs support for C++20) & make
2. Install dependencies needed to build [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/building.md)
3. Clone repo and init all submodules
4. Execute the Makefile

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/DevYukine/sys-botbaseplus/tags). 

## Authors

* **jakibaki** - *Creation of sys-netcheat* - [jakibaki](https://github.com/jakibaki)
* **olliz0r** - *Creation of sys-botbase* - [olliz0r](https://github.com/olliz0r/sys-botbase)
* **DevYukine** - *Initial work* - [DevYukine](https://github.com/DevYukine)

See also the list of [contributors](https://github.com/DevYukine/sys-botbaseplus/contributors) who participated in this project.

Also a big thanks to the people on the ReSwitched Discord Server who helped me setting up everything (especially SciresM & Behemoth)

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE.md](LICENSE.md) file for details