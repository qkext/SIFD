# SIFD Samsung Interactive File Daemon
# Version 1.0
# My Github Profile: **https://github.com/qkext**

lightweight HTTP server and dynamic page engine
for Samsung Smart TVs running VDLinux.

## Overview

SIFD is a minimal HTTP/1.0 server designed specifically for Rooted Samsung Smart TVs

It replaces traditional PHP/web servers with a lightweight alternative that supports static files, directory listings, and a custom scripting language called **Samsung Interactive File (.sif)** .

## Target Platform

**LINUX-BASED SAMSUNG TV**

## Features

- **Static File Serving**: HTML, CSS, JS, images, video, fonts
- **Directory Listing**: Apache-style automatic directory index
- **SIF Scripting Language**: Custom tags for dynamic content
- **Secure Binary Execution**: Sandboxed execution from `bin/` directory
- **GET Parameters**: Query string parsing
- **File Reading**: Read system files like `/proc/version`
- **MIME Detection**: Automatic content type detection
- **Request Logging**: Persistent HTTP and execution logs
- **Minimal Resource Usage**: Single-threaded, select()-based I/O

## SIF Tags

| Tag | Description | Example |
|-----|-------------|---------|
| `<sif echo="">` | Output text | `<sif echo="Hello"></sif>` |
| `<sif include="">` | Include another file | `<sif include="header.sif"></sif>` |
| `<sif read="">` | Read file contents | `<sif read="/proc/version"></sif>` |
| `<sif get="">` | GET parameter value | `<sif get="name"></sif>` |
| `<sif exec="">` | Execute binary | `<sif exec="sysinfo"></sif>` |

## Building

### Prerequisites

- Samsung cross-compiler: `arm-v7a8v4r3-linux-gnueabi-g++`
- GNU Make or direct g++ compilation
- Toolchain: "http://download.samygo.tv/Toolchains/arm-v7a8v4r3.tar.xz"

### Compilation

```bash
# Using the build command directly
/opt/arm-v7a8v4r3/bin/arm-v7a8v4r3-linux-gnueabi-g++ \
    -O2 -Wall -std=c++11 \
    -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
    -ffunction-sections -fdata-sections \
    -Wl,--gc-sections -Wl,--as-needed \
    -o sifd \
    utils.cpp logger.cpp mime.cpp exec.cpp listing.cpp \
    sif.cpp http.cpp server.cpp main.cpp \
    -lpthread -lrt

**Usage**

./sifd \
-f /mtd_rwarea/website \
-p 5555 \
--index index.sif \
--log /mtd_rwarea/sifd.log
