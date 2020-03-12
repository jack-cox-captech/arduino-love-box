//
//  DetailViewController.swift
//  Love Box
//
//  Created by Jack Cox on 3/1/20.
//  Copyright © 2020 CapTech Consulting. All rights reserved.
//

import UIKit
import CocoaMQTT
import CleanroomLogger

class DetailViewController: UIViewController {

    
    @IBOutlet weak var keyboardControlBottomConstraint:NSLayoutConstraint!
    
    @IBOutlet weak var detailDescriptionLabel: UILabel!
    var messageListViewController:MessageListTableViewController?
    
    var mqtt:CocoaMQTT?
    var detailItem: Box? {
        didSet {
            // Update the view.
            configureView()
        }
    }

    func configureView() {
        // Update the user interface for the detail item.
        if let detail = detailItem {
            if let label = detailDescriptionLabel {
                label.text = detail.holder
                self.connectToBox()
            }
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        


        registerForKeyboardNotifications()
        
        // Do any additional setup after loading the view.
        configureView()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        
        if let _ = self.detailItem {
            self.connectToBox()
        }
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        
        if let mqtt = self.mqtt {
            mqtt.disconnect()
        }
    }
    
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if segue.identifier == "embedKeyboardController" {
            if let ctl = segue.destination as? KeyboardViewController {
                ctl.callback = { message in
                    self.sendMessage(message)
                }
            }
        } else if segue.identifier == "embedMessageList" {
            if let ctl = segue.destination as? MessageListTableViewController {
                self.messageListViewController = ctl
            }
        }
    }
    
    func sendMessage(_ message:String) {
        let df = ISO8601DateFormatter()
        let iso = df.string(from: Date())
        Log.info?.message("sending message: \(message)")
        let message = Message(type: "message",
                              message_id: UUID.init().uuidString,
                              message_time: iso,
                              text: message)
        
        let jsonData = try! JSONEncoder().encode(message)
        let jsonString = String(data: jsonData, encoding: .utf8)!
        Log.info?.message("\(jsonString)")
        
        if let topic = self.detailItem?.topic {
            self.mqtt?.publish(topic, withString: jsonString)
        }
        
    }
    
    func registerForKeyboardNotifications() {
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardDidShow(notif:)), name: UIResponder.keyboardWillChangeFrameNotification, object: nil)
    }
    
    @objc func keyboardDidShow(notif:NSNotification) {
        if let userInfo = notif.userInfo {
            let endFrame = (userInfo[UIResponder.keyboardFrameEndUserInfoKey] as? NSValue)?.cgRectValue
            
            let endFrameY = endFrame?.origin.y ?? 0
            if endFrameY >= UIScreen.main.bounds.size.height {
                self.keyboardControlBottomConstraint.constant = 0.0
            } else {
                self.keyboardControlBottomConstraint.constant = endFrame?.size.height ?? 0.0
            }
        }
    }

    func connectToBox() {
        Log.debug?.message("Connecting to box held by \(String(describing: detailItem?.holder))")
        let clientID = "CocoaMQTT-" + String(ProcessInfo().processIdentifier)
        mqtt = CocoaMQTT(clientID: clientID, host: "m16.cloudmqtt.com", port: 27972)
        mqtt?.username = "miqfxuwi"
        mqtt?.password = "e-sHpugpRnj5"
        mqtt?.enableSSL = true
        mqtt?.allowUntrustCACertificate = false
        //mqtt.willMessage = CocoaMQTTWill(topic: "/will", message: "dieout")
        mqtt?.keepAlive = 60
        mqtt?.delegate = self
        _ = mqtt?.connect()
        
        
    }


    // TODO: Add unsubscription

}

extension DetailViewController : CocoaMQTTDelegate {
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        Log.debug?.message("mqtt:didConnectAck")
        
        if let box = self.detailItem {
            mqtt.subscribe(box.topic)
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
        Log.debug?.message("mqtt:didPublishMessage")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
        Log.debug?.message("publishAck")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        Log.debug?.message("mqtt:didReceiveMessage \(message)")
        
        let decoder = JSONDecoder()
        
        do {
            
            let data = Data(bytes: &message.payload, count: message.payload.count)
            let msg = try decoder.decode(Message.self, from: data)
            Log.debug?.message("received: \(msg)")
            if msg.type == "message" {
                self.messageListViewController?.addMessage(message: msg)
            }
        } catch {
            print(error.localizedDescription)
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopics success: NSDictionary, failed: [String]) {
        Log.debug?.message("mqtt:didSubscribeTopics \(success)")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopics topics: [String]) {
        Log.debug?.message("mqtt:didUnsubscribeTopics \(topics)")
    }
    
    func mqttDidPing(_ mqtt: CocoaMQTT) {
        Log.debug?.message("mqtt:didPing \(mqtt)")
    }
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {
        Log.debug?.message("mqtt:didPong \(mqtt)")
    }
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        Log.debug?.message("mqtt:didDisconnect \(mqtt) \(String(describing: err))")
    }
}
