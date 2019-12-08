//
//  MainViewController.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 7/14/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import UIKit
import CoreImage

let screenResolution = CGSize.init(width: 5616, height: 3744)

class ImageCell: UICollectionViewCell {
    static let reuseIdentifier = "ImageCell"
    let imageView: UIImageView = UIImageView()

    override var isSelected: Bool {
        set {
            super.isSelected = newValue
            updateSelectionState(selected: newValue)
        }
        get {
            return super.isSelected
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)

        contentView.addSubview(imageView)
        imageView.isUserInteractionEnabled = false
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        imageView.frame = contentView.bounds
    }

    func configure(image: UIImage) {
        imageView.image = image
    }


    func updateSelectionState(selected: Bool) {
        if selected {
            imageView.layer.borderColor = UIColor.white.cgColor
            imageView.layer.borderWidth = 2
            contentView.layer.shadowPath = UIBezierPath(rect: imageView.bounds).cgPath
        } else {
            imageView.layer.borderWidth = 0
        }
    }

    override func prepareForReuse() {
        super.prepareForReuse()
        updateSelectionState(selected: false)
        imageView.image = nil
    }
}

class MainViewController : UIViewController {
    let aspectRatio: CGFloat = 3.0/2.0
    var collectionViewLayout: UICollectionViewFlowLayout?
    var collectionView: UICollectionView?
    let addButton = UIButton()
    let requestManager = RequestManager()
    let gradientLayer = CAGradientLayer()
    var longPress: UILongPressGestureRecognizer?

    // Data
    var ids: [Int] = []
    var selected_id: Int = -1


    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()

        gradientLayer.frame = CGRect(x: 0, y: addButton.frame.minY - 32, width: view.frame.width, height: view.frame.height - addButton.frame.minY + 32)
        updateCollectionViewLayout()
    }

    func updateCollectionViewLayout() {
        let itemWCount: CGFloat = CGFloat(Int(view.frame.width / 120.0))
        let leftPadding = max(2, view.safeAreaInsets.left)
        let rightPadding = max(2, view.safeAreaInsets.right)
        let availableWidth = view.frame.width - itemWCount * 2.0 - leftPadding - rightPadding
        collectionViewLayout?.itemSize = CGSize(width: availableWidth / itemWCount, height: availableWidth / itemWCount * 1.0 / aspectRatio)
        collectionViewLayout?.sectionInset = UIEdgeInsets(top: 2, left: leftPadding, bottom: view.frame.height - addButton.frame.minY, right: rightPadding)
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        collectionViewLayout = UICollectionViewFlowLayout()
        collectionView = UICollectionView.init(frame: .zero, collectionViewLayout: collectionViewLayout!)
        longPress = UILongPressGestureRecognizer(target: self, action: #selector(didLongPress))
        longPress?.minimumPressDuration = 2

        collectionView?.addGestureRecognizer(longPress!)
        guard let collectionView = collectionView else {
            return
        }
        view.backgroundColor = UIColor.black
        collectionView.backgroundColor = .black
        collectionView.alwaysBounceVertical = true
        collectionView.delegate = self
        collectionView.dataSource = self
        collectionView.register(ImageCell.self, forCellWithReuseIdentifier: ImageCell.reuseIdentifier)
        collectionViewLayout?.minimumInteritemSpacing = 2.0
        collectionViewLayout?.minimumLineSpacing = 2.0
        updateCollectionViewLayout()
        view.addSubview(collectionView)

        collectionView.translatesAutoresizingMaskIntoConstraints = false

        view.layer.addSublayer(gradientLayer)
        gradientLayer.colors = [UIColor.black.withAlphaComponent(0.0).cgColor, UIColor.black.cgColor]

        addButton.layer.shadowPath = UIBezierPath(ovalIn: CGRect(x: 0, y: 0, width: 48, height: 48)).cgPath
        addButton.layer.shadowColor = UIColor.black.cgColor
        addButton.layer.shadowRadius = 3.0
        addButton.layer.shadowOpacity = 1.0
        addButton.layer.shadowOffset = .zero
        addButton.translatesAutoresizingMaskIntoConstraints = false
        addButton.setImage(UIImage(named: "icAdd"), for: .normal);
        addButton.addTarget(self, action: #selector(addPhoto), for: .touchUpInside)
        view.addSubview(addButton)

        NSLayoutConstraint.activate([
            collectionView.leftAnchor.constraint(equalTo: view.leftAnchor),
            collectionView.rightAnchor.constraint(equalTo: view.rightAnchor),
            collectionView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            collectionView.topAnchor.constraint(equalTo: view.topAnchor),
            addButton.widthAnchor.constraint(equalToConstant: 48),
            addButton.heightAnchor.constraint(equalToConstant: 48),
            addButton.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -8),
            addButton.centerXAnchor.constraint(equalTo: view.centerXAnchor)
            ])

        requestManager.getLibrary(successCallback: { [weak self] (imgs) in
            guard let strongSelf = self else {
                return
            }
            strongSelf.ids = imgs
            strongSelf.collectionView?.reloadData()
            strongSelf.updateSelectedId()
        }) { (err) in

        }

        requestManager.getCurrentItem(successCallback: { (id) in
            self.selected_id = id
            self.updateSelectedId()
        }) { (err) in

        }
    }

    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }

//    func indexToId(index: Int) -> ImageID? {
//
//    }

    func updateSelectedId() {
        var i = 0

        for image_id in ids {
            if image_id == selected_id {
                collectionView?.selectItem(at: IndexPath(item: i, section: 0), animated: true, scrollPosition: .centeredVertically)
                break
            }
            i += 1
        }
    }

    @objc func didLongPress(gest: UILongPressGestureRecognizer) {
        let pos = gest.location(in: collectionView!)

        if let indexPath = collectionView?.indexPathForItem(at: pos) {
            let toRemove = ids[indexPath.item]
            requestManager.deleteItemWithId(id: toRemove, successCallback: {
                self.ids.remove(at: indexPath.item)
                self.collectionView?.reloadData()
            }) { (err) in
                print("failed: \(err)")
            }
        }
    }

    @objc func addPhoto() {
        let controller = UIImagePickerController()
        controller.delegate = self
        present(controller, animated: true, completion: nil)
    }
}

extension MainViewController: UIImagePickerControllerDelegate, UINavigationControllerDelegate {
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

extension MainViewController: CropViewControllerDelegate {
    func cropViewController(_ cropViewController: CropViewController, didCropToImage image: UIImage, withRect cropRect: CGRect, angle: Int) {
        cropViewController.dismiss(animated: false, completion: nil)

        requestManager.uploadImage(image: image.convertToCG, successCallback: { [weak self] (image) in
            guard let strongSelf = self else {
                return
            }
            strongSelf.ids.append(image)
            strongSelf.collectionView?.reloadData()
            strongSelf.updateSelectedId()
        }) { (err) in

        }
    }
}

extension MainViewController: UICollectionViewDelegate {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        selected_id = ids[indexPath.item]
        requestManager.setCurrentItem(newSelection: ids[indexPath.item], successCallback: {

        }) { (err) in

        }
    }
}

extension MainViewController: UICollectionViewDataSource {
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return self.ids.count
    }

    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        guard let cell = collectionView.dequeueReusableCell(withReuseIdentifier: ImageCell.reuseIdentifier, for: indexPath) as? ImageCell else {
            return UICollectionViewCell()
        }

        requestManager.getImageWithId(id: ids[indexPath.item], successCallback: { (img) in
            cell.configure(image: img)
        }) { (err) in

        }
        return cell
    }
}
