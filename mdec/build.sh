#!/bin/bash

docker run -it --rm -v "$PWD:/build" root670/docker-psxsdk make
