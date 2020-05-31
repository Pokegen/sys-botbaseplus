# sys-botbaseplus

This is a modified C++ port of [olliz0r sys-botbase](https://github.com/olliz0r/sys-botbase) which is also based on [jakibakis sys-netcheat](https://github.com/jakibaki/sys-netcheat) 

This sysmodule adds optional HTTP support, which has a few advantages over raw tcp:

- HTTP is a standardized protocol with well defined uses
- Simple & Easy implementation on the client side
- Usable from almost everywhere (e.g. Browsers, Apps, Server Applications etc.)


## Prerequisites

If you wanna run this you need a switch with cfw installed (preferably Atmosphere 0.12.0)

## Getting Started

> This sys-module was only tested on Atmosphere 0.12.0, so if you use another version or cfw your experience might vary

1. Copy the sys-botbaseplus.nsp file to sdmc://atmosphere/contents/430000000000000C rename it to exefs.nsp
2. Optional, create a password.txt in sdmc://atmosphere/contents/430000000000000C containing a password used for http communication
3. Create a new folder in sdmc://atmosphere/contents/430000000000000C names "flags"
4. Create a empty file called boot2.flag inside this folder
5. Restart your switch

### Connecting

- sys-botbase compatible TCP server running on port 6000
- http server running on port 9999

### Http Routes

All Endpoints expect a `Authorization` header with the specified password from the `password.txt`, if not present defaults to `youshallnotpass`.

#### Get

 - `/version` Get current version of sys-botbaseplus
 - `/healtz` Health check to see if the http server is running
 - `/metadata` Metadata about Switch (current titleId & buildId)

#### Post

- `/` Main Command endpoint, expects JSON body see below

### Request & Response format

All request data is expected to be JSON format, all response will be in JSON format.

`command` & `button` values are case insensetive.

#### Examples

```json
{
	"command": "CLICK",
	"button": "a"
}
```

```json
{
	"command": "press",
	"button": "A"
}
```

```json
{
	"command": "release",
	"button": "a"
}
```

```json
{
	"command": "peek",
	"offset": "0x4293D8B0",
	"size": 344
}
```

```json
{
	"command": "poke",
	"offset": "0x4293D8B0",
	"value": "0x41964b8b0000df79b90c04dbce1ef730d83539c2cf6098790106e859eb4f9eba60fa881eae9cd5f25383e8e49873bfa8cb73e17c67005dfb21be932602298b221968dd866bbeab80dda6ecd64fbe51d19e0105a4e40c7fa5eecb9a085f767c6f7c9de5281ff787cfcaac76bda692cf1d113278d36537a55754efb6fd757ce7f19c4b369d0718713c379eb61164e8cf4c969084d8cbabb4a479bad206733a4a0748dcd19a9062ee319e5b5c86ffbb546cb1e63481ad9dd122e6606ffb920da002e87d4fbdcaa2fc4755b1db2a4dbceb2bd94fa36de6d861bb9e082657d7dc5b4181355c58c82bf88803617a9caa3f6b1fba383ede8e5109fbb0d96b0e95172eaddceeb74ea542ced7373d9215e20d442bef8e4887feb3ae61d10eca5d9b84cba7638a08d88fa3cb29902ea1fd2fb469b72cb21d56d0f475b2b60531788a8e97bef9f04b563f1db5e1010ed8db252e73e4ec2b91c2de609979"
}
```

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
