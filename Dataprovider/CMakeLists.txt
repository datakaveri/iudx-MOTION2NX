add_subdirectory(image_provider)
add_subdirectory(weights_provider)


if (MOTION_BUILD_ONNX_ADAPTER)
  add_subdirectory(onnx2motion)
endif (MOTION_BUILD_ONNX_ADAPTER)

if (MOTION_BUILD_HYCC_ADAPTER)
  add_subdirectory(hycc2motion)
endif (MOTION_BUILD_HYCC_ADAPTER)
