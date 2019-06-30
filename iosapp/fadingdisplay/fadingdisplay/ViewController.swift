//
//  ViewController.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 6/12/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import UIKit
import CoreImage

extension ViewController: CropViewControllerDelegate {
    func cropViewController(_ cropViewController: CropViewController, didCropToImage image: UIImage, withRect cropRect: CGRect, angle: Int) {
        currentImage.image = image
        cropViewController.dismissAnimatedFrom(self, toView: currentImage, toFrame: currentImage.frame, setup: nil, completion: nil)
    }
}

extension ViewController: UIImagePickerControllerDelegate, UINavigationControllerDelegate {
    func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey : Any]) {
        let filter = CIFilter.init(name: "CIPhotoEffectNoir")!
        var image: UIImage?

        if let chosenImage = info[UIImagePickerController.InfoKey.editedImage] as? UIImage {
            image = chosenImage
        } else if let chosenImage = info[UIImagePickerController.InfoKey.originalImage] as? UIImage {
            image = chosenImage
        }

        guard var choosenImage = image else {
            return
        }


        filter.setValue(CIImage(image: choosenImage), forKey: kCIInputImageKey)
        if let output = filter.outputImage {
            let outputUIImage = UIImage.init(ciImage: output)
            choosenImage = outputUIImage
        }

        picker.dismiss(animated: false, completion: nil)
        let cropViewController = CropViewController(image: choosenImage)
        cropViewController.delegate = self
        cropViewController.aspectRatioPreset = CropViewControllerAspectRatioPreset.presetCustom
        cropViewController.customAspectRatio = screenResolution
        cropViewController.aspectRatioLockEnabled = true
        self.present(cropViewController, animated: true, completion: nil)

        return

    }
}

class ViewController: UIViewController {
    let screenResolution = CGSize.init(width: 2880, height: 2160)
    let currentImage = UIImageView.init(image: nil)

    override func viewDidLoad() {
        super.viewDidLoad()


        currentImage.backgroundColor = UIColor.black
        currentImage.translatesAutoresizingMaskIntoConstraints = false
        // Do any additional setup after loading the view.

        view.addSubview(currentImage)
        NSLayoutConstraint.activate([
            currentImage.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            currentImage.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            currentImage.widthAnchor.constraint(equalTo: view.widthAnchor, multiplier: 0.6),
            currentImage.heightAnchor.constraint(equalTo: currentImage.widthAnchor, multiplier: screenResolution.height / screenResolution.width)
        ])

        let tapGesture = UITapGestureRecognizer.init(target: self, action: #selector(didTapImageView))

        currentImage.addGestureRecognizer(tapGesture)
        currentImage.isUserInteractionEnabled = true
    }

    @objc func didTapImageView(gest: UITapGestureRecognizer) {
        let controller = UIImagePickerController()
        controller.delegate = self
        present(controller, animated: true, completion: nil)
    }
}

