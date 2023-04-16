#!/bin/bash
# To be executed from project root
docker buildx build    --cache-to type=inline   --cache-from type=registry,ref=ghcr.io/datakaveri/motion2nx:build --push  -t ghcr.io/datakaveri/motion2nx:build -f docker/Dockerfile.build  .