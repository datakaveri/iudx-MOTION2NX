#!/bin/bash

MAJOR_RELEASE=1
MINOR_RELEASE=0
PATCH_RELEASE=0
# last commit id of master branch
COMMIT_ID=`git log  -1 --pretty=%h`
# To be executed from project root
docker build -t ghcr.io/datakaveri/motion2nx:$MAJOR_RELEASE.$MINOR_RELEASE.$PATCH_RELEASE-$COMMIT_ID -f docker/Dockerfile . && \
docker push ghcr.io/datakaveri/motion2nx:$MAJOR_RELEASE.$MINOR_RELEASE.$PATCH_RELEASE-$COMMIT_ID
