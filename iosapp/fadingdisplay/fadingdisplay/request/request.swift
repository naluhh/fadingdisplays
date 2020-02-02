//
//  request.swift
//  fadingdisplay
//
//  Created by Charly Delaroche on 7/14/19.
//  Copyright © 2019 Charly Delaroche. All rights reserved.
//

import Foundation
import UIKit

#if MOCK_API
var mock_images = ["aeroplane-airplane-architectural-design-1010079-out.png-crop",
    "aging-black-and-white-homeless-34534-out.png",
    "animals-black-and-white-equine-52500-out.png",
    "architecture-black-and-white-bridge-164357-out.png",
    "asphalt-bitumen-black-and-white-1038935-out.png",
    "astronomy-black-and-white-bright-47367-out.png",
    "attractive-beautiful-beauty-594421-out.png",
    "bare-tree-black-and-white-black-and-white-753575-out.png",
    "beautiful-black-and-white-bloom-414214-out.png",
    "big-ben",
    "black-and-white-black-and-white-card-1496146-out.png",
    "black-and-white-black-and-white-monochrome-707676-out.png",
    "black-and-white-bloom-blossom-57905-out.png",
    "black-and-white-bookcase-books-256453-out.png",
    "black-and-white-branch-cherry-blossom-34435-out.png",
    "black-and-white-child-cute-48794-out.png",
    "black-and-white-close-up-drop-of-water-823-out.png",
    "black-and-white-perspective-railroad-285286-out.png",
    "black-black-and-white-close-up-1053924-out.png",
    "dandelion.png",
    "lake_waterfall.png",
    "trees-crop.png",
    "africa-animal-black-and-white-39245-out.png",
    "aircraft-black-and-white-cold-1867001-out.png",
    "animal-cat-eyes-33537-out.png",
    "aquatic-plant-beautiful-black-and-white-1563895-out.png",
    "architecture-black-and-white-building-358345-out.png",
    "black-and-white-black-and-white-boats-33229-out.png",
    "black-black-and-white-black-and-white-236296-out.png",
    "black-white-black-and-white-board-game-957312-out.png"]

var extra_images: [UIImage] = []
#endif
typealias ImageID = Int

class RequestManager {
    var baseUrl = "http://192.168.86.160:3000/"
//        var baseUrl = "http://192.168.86.27:3000/"


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
        #if MOCK_API
        successCallback(Array<ImageID>(0...mock_images.count - 1 + extra_images.count - 1));
            return
        #endif


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
        #if MOCK_API
        extra_images.append(image)
        successCallback(mock_images.count + extra_images.count - 1)
        return
        #endif

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
        #if MOCK_API
        if id >= mock_images.count {
            successCallback(extra_images[id - mock_images.count])
        } else if let image = UIImage(named: mock_images[id]) {
            successCallback(image);
        }
        return;
        #endif

        let url = baseUrl + "library/\(id)"

        let task = URLSession.shared.dataTask(with: URL(string: url)!) { (data, response, error) in
            guard let downloadedData = data, let image = UIImage(data: downloadedData) else {
                if let error = error {
                    DispatchQueue.main.async {
                        errorCallback(error)
                    }
                }
                return
            }

            DispatchQueue.main.async {
                successCallback(image)
            }
        }

        task.resume()
    }
}
