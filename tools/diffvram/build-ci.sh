#!/bin/bash

name="diffvram"
docker_image="golang:1.12-alpine"
platforms=("windows/amd64" "linux/amd64" "darwin/amd64")
basedir=$(dirname "$0")

pushd $basedir
for platform in "${platforms[@]}"
do
    platform_split=(${platform//\// })
    export GOOS=${platform_split[0]}
    export GOARCH=${platform_split[1]}
    output_name=$name'-'$GOOS'-'$GOARCH
    if [ $GOOS = "windows" ]; then
        output_name+='.exe'
    fi

    echo Building $output_name
    docker run -e GOOS -e GOARCH -v $(pwd):/build -w /build golang:1.12-alpine go build -o $output_name .
    if [ $? -ne 0 ]; then
        echo 'An error has occurred! Aborting the script execution...'
        exit 1
    fi
done

popd
