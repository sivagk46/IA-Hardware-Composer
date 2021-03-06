/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "videolayerrenderer.h"
#include <stdio.h>
#include <drm/drm_fourcc.h>

VideoLayerRenderer::VideoLayerRenderer(struct gbm_device* gbm_dev)
    : LayerRenderer(gbm_dev) {
}

VideoLayerRenderer::~VideoLayerRenderer() {
  if (resource_fd_)
    fclose(resource_fd_);
  resource_fd_ = NULL;
}

bool VideoLayerRenderer::Init(uint32_t width, uint32_t height, uint32_t format,
                              glContext* gl, const char* resource_path) {
  if (!LayerRenderer::Init(width, height, format, gl, resource_path))
    return false;
  if (!resource_path) {
    ETRACE("resource file no provided");
    return false;
  }

  resource_fd_ = fopen(resource_path, "r");
  if (!resource_fd_) {
    ETRACE("Could not open the resource file");
    return false;
  }

  return true;
}

static int get_bpp_from_format(uint32_t format, size_t plane) {
  // assert(plane < drv_num_planes_from_format(format));
  switch (format) {
    case DRM_FORMAT_BGR233:
    case DRM_FORMAT_C8:
    case DRM_FORMAT_R8:
    case DRM_FORMAT_RGB332:
    case DRM_FORMAT_YVU420:
      return 8;
    case DRM_FORMAT_NV12:
      return (plane == 0) ? 8 : 16;

    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_ABGR4444:
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_ARGB4444:
    case DRM_FORMAT_BGR565:
    case DRM_FORMAT_BGRA4444:
    case DRM_FORMAT_BGRA5551:
    case DRM_FORMAT_BGRX4444:
    case DRM_FORMAT_BGRX5551:
    case DRM_FORMAT_GR88:
    case DRM_FORMAT_RG88:
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_RGBA4444:
    case DRM_FORMAT_RGBA5551:
    case DRM_FORMAT_RGBX4444:
    case DRM_FORMAT_RGBX5551:
    case DRM_FORMAT_UYVY:
    case DRM_FORMAT_VYUY:
    case DRM_FORMAT_XBGR1555:
    case DRM_FORMAT_XBGR4444:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_XRGB4444:
    case DRM_FORMAT_YUYV:
    case DRM_FORMAT_YVYU:
      return 16;

    case DRM_FORMAT_BGR888:
    case DRM_FORMAT_RGB888:
      return 24;

    case DRM_FORMAT_ABGR2101010:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_ARGB2101010:
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_AYUV:
    case DRM_FORMAT_BGRA1010102:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_BGRX1010102:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_RGBA1010102:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_RGBX1010102:
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_XBGR2101010:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XRGB8888:
      return 32;
  }

  ETRACE("UNKNOWN FORMAT %d", format);
  return 0;
}

static uint32_t get_linewidth_from_format(uint32_t format, uint32_t width,
                                          size_t plane) {
  int stride = width * ((get_bpp_from_format(format, plane) + 7) / 8);

  /*
   * Only downsample for certain multiplanar formats which have horizontal
   * subsampling for chroma planes.  Only formats supported by our drivers
   * are listed here -- add more as needed.
   */
  if (plane != 0) {
    switch (format) {
      case DRM_FORMAT_NV12:
      case DRM_FORMAT_YVU420:
        stride = stride / 2;
        break;
    }
  }

  return stride;
}

static uint32_t get_height_from_format(uint32_t format, uint32_t height,
                                       size_t plane) {
  switch (format) {
    case DRM_FORMAT_BGR233:
    case DRM_FORMAT_C8:
    case DRM_FORMAT_R8:
    case DRM_FORMAT_RGB332:
    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_ABGR4444:
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_ARGB4444:
    case DRM_FORMAT_BGR565:
    case DRM_FORMAT_BGRA4444:
    case DRM_FORMAT_BGRA5551:
    case DRM_FORMAT_BGRX4444:
    case DRM_FORMAT_BGRX5551:
    case DRM_FORMAT_GR88:
    case DRM_FORMAT_RG88:
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_RGBA4444:
    case DRM_FORMAT_RGBA5551:
    case DRM_FORMAT_RGBX4444:
    case DRM_FORMAT_RGBX5551:
    case DRM_FORMAT_XBGR1555:
    case DRM_FORMAT_XBGR4444:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_XRGB4444:
    case DRM_FORMAT_BGR888:
    case DRM_FORMAT_RGB888:
    case DRM_FORMAT_ABGR2101010:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_ARGB2101010:
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_AYUV:
    case DRM_FORMAT_BGRA1010102:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_BGRX1010102:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_RGBA1010102:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_RGBX1010102:
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_XBGR2101010:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_UYVY:
    case DRM_FORMAT_VYUY:
    case DRM_FORMAT_YUYV:
    case DRM_FORMAT_YVYU:
      return height;
    case DRM_FORMAT_YVU420:
    case DRM_FORMAT_NV12:
      return (plane == 0) ? height : height / 2;
  }
  ETRACE("UNKNOWN FORMAT %d", format);
  return 0;
}

void VideoLayerRenderer::Draw(int64_t* pfence) {
  if (!resource_fd_) {
    return;
  }

  void* pOpaque = NULL;
  uint32_t width = native_handle_.import_data.width;
  uint32_t height = native_handle_.import_data.height;
  uint32_t mapStride;
  void* pBo = gbm_bo_map(gbm_bo_, 0, 0, width, height, GBM_BO_TRANSFER_WRITE,
                         &mapStride, &pOpaque, 0);
  if (!pBo) {
    ETRACE("gbm_bo_map is not successful!");
    return;
  }

  char* pReadLoc = NULL;
  for (int i = 0; i < planes_; i++) {
    uint32_t planeReadCount = 0;
    pReadLoc = (char*)pBo + native_handle_.import_data.offsets[i];

    uint32_t lineBytes =
        get_linewidth_from_format(native_handle_.import_data.format, width, i);
    uint32_t readHeight = 0;
    uint32_t planeHeight =
        get_height_from_format(native_handle_.import_data.format, height, i);

    while (readHeight < planeHeight) {
      uint32_t lineReadCount = fread(pReadLoc, 1, lineBytes, resource_fd_);
      if (lineReadCount <= 0) {
        fseek(resource_fd_, 0, SEEK_SET);
        break;
      } else if (lineReadCount != lineBytes) {
        ETRACE("Maybe not aligned video source file with line width!");
        break;
      }
      pReadLoc += native_handle_.import_data.strides[i];
      readHeight++;
    }
  }

  gbm_bo_unmap(gbm_bo_, pOpaque);
  *pfence = -1;
}
