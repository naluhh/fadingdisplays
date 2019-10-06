//
//  image_processing.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 9/29/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import Foundation
import UIKit

extension UIImage {
    var blackAndWhite: UIImage {
        // Create image rectangle with current image width/height
        let imageRect: CGRect = CGRect(x: 0, y: 0, width: size.width, height: size.height)

        // Grayscale color space
        let colorSpace = CGColorSpaceCreateDeviceGray()
        let width = size.width
        let height = size.height

        // Create bitmap content with current image size and grayscale colorspace
        let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.none.rawValue)

        // Draw image into current context, with specified rectangle
        // using previously defined context (with grayscale colorspace)
        let context = CGContext(data: nil, width: Int(width), height: Int(height), bitsPerComponent: 8, bytesPerRow: 0, space: colorSpace, bitmapInfo: bitmapInfo.rawValue)
        context?.draw(cgImage!, in: imageRect)
        let imageRef = context!.makeImage()

        // Create a new UIImage object
        let newImage = UIImage(cgImage: imageRef!)

        return newImage
    }
}
