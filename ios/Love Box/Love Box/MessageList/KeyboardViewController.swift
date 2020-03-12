//
//  KeyboardViewController.swift
//  Love Box
//
//  Created by Jack Cox on 3/3/20.
//  Copyright © 2020 CapTech Consulting. All rights reserved.
//

import UIKit

class KeyboardViewController: UIViewController, UITextFieldDelegate {

    @IBOutlet var field:UITextField!
    @IBOutlet var send:UIButton!
    
    var callback:(_ message:String) -> Void = { message in return }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.conditionallyEnableSendButton()

        // Do any additional setup after loading the view.
    }
    
    func conditionallyEnableSendButton() {
        if (self.field.text?.lengthOfBytes(using: .utf8) ?? 0 > 0) {
            self.send.isEnabled = true
        } else {
            self.send.isEnabled = false
        }
    }
    
    @IBAction func sendPushed(_ sender: Any) {
        if (self.field.text?.lengthOfBytes(using: .utf8) ?? 0 > 0) {
            if let text = self.field.text {
                self.callback(text)
            }
        }
        
        self.resignFirstResponder()
    }
    
    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destination.
        // Pass the selected object to the new view controller.
    }
    */

    func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString string: String) -> Bool {
        conditionallyEnableSendButton()
        return true
    }
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        if textField.text?.lengthOfBytes(using: .utf8) ?? 0 > 0 {
            self.sendPushed(textField)
            textField.text = ""
            return true
        }
        return false
    }
    

}
