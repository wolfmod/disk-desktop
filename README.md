# Disk Ra Grad — file sync client for Windows

**Диск** (Disk Ra Grad) is the Windows desktop client of the file storage of Ra Grad LLC (ООО «Ра Град»), an engineering survey company. It adds a «Диск» folder to Windows Explorer and keeps company files in sync with the storage server. Files are downloaded on demand (Windows Cloud Files API), so they take no disk space until they are opened.

This repository is a fork of [OpenCloud Desktop](https://github.com/opencloud-eu/desktop) (GPL-2.0-or-later). Changes in the fork (branch `ragrad`):

- sign-in with a phone number or e-mail and a password, then a per-device key that is renewed automatically before it expires;
- the address of the company storage server is built in;
- Russian user interface, simplified main window, tray menu and settings;
- own installer: Russian, per-user installation without administrator rights, uninstaller included.

Клиент синхронизации файлового хранилища ООО «Ра Град» для Windows (форк OpenCloud Desktop).

## Download

- Releases: https://github.com/wolfmod/disk-desktop/releases (`DiskSetup-<version>.exe` and its SHA-256 checksum)
- Download page: https://cloud.ra-grad.ru/app/windows.html

Windows 10 and 11, 64-bit.

## Build

Installers are built by GitHub Actions ([`ragrad-windows.yml`](.github/workflows/ragrad-windows.yml)) with [KDE Craft](https://community.kde.org/Craft), using the recipe in [`admin/craft/ragrad/disk-desktop`](admin/craft/ragrad/disk-desktop) and the NSIS script [`admin/win/disk-ragrad.nsi`](admin/win/disk-ragrad.nsi). A push to `ragrad` produces a test build (workflow artifact), a `v*` tag produces a release.

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

- Signed files: the Windows installer `DiskSetup-<version>.exe`, built from this repository by the GitHub Actions workflow above on GitHub-hosted runners. Third-party libraries in the installer (Qt, KDE Frameworks, OpenSSL and others) are included as provided by KDE Craft.
- Committers and reviewers: [wolfmod](https://github.com/wolfmod)
- Approvers: [wolfmod](https://github.com/wolfmod)
- Every release is approved manually before it is signed.

### Privacy policy

This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it.

The client connects only to the file storage server of Ra Grad LLC (`cloud.ra-grad.ru`) after the user signs in, in order to synchronize the user's files. Automatic update checks and crash reporting are disabled in this build.

## License

GPL-2.0-or-later, the same as upstream (see [COPYING](COPYING)).

---

*The original README of OpenCloud Desktop follows.*

[![OpenCloud Desktop CI](https://github.com/opencloud-eu/desktop/actions/workflows/main.yml/badge.svg)](https://github.com/opencloud-eu/desktop/actions/workflows/main.yml)
# `OpenCloud Desktop`

## Introduction

`OpenCloud Desktop` is a tool to synchronize files from `OpenCloud`
with your computer.

## Download

### Binary packages

- Please have a look at our releases https://github.com/opencloud-eu/desktop/releases

### Source code

The `OpenCloud Desktop` is developed in Git. Since Git makes it easy to
fork and improve the source code and to adapt it to your need, many copies
can be found on the Internet, in particular on GitHub. However, the
authoritative repository maintained by the developers is located at
https://github.com/opencloud-eu/desktop/.

## Reporting issues and contributing

If you find any bugs or have any suggestion for improvement, please
file an issue at https://github.com/opencloud-eu/desktop/issues. Do not
contact the authors directly by mail, as this increases the chance
of your report being lost.

If you created a patch, please submit a [Pull
Request](https://github.com/opencloud-eu/desktop/pulls).

## Maintainers and Contributors

`OpenCloud Desktop` is developed by the `OpenCloud` community and [receives
patches from a variety of authors](https://github.com/opencloud-eu/desktop/graphs/contributors).


## License

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
    for more details.
