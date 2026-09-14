//
//  ContentView.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import SwiftUI

struct Zyncounter: View {
    @State private var bleManager = BLEManager.shared

    var body: some View {
        VStack {
            Text(bleManager.statusText)
            ForEach(bleManager.receivedTimestamps, id: \.self) { timestamp in
                Text(timestamp.formatted())
            }
        }
        .padding()
    }
}

#Preview {
    Zyncounter()
}
