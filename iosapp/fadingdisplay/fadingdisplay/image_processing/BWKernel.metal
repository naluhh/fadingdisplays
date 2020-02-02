//
//  BWKernel.metal
//  fadingdisplay
//
//  Created by Charly Delaroche on 1/26/20.
//  Copyright © 2020 Charly Delaroche. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;
#include <CoreImage/CoreImage.h> // includes CIKernelMetalLib.h

extern "C" { namespace coreimage {

    float4 myColor(sample_t s) {
        float d = min(dot(s.rgb, float3(0.299, 0.587, 0.114)), 1.0);

        return float4(d, d, d, 1.0);
    }

}}

