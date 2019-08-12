# ps1-tests

## Tests

Name   | Description
-------|------------
timers | Run Timer0,1,2 using various clock sources and sync modes and time them using busy loops and vblank interrupt

## Download

[Latest release](https://github.com/JaCzekanski/ps1-tests/releases/latest)

## Build

```
docker run -it -v $(pwd):/build jaczekanski/psn00bsdk:latest make
```

Note: requires Docker
