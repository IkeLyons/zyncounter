//
//  ZyncounterApp.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import SwiftUI

@main
struct ZyncounterApp: App {
    @UIApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

    var body: some Scene {
        WindowGroup {
            Zyncounter()
        }
    }
}
