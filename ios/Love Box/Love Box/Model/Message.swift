//
//  Message.swift
//  Love Box
//
//  Created by Jack Cox on 3/1/20.
//  Copyright © 2020 CapTech Consulting. All rights reserved.
//

import Foundation


struct Message: Codable {
    var type: String
    var message_id: String
    var message_time: String
    var text: String
}
