//
//  MainViewController.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 7/14/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import UIKit

class ImageCell: UICollectionViewCell {
    static let reuseIdentifier = "ImageCell"
    let imageView: UIImageView = UIImageView()

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

    override func prepareForReuse() {
        super.prepareForReuse()
        imageView.image = nil
    }

}

class MainViewController : UIViewController {
    let aspectRatio: CGFloat = 2880.0/2160.0
    var collectionViewLayout: UICollectionViewFlowLayout?
    var collectionView: UICollectionView?
    let addButton = UIButton()
    let requestManager = RequestManager()
    let gradientLayer = CAGradientLayer()

    // Data
    var ids: [Int] = []


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
        }) { (err) in

        }
    }

    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
}

extension MainViewController: UICollectionViewDelegate {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        if let cellView = collectionView.cellForItem(at: indexPath) as? ImageCell {
            cellView.imageView.layer.borderColor = UIColor.white.cgColor
            cellView.imageView.layer.borderWidth = 2
            cellView.contentView.layer.shadowPath = UIBezierPath(rect: cellView.imageView.bounds).cgPath
        }
    }

    func collectionView(_ collectionView: UICollectionView, didDeselectItemAt indexPath: IndexPath) {
        if let cellView = collectionView.cellForItem(at: indexPath) as? ImageCell {
            cellView.imageView.layer.borderWidth = 0
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
