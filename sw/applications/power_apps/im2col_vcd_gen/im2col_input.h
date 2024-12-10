#ifndef INPUT_IMAGE_NCHW_H
#define INPUT_IMAGE_NCHW_H

/*
	Copyright EPFL contributors.
	Licensed under the Apache License, Version 2.0, see LICENSE for details.
	SPDX-License-Identifier: Apache-2.0
*/

#include <stdint.h>

#define IH 27
#define IW 27
#define CH 3
#define BATCH 1
#define FH 3
#define FW 3
#define TOP_PAD 2
#define BOTTOM_PAD 2
#define LEFT_PAD 2
#define RIGHT_PAD 2
#define STRIDE_D1 1
#define STRIDE_D2 1

extern const uint8_t input_image_nchw[2187];

#endif // INPUT_IMAGE_NCHW_H
