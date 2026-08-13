//
//  ContentView.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import SwiftUI

struct Zyncounter: View {
    @State private var bleManager = BLEManager()

    var body: some View {
        Text(bleManager.statusText)
            .padding()
    }
}

#Preview {
    Zyncounter()
}
