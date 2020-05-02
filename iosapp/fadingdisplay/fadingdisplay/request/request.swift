//
//  request.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 7/14/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import Foundation
import UIKit

typealias ImageID = Int

extension UIImage {

    func resize(target: CGSize) -> UIImage? {
        UIGraphicsBeginImageContext(target)
        self.draw(in: CGRect(x: 0, y: 0, width: target.width, height: target.height))
        let newImage = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()

        return newImage
    }

    class func withImageIdFromCache(imageId: ImageID) -> UIImage? {
        let named = "\(imageId).png"
        if let dir = try? FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false) {
            return UIImage(contentsOfFile: URL(fileURLWithPath: dir.absoluteString).appendingPathComponent(named).path)
        }
        return nil
    }

    func saveWithImageId(imageId: ImageID) {
        guard let data = self.jpegData(compressionQuality: 1.0) else {
            return
        }
        guard let directory = try? FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false) as NSURL else {
            return
        }
        do {
            let named = "\(imageId).png"
            try data.write(to: directory.appendingPathComponent(named)!)
            return
        } catch {
            print(error.localizedDescription)
            return
        }
    }
}

class RequestManager {
//    var baseUrl = "http://192.168.86.160:3000/"
        var baseUrl = "http://192.168.86.160:3000/"


    func getCurrentItem(successCallback: @escaping (ImageID)->(), errorCallback: @escaping (Error)->()) {
        let url = baseUrl + "current"

        let task = URLSession.shared.dataTask(with: URL(string: url)!) { (data, response, error) in
            guard let data = data, let json = try? JSONSerialization.jsonObject(with: data, options: []) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            if let dict = json as? [String: ImageID], let lib = dict["selected_image"] {
                DispatchQueue.main.async {
                    successCallback(lib)
                }
            }
        }

        task.resume()
    }

    func deleteItemWithId(id: ImageID, successCallback: @escaping ()->(), errorCallback: @escaping (Error)->()) {
        let url = baseUrl + "library/\(id)"
        var request = URLRequest(url: URL(string: url)!)
        request.httpMethod = "DELETE"


        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in
            guard let data = data, let json = try? JSONSerialization.jsonObject(with: data, options: []) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            DispatchQueue.main.async {
                successCallback()
            }
        }

        task.resume()
    }

    func setCurrentItem(newSelection: ImageID, successCallback: @escaping ()->(), errorCallback: @escaping (Error)->()) {
        let url = baseUrl + "current/\(newSelection)"
        var request = URLRequest(url: URL(string: url)!)
        request.httpMethod = "PUT"


        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in
            guard let data = data, let json = try? JSONSerialization.jsonObject(with: data, options: []) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            DispatchQueue.main.async {
                successCallback()
            }
        }

        task.resume()
    }

    func getLibrary(successCallback: @escaping ([ImageID])->(), errorCallback: @escaping (Error)->()) {
        let url = baseUrl + "library"

        let task = URLSession.shared.dataTask(with: URL(string: url)!) { (data, response, error) in
            guard let data = data, let json = try? JSONSerialization.jsonObject(with: data, options: []) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            if let dict = json as? [String: [ImageID]], let lib = dict["library"] {
                DispatchQueue.main.async {
                    successCallback(lib)
                }
            }
        }

        task.resume()
    }

    func uploadImage(image: UIImage, successCallback: @escaping (ImageID)->(), errorCallback: @escaping (Error)->()) {
        guard let imageData = image.pngData() else {
            return
        }
        let filename = "avatar.png"
        let boundary = UUID().uuidString
        let url = baseUrl + "libraryp"
        var request = URLRequest(url: URL(string: url)!)
        request.httpMethod = "POST"
        request.setValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")

        var data = Data()
        data.append("\r\n--\(boundary)\r\n".data(using: .utf8)!)
        data.append("Content-Disposition: form-data; name=\"picture\"; filename=\"\(filename)\"\r\n".data(using: .utf8)!)
        data.append("Content-Type: image/png\r\n\r\n".data(using: .utf8)!)
        data.append(imageData)

        // End the raw http request data, note that there is 2 extra dash ("-") at the end, this is to indicate the end of the data
        // According to the HTTP 1.1 specification https://tools.ietf.org/html/rfc7230
        data.append("\r\n--\(boundary)--\r\n".data(using: .utf8)!)


        URLSession.shared.uploadTask(with: request, from: data) { (data, response, error) in
            guard let data = data, let json = try? JSONSerialization.jsonObject(with: data, options: []) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            if let dict = json as? [String: ImageID], let added_item = dict["new_item"] {
                DispatchQueue.main.async {
                    successCallback(added_item)
                }
            }
        }.resume()
    }

    func getImageWithId(id: ImageID, successCallback: @escaping (UIImage)->(), errorCallback: @escaping (Error)->()) {
        let url = baseUrl + "library/\(id)"

        DispatchQueue.global().async {
            if let image = UIImage.withImageIdFromCache(imageId: id) {
                DispatchQueue.main.async {
                    successCallback(image)
                }
                return
            }

            let task = URLSession.shared.dataTask(with: URL(string: url)!) { (data, response, error) in
                guard let downloadedData = data, var image = UIImage(data: downloadedData) else {
                    if let error = error {
                        DispatchQueue.main.async {
                            errorCallback(error)
                        }
                    }
                    return
                }

                if image.size.width > 1000 || image.size.height > 1000 {
                    let factor = 1000.0 / max(image.size.width, image.size.height)
                    let target = CGSize(width: (image.size.width * factor).rounded(), height: (image.size.height * factor).rounded())
                    if let newImage = image.resize(target: target) {
                        image = newImage
                    }
                }
                image.saveWithImageId(imageId: id)
                DispatchQueue.main.async {
                    successCallback(image)
                }
            }

            task.resume()
        }
    }
}
