# LAN-fileshare
Transfer files over LAN
# Usage
The person downloading uses `server.c` the person uploading uses `client.c`
### server.c
Compile and run `server.c` with the following arguments
* port - the port you want to use for this file transfer (the client will have to match this)
* directory - the directory where everything will be downloaded
### client.c
* ip - the local ip address of the machine running `server.c` (use `ip a` to find this on linux or `ipconfig` on windows)
* port - the port the server chose to host on
* directory1, directory2, ... - the directories to upload. NOTE: a folder will not be created for each directory and everything will be placed in the server's root directory.
### Important
This is incredibly unsecure as `server.c` downloads whatever `client.c` orders to, without checking the available disk space, the contents and without any form of encryption. If you want to you can encrypt files on the client's disk and decrypt them on the server using something like [xor encryption](https://github.com/justinas2314/XOR-Encryption).
### Speed
The speed depends almost entirely on your internet speed which you can check with online internet speed tests.
