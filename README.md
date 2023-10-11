# ntpclient
A simple Windows NTP client implementation in C++ with time synchronization.

## Usage
```
ntpclient.exe [-server name] [-port number] [-sync]

-server <name>  Set NTP server (default pool.ntp.org).
-port <number>  Set TNP port number (default 123).
-sync           Set the computer clock.
```

## Notes

- Created with Visual Studio 2022
- Tested with MinGW-w64 11.0 (GCC 13.2)
