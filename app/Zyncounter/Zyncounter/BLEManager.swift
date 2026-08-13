//
//  BLEManager.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import Foundation
import CoreBluetooth

@Observable
final class BLEManager: NSObject, CBCentralManagerDelegate {
    // Must match SERVICE_UUID in esp/hall-sensor-test/hall-sensor-test.ino
    static let serviceUUID = CBUUID(string: "96BDE720-973D-4F43-820B-0CD2FF8B666C")

    var statusText = "Not connected"

    private var centralManager: CBCentralManager!
    private var peripheral: CBPeripheral?

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            statusText = "Scanning..."
            central.scanForPeripherals(withServices: [Self.serviceUUID])
        } else {
            statusText = "Bluetooth is off"
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        central.stopScan()
        self.peripheral = peripheral
        statusText = "Connecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        statusText = "Connected to \(peripheral.name ?? "device")"
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        statusText = "Disconnected"
    }
}
