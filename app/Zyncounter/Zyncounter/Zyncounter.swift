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
        VStack {
            Text(bleManager.statusText)
            Text(bleManager.characteristicValue)
        }
        .padding()
    }
}

#Preview {
    Zyncounter()
}
