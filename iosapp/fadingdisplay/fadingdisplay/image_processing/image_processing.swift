//
//  image_processing.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 9/29/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import Foundation
import UIKit
import CoreImage

class MetalFilter: CIFilter {

    private let kernel: CIColorKernel

    var inputImage: CIImage?

    override init() {
        let url = Bundle.main.url(forResource: "default", withExtension: "metallib")!
        let data = try! Data(contentsOf: url)
        kernel = try! CIColorKernel(functionName: "myColor", fromMetalLibraryData: data)
        super.init()
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func outputImage() -> CIImage? {
        guard let inputImage = inputImage else {return nil}
        return kernel.apply(extent: inputImage.extent, arguments: [inputImage])
    }
}


extension UIImage {
    var convertToCG: UIImage {
//        CGImageRef img = [myContext createCGImage:ciImage fromRect:[ciImage extent]];
        let context: CIContext = CIContext.init(options: nil)
        if let ciImage = self.ciImage {
            let cgImage: CGImage = context.createCGImage(ciImage, from: ciImage.extent)!

            return UIImage(cgImage: cgImage)
        }

        return self
    }

//    var blackAndWhite: UIImage {
//        // Create image rectangle with current image width/height
//        let imageRect: CGRect = CGRect(x: 0, y: 0, width: size.width, height: size.height)
//
//        // Grayscale color space
//        let colorSpace = CGColorSpaceCreateDeviceGray()
//        let width = size.width
//        let height = size.height
//
//        // Create bitmap content with current image size and grayscale colorspace
//        let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.none.rawValue)
//
//        // Draw image into current context, with specified rectangle
//        // using previously defined context (with grayscale colorspace)
//        let context = CGContext(data: nil, width: Int(width), height: Int(height), bitsPerComponent: 8, bytesPerRow: 0, space: colorSpace, bitmapInfo: bitmapInfo.rawValue)
//        context?.draw(cgImage!, in: imageRect)
//        let imageRef = context!.makeImage()
//
//        // Create a new UIImage object
//        let newImage = UIImage(cgImage: imageRef!)
//
//        return newImage
//    }
}
